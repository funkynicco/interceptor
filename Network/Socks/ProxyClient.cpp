#include "StdAfx.h"
#include "ProxyServer.h"

void ProxyClient::Disconnect()
{
    if( bQueryDisconnect )
        return;

    bQueryDisconnect = TRUE;
    pServer->Disconnect( dpId );
}

void ProxyClient::SendRequestError( BYTE byError, DWORD dwAddress, WORD wPort )
{
    CAr ar;
    ar << (BYTE)0x05; // protocol version
    ar << byError; // status
    ar << (BYTE)0x00; // reserved

    ar << (BYTE)0x01; // ipv4 type
    ar << dwAddress;
    ar << wPort;

    pServer->Send( dpId, ar.GetBuffer(), ar.GetLength() );
}

void ProxyClient::Process()
{
    static char tempBuffer[ 32768 ];

    if( stage == STAGE_ESTABLISHED &&
        Socket != INVALID_SOCKET )
    {
        FD_ZERO( &fd );
        FD_SET( Socket, &fd );
        if( select( 0, &fd, NULL, NULL, &tv ) > 0 &&
            FD_ISSET( Socket, &fd ) )
        {
            int len = recv( Socket, tempBuffer, sizeof( tempBuffer ), 0 );
            if( len > 0 )
            {
                g_nTotalIn += len;
                connectionInfo.dataIn += len;
                pServer->Send( dpId, (LPBYTE)tempBuffer, len );
                //printf( "[S->C] %u bytes\n", len );
            }
            else
            {
                WriteDebug( "Server closed connection on %s", szIP );
                Disconnect();
            }
        }
    }
}