#include "StdAfx.h"
#include "ProxyServer.h"

CProxyServer::CProxyServer() :
    m_pFreeList(NULL)
{

}

CProxyServer::~CProxyServer()
{
    for (map<DPID, ProxyClient*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
        delete it->second;
}

void CProxyServer::OnClientConnected(DPID dpId)
{
    DWORD dwIP = GetClientIP(dpId);

    auto client = AllocateClient(dpId);
    if (!GetClientIP(dpId, client->szIP))
    {
        WriteDebug("[New client] Fatal error, could not retrieve IP for socket: %u", dpId);
        *client->szIP = 0;
        FreeClient(client);
        return;
    }
    client->dwIP = dwIP;
    WriteDebug("[%s] connected", client->szIP);

    client->connectionInfo.csState = CST_ESTABLISHING;
    strcpy(client->connectionInfo.szLocalIp, client->szIP);

    m_clients.insert(pair<DPID, ProxyClient*>(dpId, client));
}

void CProxyServer::OnClientDisconnected(DPID dpId)
{
    map<DPID, ProxyClient*>::iterator it = m_clients.find(dpId);
    if (it == m_clients.end())
    {
        WriteDebug("FATAL error in OnClientDisconnected (dpId not found in clients)");
        return;
    }

    ProxyClient* client = it->second;
    m_clients.erase(it);

    // do some shit here

    WriteDebug("[%s] disconnected", client->szIP);
    FreeClient(client);
}

void CProxyServer::OnClientDataReceived(DPID dpId, LPBYTE lpByte, DWORD dwSize)
{
    map<DPID, ProxyClient*>::iterator it = m_clients.find(dpId);
    if (it == m_clients.end())
    {
        WriteDebug("FATAL error in OnClientDataReceived (dpId not found in clients)");
        return;
    }

    ProxyClient* client = it->second;

    if (client->IsDisconnect())
        return;

    static char szHex[65536];

    //GetHex( lpByte, dwSize, szHex );
    //printf( "%s\n", szHex );

    // client sends first 05 01 00 -- [1b:socks_ver][1b:nr_of_auths][x:auths]
    // server sends 05 00
    // client sends 05 01 00 01 ad c2 20 18 00 50
    if (client->stage != STAGE_ESTABLISHED)
    {
        client->buffer.SetOffset(client->buffer.GetLength());
        client->buffer.Write(lpByte, dwSize);

        client->buffer.SetOffset(0);
    }

    LPBYTE lpBuf = client->buffer.GetBuffer();
    size_t buflen = client->buffer.GetLength();

    if (client->stage == STAGE_INITIAL)
    {
        // verify size
        if (buflen >= 2 && // make sure theres atleast 2 bytes for ver & nr of auths
            buflen - 2 >= lpBuf[1]) // check that we received all 
        {
            if (*lpBuf != 0x05) // SOCKS version is not 5
            {
                WriteDebug("[AUTH] Version received from %s is not SOCKS5", client->szIP);
                client->Disconnect();
                return;
            }

            unordered_set<BYTE> auths;
            //printf( "AUTHS: " );
            for (BYTE n = 0; n < lpBuf[1]; ++n)
            {
                auths.insert(lpBuf[2 + n]);
                /*if( n > 0 )
                    printf( ", " );
                switch( lpBuf[ 2 + n ] )
                {
                case 0x00: printf( "NoAuth" ); break;
                case 0x01: printf( "GSSAPI" ); break;
                case 0x02: printf( "UserPass" ); break;
                default: printf( "0x%02X", lpBuf[ 2 + n ] ); break;
                }*/
            }
            //printf( "\n" );

            if (auths.find(SOCKSAUTH_NONE) == auths.end())
            {
                /*
                BYTE buf[] = { 0x05, 0xff }; // socks5_ver, 0xff <--- notifies the client that it provided wrong authentication methods
                Send( dpId, buf, sizeof( buf ) );
                */

                WriteDebug("[AUTH] Unknown authentication methods provided by client %s", client->szIP);
                client->Disconnect();
                return;
            }

            client->buffer.Remove(2 + lpBuf[1]);
            if (client->buffer.GetLength() > 0)
            {
                WriteDebug("ERR_DATA_NOT_CLEARED in STAGE_INITIAL (remain: %u)", client->buffer.GetLength());
            }

            client->stage = STAGE_REQUEST;

            BYTE buf[] = { 0x05, 0x00 }; // socks5_ver, auth method (0x00 for no auth)
            Send(dpId, buf, sizeof(buf));
        }
    }
    else if (client->stage == STAGE_LOGIN)
    {
        // parse user & pass
    }
    else if (client->stage == STAGE_REQUEST)
    {
        // client sends
        // 05            01      00           01            ad c2 20 18            00 50
        // [1b:socks_ver][1b:cmd][1b:reserved][1b:addr_type][x:destination_address][2b:port]
        // atleast 6 bytes (without destination_address)
        if (buflen > 6) // requires at least 1 byte in destination_address
        {
            if (*lpBuf != 0x05)
            {
                WriteDebug("[AUTH] Invalid SOCKS version in request from %s: 0x%02X", client->szIP, *lpBuf);
                client->Disconnect();
                return;
            }

            // very all data has received
            //BYTE addr_type = lpBuf[ 3 ];
            BOOL bAllDataReceived = FALSE;
            switch (lpBuf[3]) // addr_type
            {
            case 0x01: // ipv4 address
                bAllDataReceived = buflen >= 10;
                break;
            case 0x03: // domain name
                bAllDataReceived = buflen - 7 >= lpBuf[4];
                break;
            case 0x04: // ipv6 address
                WriteDebug("[AUTH] IPv6 not supported, from %s", client->szIP);
                client->Disconnect();
                return;
            }

            if (bAllDataReceived)
            {
                if (lpBuf[1] != 0x01) // command must be 0x01 = establish a TCP/IP stream connection
                {
                    WriteDebug("[AUTH] Unsupported command from %s: 0x%02X", client->szIP, lpBuf[1]);
                    client->Disconnect();
                    return;
                }

                sockaddr_in addr;
                addr.sin_family = AF_INET;

                if (lpBuf[3] == 0x03)
                {
                    // do domain resolve
                    BYTE domainLen = lpBuf[4];
                    if (domainLen + 1 > 128)
                    {
                        WriteDebug("[AUTH] Too long domain from %s (%u bytes)", client->szIP, domainLen);
                        client->Disconnect();
                        return;
                    }

                    char szDomain[128];
                    memcpy(szDomain, lpBuf + 5, domainLen);
                    szDomain[domainLen] = 0;

                    addr.sin_port = *(u_short*)(lpBuf + 5 + domainLen);

                    // resolve
                    LPHOSTENT lpHost = gethostbyname(szDomain);
                    if (lpHost)
                        addr.sin_addr.s_addr = *(u_long*)lpHost->h_addr_list[0];
                    else
                        addr.sin_addr.s_addr = inet_addr(szDomain);
                }
                else
                {
                    addr.sin_addr.s_addr = *(DWORD*)(lpBuf + 4);
                    addr.sin_port = *(u_short*)(lpBuf + 8);
                }

                client->stage = STAGE_CONNECTING;

                //printf( "Sent Request Granted\n" );
                //client->SendRequestError( 0x00, addr.sin_addr.s_addr, addr.sin_port );

                strcpy(client->connectionInfo.szRemoteIp, inet_ntoa(addr.sin_addr));
                client->connectionInfo.wPort = ntohs(addr.sin_port);
                UI_PrintConnections();
                WriteDebug("Connecting %s -> %s:%u", client->szIP, client->connectionInfo.szRemoteIp, client->connectionInfo.wPort);
                // todo: add connecting code here for screen

                client->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (connect(client->Socket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
                {
                    WriteDebug("FAILED %s -> %s:%u (ERR: %u)", client->szIP, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), WSAGetLastError());
                    client->SendRequestError(0x04, addr.sin_addr.s_addr, addr.sin_port);
                    client->Disconnect();
                    return;
                }

                client->SendRequestError(0x00, addr.sin_addr.s_addr, addr.sin_port);
                client->stage = STAGE_ESTABLISHED;

                client->connectionInfo.csState = CST_CONNECTED;
                //UI_PrintConnections();
            }
        }
    }
    else if (client->stage == STAGE_ESTABLISHED)
    {
        int len = send(client->Socket, (const char*)lpByte, (int)dwSize, 0);
        if (len != dwSize)
        {
            WriteDebug("ERROR_TOO_LITTLE_SENT");
        }
        g_nTotalOut += len;
        client->connectionInfo.dataOut += len;
        //printf( "[C->S] %u bytes\n", (int)dwSize );
    }
}

void CProxyServer::OnSendCompleted(DPID dpId)
{

}

void CProxyServer::Process()
{
    for (map<DPID, ProxyClient*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (!it->second->IsDisconnect())
            it->second->Process();
    }
}

ProxyClient* CProxyServer::AllocateClient(DPID dpId)
{
    auto client = m_pFreeList;
    if (client)
        m_pFreeList = m_pFreeList->next;
    else
        client = new ProxyClient(this);

    client->Init(dpId);
    return client;
}

void CProxyServer::FreeClient(ProxyClient* client)
{
    client->Free(); // close remote proxy socket and such

#ifdef _DEBUG
    client->Init(DPID_UNKNOWN); // Re-initialize all variables for debugging
#endif // _DEBUG

    client->next = m_pFreeList;
    m_pFreeList = client;
}