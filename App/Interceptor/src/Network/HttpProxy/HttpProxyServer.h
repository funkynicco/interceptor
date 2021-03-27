#pragma once

#include <NativeLib/Network/AsynchronousTcpServer.h>

#include "../Shared/NetworkDefines.h"

struct HttpProxyClient
{
    class HttpProxyServer* pServer;
    BOOL bQueryDisconnect;
    nl::network::DPID dpId;
    char szIP[16];
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

    HttpProxyClient(class HttpProxyServer* pServer) :
        pServer(pServer),
        next(nullptr),
        Socket(INVALID_SOCKET)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 1;
        Init(DPID_UNKNOWN);
    }

    ~HttpProxyClient()
    {
        Free();
    }

    void Init(nl::network::DPID dpId)
    {
        this->dpId = dpId;
        dwIP = 0;
        *szIP = 0;
        bQueryDisconnect = FALSE;
        stage = STAGE_REQUEST;
        SAFE_CLOSE_SOCKET(Socket);
        buffer.Flush();
        ZeroMemory(&connectionInfo, sizeof(connectionInfo));
        remainingRequestData = 0;
    }

    void Free()
    {
        SAFE_CLOSE_SOCKET(Socket);
    }

    void Disconnect();
    inline BOOL IsDisconnect() { return bQueryDisconnect; }

    void SendHtml(const char* text);
    void Process();
};

class HttpProxyServer : public nl::network::AsynchronousTcpServer
{
public:
    HttpProxyServer();
    virtual ~HttpProxyServer();
    void OnClientConnected(nl::network::DPID dpId);
    void OnClientDisconnected(nl::network::DPID dpId);
    void OnClientDataReceived(nl::network::DPID dpId, LPBYTE lpByte, DWORD dwSize);
    void OnSendCompleted(nl::network::DPID dpId);
    void Process();

    inline std::map<nl::network::DPID, HttpProxyClient*>& GetClients() { return m_clients; }

private:
    HttpProxyClient* AllocateClient(nl::network::DPID dpId);
    void FreeClient(HttpProxyClient* client);

    HttpProxyClient* m_pFreeList;
    std::map<nl::network::DPID, HttpProxyClient*> m_clients;
};