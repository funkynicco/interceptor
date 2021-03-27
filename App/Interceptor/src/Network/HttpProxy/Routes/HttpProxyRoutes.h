#pragma once

typedef struct _tagHTTP_PROXY_ROUTE
{
    char Address[128];
    int Port;
    char DestinationHttpHost[128];

} HTTP_PROXY_ROUTE, * LPHTTP_PROXY_ROUTE;

class HttpProxyRoutes
{
public:
    HttpProxyRoutes();
    virtual ~HttpProxyRoutes();

    void AddRoute(const char* hostname, const char* destination_address, int destination_port, const char* destination_http_host = nullptr);
    LPHTTP_PROXY_ROUTE GetRoute(const char* hostname);
    BOOL LoadRoutes(const char* filename);
    void Clear();

    static HttpProxyRoutes* GetInstance()
    {
        static HttpProxyRoutes instance;
        return &instance;
    }

private:
    std::map<std::string, LPHTTP_PROXY_ROUTE> m_routes;
};

inline LPHTTP_PROXY_ROUTE HttpProxyRoutes::GetRoute(const char* hostname)
{
    char temp[128];
    strcpy(temp, hostname);
    _strlwr(temp);

    std::map<std::string, LPHTTP_PROXY_ROUTE>::iterator it = m_routes.find(temp);
    return it != m_routes.end() ? it->second : nullptr;
}