#include "StdAfx.h"
#include "HttpProxyServer.h"

#include "../Shared/HostnameExtractor.inl"

CHttpProxyServer::CHttpProxyServer() :
    m_pFreeList(NULL)
{

}

CHttpProxyServer::~CHttpProxyServer()
{
    for (map<DPID, HttpProxyClient*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
        delete it->second;
}

void CHttpProxyServer::OnClientConnected(DPID dpId)
{
    DWORD dwIP = GetClientIP(dpId);

    auto client = AllocateClient(dpId);
    if (!GetClientIP(dpId, client->szIP))
    {
        WriteDebug("[New client] Fatal error, could not retrieve IP for socket: %u", dpId);
        *client->szIP = 0;
        FreeClient(client);
        Disconnect(dpId);
        return;
    }
    client->dwIP = dwIP;
    WriteDebug("[%s-%u] connected", client->szIP, client->dpId);

    //client->SendHtml( "This is not your scene anymore." );
    //client->Disconnect();

    client->connectionInfo.csState = CST_ESTABLISHING;
    strcpy(client->connectionInfo.szLocalIp, client->szIP);

    m_clients.insert(pair<DPID, HttpProxyClient*>(dpId, client));
}

void CHttpProxyServer::OnClientDisconnected(DPID dpId)
{
    map<DPID, HttpProxyClient*>::iterator it = m_clients.find(dpId);
    if (it == m_clients.end())
    {
        WriteDebug("FATAL error in OnClientDisconnected (dpId %u not found in clients)", dpId);
        return;
    }

    HttpProxyClient* client = it->second;
    m_clients.erase(it);

    // do some shit here

    WriteDebug("[%s-%u] disconnected", client->szIP, client->dpId);
    FreeClient(client);
}

#define SET_STAGE( _stage ) { client->stage = _stage; switch(_stage){\
case STAGE_CONNECTING: WriteDebug( __FUNCTION__ " - SetStage( STAGE_CONNECTING ), Socket: %u", client->dpId ); break; \
case STAGE_REQUEST: WriteDebug( __FUNCTION__ " - SetStage( STAGE_REQUEST ), Socket: %u", client->dpId ); break; \
case STAGE_ESTABLISHED: WriteDebug( __FUNCTION__ " - SetStage( STAGE_ESTABLISHED ), Socket: %u", client->dpId ); break; \
}}

void CHttpProxyServer::OnClientDataReceived(DPID dpId, LPBYTE lpByte, DWORD dwSize)
{
    map<DPID, HttpProxyClient*>::iterator it = m_clients.find(dpId);
    if (it == m_clients.end())
    {
        WriteDebug("FATAL error in OnClientDataReceived (dpId %u not found in clients)", dpId);
        return;
    }

    HttpProxyClient* client = it->second;

    if (client->IsDisconnect())
        return;

    static char szHex[65536];

    DWORD dwMaxPrint = min(sizeof(szHex) - 1, dwSize);
    memcpy(szHex, lpByte, dwMaxPrint);
    *(szHex + dwMaxPrint) = 0;

    WriteDebug("Socket %u\r\n\r\n%s", client->dpId, szHex);

    //if( client->stage != STAGE_ESTABLISHED )
    {
        client->buffer.SetOffset(client->buffer.GetLength());
        client->buffer.Write(lpByte, dwSize);

        client->buffer.SetOffset(0);
    }

    if (client->stage == STAGE_ESTABLISHED)
    {
        if (client->remainingRequestData > 0)
        {
            // send up to client->remainingRequestData amount of data
            size_t toSend = min(client->remainingRequestData, dwSize);
            int len = send(client->Socket, (const char*)lpByte, (int)toSend, 0);
            if (len != toSend)
            {
                WriteDebug("ERROR_TOO_LITTLE_SENT");
            }
            g_nTotalOut += len;
            client->connectionInfo.dataOut += len;
            client->remainingRequestData -= len;

            if (client->remainingRequestData == 0)
                SET_STAGE(STAGE_REQUEST);

            if (dwSize > toSend)
            {
                client->buffer.SetOffset(client->buffer.GetLength());
                client->buffer.Write(lpByte + toSend, dwSize - toSend);
                client->buffer.SetOffset(0);
            }
        }
        else
            SET_STAGE(STAGE_REQUEST);
    }

    if (client->stage == STAGE_ESTABLISHED && 0)
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

    if (client->stage == STAGE_REQUEST)
    {
        LPBYTE lpBuf = client->buffer.GetBuffer();
        size_t buflen = client->buffer.GetLength();

        // GET / HTTP/1.1\n\n is the minimum request possible
        if (buflen >= 16)
        {
            if (memcmp(lpBuf, "GET /", 5) != 0 &&
                memcmp(lpBuf, "POST /", 6) != 0)
            {
                char _buf[17];
                memcpy(_buf, lpBuf, 16);
                _buf[16] = 0;

                WriteDebug("[AUTH] Invalid first request from %s: %s...", client->szIP, _buf);
                client->Disconnect();
                return;
            }

            BOOL bIsGet = TRUE;
            if (memcmp(lpBuf, "POST /", 6) == 0)
                bIsGet = FALSE;

            // make sure its a valid header and extract the hostname
            char hostname[128];
            if (ValidateAndExtractHost((const char*)lpBuf, buflen, hostname, sizeof(hostname)))
            {
                LPHTTP_PROXY_ROUTE lpRoute = CHttpProxyRoutes::GetInstance()->GetRoute(hostname);
                if (lpRoute == NULL)
                {
                    WriteDebug("No route found for hostname: '%s'\n", hostname);
                    client->Disconnect();
                    return;
                }

                LPHOSTENT lpHost = gethostbyname(lpRoute->Address);

                sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = lpHost == NULL ? inet_addr(lpRoute->Address) : *(u_long*)lpHost->h_addr_list[0];
                addr.sin_port = htons(lpRoute->Port);

                map<string, string> headers;
                size_t header_len = 0;
                ValidateAndExtractRequestHeader((const char*)lpBuf, buflen, headers, &header_len);

                CString strHeader;
                strHeader.AppendEx("%s %s HTTP/1.1\r\n", bIsGet ? "GET" : "POST", headers["_GET_QUERY_"].c_str());
                for (auto it : headers)
                {
                    if (_strcmpi(it.first.c_str(), "_get_query_") == 0)
                        continue;

                    if (_strcmpi(it.first.c_str(), "host") == 0 &&
                        *lpRoute->DestinationHttpHost)
                    {
                        strHeader.AppendEx("Host: %s\r\n", lpRoute->DestinationHttpHost);
                    }
                    else
                        strHeader.AppendEx("%s: %s\r\n", it.first.c_str(), it.second.c_str());
                }
                strHeader += "\r\n";

                //client->buffer.Flush();
                client->buffer.Remove(header_len);
                client->buffer.Insert(0, (LPSTR)(LPCSTR)strHeader, strHeader.GetLength());
                buflen = client->buffer.GetLength();

                map<string, string>::iterator it = headers.find("Content-Length");
                if (it != headers.end())
                {
                    size_t content_length = 0;
                    if (sscanf(it->second.c_str(), "%u", &content_length) > 0)
                    {
                        size_t current_content_length = buflen - strHeader.GetLength();
                        if (current_content_length < content_length)
                            client->remainingRequestData = content_length - current_content_length;
                        else
                            client->remainingRequestData = 0;
                    }
                }

                SET_STAGE(STAGE_CONNECTING);

                //printf( "Sent Request Granted\n" );
                //client->SendRequestError( 0x00, addr.sin_addr.s_addr, addr.sin_port );

                strcpy(client->connectionInfo.szRemoteIp, inet_ntoa(addr.sin_addr));
                client->connectionInfo.wPort = ntohs(addr.sin_port);
                UI_PrintConnections();
                WriteDebug("[Socket: %u] Connecting %s -> %s:%u", client->dpId, client->szIP, client->connectionInfo.szRemoteIp, client->connectionInfo.wPort);
                // todo: add connecting code here for screen

                client->Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                if (connect(client->Socket, (LPSOCKADDR)&addr, sizeof(addr)) == SOCKET_ERROR)
                {
                    WriteDebug("[Socket: %u] FAILED %s -> %s:%u (ERR: %u)", client->dpId, client->szIP, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), WSAGetLastError());
                    //client->SendRequestError( 0x04, addr.sin_addr.s_addr, addr.sin_port );
                    client->SendHtml("Request did not successfully complete, host connection failed.");
                    client->Disconnect();
                    return;
                }

                //client->SendRequestError( 0x00, addr.sin_addr.s_addr, addr.sin_port );
                SET_STAGE(STAGE_ESTABLISHED);

                client->connectionInfo.csState = CST_CONNECTED;
                //UI_PrintConnections();

                // send same header to remote server
                send(client->Socket, (const char*)lpBuf, (int)buflen, 0);
                g_nTotalOut += buflen;
                client->connectionInfo.dataOut += buflen;
                client->buffer.Flush();

                char __buf216[8192];
                memcpy(__buf216, lpBuf, buflen);
                __buf216[buflen] = 0;
                WriteDebug("REQUEST:\r\n%s", __buf216);
            }
        }
    }
}

void CHttpProxyServer::OnSendCompleted(DPID dpId)
{

}

void CHttpProxyServer::Process()
{
    for (map<DPID, HttpProxyClient*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (!it->second->IsDisconnect())
            it->second->Process();
    }
}

HttpProxyClient* CHttpProxyServer::AllocateClient(DPID dpId)
{
    auto client = m_pFreeList;
    if (client)
        m_pFreeList = m_pFreeList->next;
    else
        client = new HttpProxyClient(this);

    client->Init(dpId);
    return client;
}

void CHttpProxyServer::FreeClient(HttpProxyClient* client)
{
    client->Free(); // close remote proxy socket and such

#ifdef _DEBUG
    client->Init(DPID_UNKNOWN); // Re-initialize all variables for debugging
#endif // _DEBUG

    client->next = m_pFreeList;
    m_pFreeList = client;
}