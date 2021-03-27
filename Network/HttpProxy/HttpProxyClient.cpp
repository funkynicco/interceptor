#include "StdAfx.h"
#include "HttpProxyServer.h"

void HttpProxyClient::Disconnect( )
{
    if( bQueryDisconnect )
        return;

    bQueryDisconnect = TRUE;
    pServer->Disconnect( dpId );
}

void HttpProxyClient::SendHtml( const char* text )
{
    static char buffer[ 32768 ];

    size_t text_len = strlen( text );
    size_t len = sprintf( buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %u\r\n\r\n", text_len );

    memcpy( buffer + len, text, text_len );
    len += text_len;
    *( buffer + len ) = 0;
    
    pServer->Send( dpId, (LPBYTE)buffer, len );
}

void HttpProxyClient::Process( )
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