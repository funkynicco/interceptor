#include "StdAfx.h"
#include "HttpProxyRoutes.h"

CHttpProxyRoutes::CHttpProxyRoutes()
{

}

CHttpProxyRoutes::~CHttpProxyRoutes()
{
    Clear();
}

void CHttpProxyRoutes::AddRoute( const char* hostname, const char* destination_address, int destination_port, const char* destination_http_host )
{
    char temp[ 128 ];
    strcpy( temp, hostname );
    _strlwr( temp );

    map<string, LPHTTP_PROXY_ROUTE>::iterator it = m_routes.find( temp );
    if( it != m_routes.end() )
    {
        WriteDebug( __FUNCTION__ " - Route '%s' already exists, pointing to %s:%d", hostname, destination_address, destination_port );
        return;
    }

    LPHTTP_PROXY_ROUTE lpRoute = new HTTP_PROXY_ROUTE;
    strcpy( lpRoute->Address, destination_address );
    lpRoute->Port = destination_port;

    if( destination_http_host )
        strcpy( lpRoute->DestinationHttpHost, destination_http_host );
    else
        *lpRoute->DestinationHttpHost = 0;

    m_routes.insert( pair<string, LPHTTP_PROXY_ROUTE>( temp, lpRoute ) );
}

BOOL CHttpProxyRoutes::LoadRoutes( const char* filename )
{
    CScanner scanner;
    if( !scanner.Load( filename ) )
        return FALSE;

    Clear();

    scanner.GetToken();
    while( scanner.tok != FINISHED )
    {
        if( _strcmpi( scanner.token, "Routes" ) == 0 )
        {
            scanner.GetToken();
            if( scanner.tok != FINISHED &&
                *scanner.token == '{' )
            {
                scanner.GetToken();
                while( scanner.tok != FINISHED &&
                    *scanner.token != '}' )
                {
                    char hostname[ 128 ] = { 0 };
                    char destination[ 128 ] = { 0 };
                    int port = 0;
                    char destinationHttpHost[ 128 ] = { 0 };
                    
                    strcpy( hostname, scanner.token );
                    scanner.GetToken();
                    strcpy( destination, scanner.token );
                    port = scanner.GetNumber();

                    scanner.GetToken();
                    strcpy( destinationHttpHost, scanner.token );

                    AddRoute( hostname, destination, port, destinationHttpHost );

                    scanner.GetToken();
                }
            }
        }

        scanner.GetToken();
    }

    return TRUE;
}

void CHttpProxyRoutes::Clear( )
{
    for( map<string, LPHTTP_PROXY_ROUTE>::iterator it = m_routes.begin(); it != m_routes.end(); ++it )
    {
        delete it->second;
    }

    m_routes.clear();
}