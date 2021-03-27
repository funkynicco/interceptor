#include "StdAfx.h"
#include "ProxyServer.h"

void ProxyClient::Disconnect()
{
    if (bQueryDisconnect)
        return;

    bQueryDisconnect = TRUE;
    pServer->Disconnect(dpId);
}

void ProxyClient::SendRequestError(BYTE byError, DWORD dwAddress, WORD wPort)
{
    char buffer[10];
    buffer[0] = 0x05; // protocol version
    buffer[1] = byError; // status
    buffer[2] = 0x00; // reserved
    
    buffer[3] = 0x01; // ipv4 type

    // dwAddress
    buffer[4] = (dwAddress >> 24) & 0xff;
    buffer[5] = (dwAddress >> 16) & 0xff;
    buffer[6] = (dwAddress >> 8) & 0xff;
    buffer[7] = dwAddress & 0xff;

    // wPort
    buffer[8] = (wPort >> 8) & 0xff;
    buffer[9] = wPort & 0xff;

    pServer->Send(dpId, buffer, sizeof(buffer));
}

void ProxyClient::Process()
{
    static char tempBuffer[32768];

    if (stage == STAGE_ESTABLISHED &&
        Socket != INVALID_SOCKET)
    {
        FD_ZERO(&fd);
        FD_SET(Socket, &fd);
        if (select(0, &fd, NULL, NULL, &tv) > 0 &&
            FD_ISSET(Socket, &fd))
        {
            int len = recv(Socket, tempBuffer, sizeof(tempBuffer), 0);
            if (len > 0)
            {
                g_nTotalIn += len;
                connectionInfo.dataIn += len;
                pServer->Send(dpId, (LPBYTE)tempBuffer, len);
                //printf( "[S->C] %u bytes\n", len );
            }
            else
            {
                WriteDebug("Server closed connection on %s", szIP);
                Disconnect();
            }
        }
    }
}