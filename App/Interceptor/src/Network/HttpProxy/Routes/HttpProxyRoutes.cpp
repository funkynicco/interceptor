#include "StdAfx.h"
#include "HttpProxyRoutes.h"

#include <NativeLib/Parsing/Scanner.h>

CHttpProxyRoutes::CHttpProxyRoutes()
{

}

CHttpProxyRoutes::~CHttpProxyRoutes()
{
    Clear();
}

void CHttpProxyRoutes::AddRoute(const char* hostname, const char* destination_address, int destination_port, const char* destination_http_host)
{
    char temp[128];
    strcpy(temp, hostname);
    _strlwr(temp);

    map<string, LPHTTP_PROXY_ROUTE>::iterator it = m_routes.find(temp);
    if (it != m_routes.end())
    {
        WriteDebug(__FUNCTION__ " - Route '%s' already exists, pointing to %s:%d", hostname, destination_address, destination_port);
        return;
    }

    LPHTTP_PROXY_ROUTE lpRoute = new HTTP_PROXY_ROUTE;
    strcpy(lpRoute->Address, destination_address);
    lpRoute->Port = destination_port;

    if (destination_http_host)
        strcpy(lpRoute->DestinationHttpHost, destination_http_host);
    else
        *lpRoute->DestinationHttpHost = 0;

    m_routes.insert(pair<string, LPHTTP_PROXY_ROUTE>(temp, lpRoute));
}

BOOL CHttpProxyRoutes::LoadRoutes(const char* filename)
{
    nl::parsing::Scanner scanner;
    try
    {
        scanner = nl::parsing::Scanner::FromFile(filename);
    }
    catch (Exception)
    {
        return FALSE;
    }

    Clear();

    auto token = scanner.Next();
    while (token)
    {
        if (token == "Routes")
        {
            token = scanner.Next();
            if (token &&
                token == '{')
            {
                token = scanner.Next();
                while (
                    token &&
                    token != '}')
                {
                    char hostname[128] = { 0 };
                    char destination[128] = { 0 };
                    int port = 0;
                    char destinationHttpHost[128] = { 0 };

                    StringViewToCharArray(hostname, token);
                    StringViewToCharArray(destination, scanner.Next());

                    scanner
                        .Next()
                        .GetToken(&port);

                    StringViewToCharArray(destinationHttpHost, scanner.Next());

                    AddRoute(hostname, destination, port, destinationHttpHost);

                    token = scanner.Next();
                }
            }
        }

        token = scanner.Next();
    }

    return TRUE;
}

void CHttpProxyRoutes::Clear()
{
    for (map<string, LPHTTP_PROXY_ROUTE>::iterator it = m_routes.begin(); it != m_routes.end(); ++it)
    {
        delete it->second;
    }

    m_routes.clear();
}