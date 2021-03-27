#pragma once

typedef struct _tagHTTP_PROXY_ROUTE
{
    char Address[128];
    int Port;
    char DestinationHttpHost[128];

} HTTP_PROXY_ROUTE, * LPHTTP_PROXY_ROUTE;

class CHttpProxyRoutes
{
public:
    CHttpProxyRoutes();
    virtual ~CHttpProxyRoutes();

    void AddRoute(const char* hostname, const char* destination_address, int destination_port, const char* destination_http_host = NULL);
    LPHTTP_PROXY_ROUTE GetRoute(const char* hostname);
    BOOL LoadRoutes(const char* filename);
    void Clear();

    static CHttpProxyRoutes* GetInstance()
    {
        static CHttpProxyRoutes instance;
        return &instance;
    }

private:
    map<string, LPHTTP_PROXY_ROUTE> m_routes;
};

inline LPHTTP_PROXY_ROUTE CHttpProxyRoutes::GetRoute(const char* hostname)
{
    char temp[128];
    strcpy(temp, hostname);
    _strlwr(temp);

    map<string, LPHTTP_PROXY_ROUTE>::iterator it = m_routes.find(temp);
    return it != m_routes.end() ? it->second : NULL;
}