#pragma once

#include "../Shared/NetworkDefines.h"

class CHttpProxyServer;
struct HttpProxyClient
{
    CHttpProxyServer* pServer;
    BOOL bQueryDisconnect;
    DPID dpId;
    char szIP[ 16 ];
    DWORD dwIP;
    // Socket that is connected to the target host
    SOCKET Socket;
    CAr buffer;
    AuthenticationStage stage;
    fd_set fd;
    TIMEVAL tv;
    ConnectionInfo connectionInfo;

    // http
    size_t remainingRequestData;

    HttpProxyClient* next;

    HttpProxyClient( CHttpProxyServer* pServer ) :
        pServer( pServer ),
        next( NULL ),
        Socket( INVALID_SOCKET )
    {
        tv.tv_sec = 0;
        tv.tv_usec = 1;
        Init( DPID_UNKNOWN );
    }

    ~HttpProxyClient()
    {
        Free();
    }

    void Init( DPID dpId )
    {
        this->dpId = dpId;
        dwIP = 0;
        *szIP = 0;
        bQueryDisconnect = FALSE;
        stage = STAGE_REQUEST;
        SAFE_CLOSE_SOCKET( Socket );
        buffer.Flush();
        ZeroMemory( &connectionInfo, sizeof( connectionInfo ) );
        remainingRequestData = 0;
    }

    void Free()
    {
        SAFE_CLOSE_SOCKET( Socket );
    }

    void Disconnect();
    inline BOOL IsDisconnect() { return bQueryDisconnect; }

    void SendHtml( const char* text );
    void Process();
};

class CHttpProxyServer : public CIocpServer
{
public:
    CHttpProxyServer( );
    virtual ~CHttpProxyServer( );
    void OnClientConnected( DPID dpId );
    void OnClientDisconnected( DPID dpId );
    void OnClientDataReceived( DPID dpId, LPBYTE lpByte, DWORD dwSize );
    void OnSendCompleted( DPID dpId );
    void Process();

    inline map<DPID, HttpProxyClient*>& GetClients( ) { return m_clients; }

private:
    HttpProxyClient* AllocateClient( DPID dpId );
    void FreeClient( HttpProxyClient* client );

    HttpProxyClient* m_pFreeList;
    map<DPID, HttpProxyClient*> m_clients;
};