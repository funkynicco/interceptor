#pragma once

#include "../Shared/NetworkDefines.h"

enum SocksAuthentication : BYTE
{
    SOCKSAUTH_NONE = 0x00,
    SOCKSAUTH_GSSAPI = 0x01,
    SOCKSAUTH_LOGIN = 0x02
};

class CProxyServer;
struct ProxyClient
{
    CProxyServer* pServer;
    BOOL bQueryDisconnect;
    DPID dpId;
    char szIP[16];
    DWORD dwIP;
    // Socket that is connected to the target host
    SOCKET Socket;
    CAr buffer;
    AuthenticationStage stage;
    fd_set fd;
    TIMEVAL tv;
    ConnectionInfo connectionInfo;

    ProxyClient* next;

    ProxyClient(CProxyServer* pServer) :
        pServer(pServer),
        next(NULL),
        Socket(INVALID_SOCKET)
    {
        tv.tv_sec = 0;
        tv.tv_usec = 1;
        Init(DPID_UNKNOWN);
    }

    ~ProxyClient()
    {
        Free();
    }

    void Init(DPID dpId)
    {
        this->dpId = dpId;
        dwIP = 0;
        *szIP = 0;
        bQueryDisconnect = FALSE;
        stage = STAGE_INITIAL;
        SAFE_CLOSE_SOCKET(Socket);
        buffer.Flush();
        ZeroMemory(&connectionInfo, sizeof(connectionInfo));
    }

    void Free()
    {
        SAFE_CLOSE_SOCKET(Socket);
    }

    void Disconnect();
    inline BOOL IsDisconnect() { return bQueryDisconnect; }

    void SendRequestError(BYTE byError, DWORD dwAddress, WORD wPort);
    void Process();
};

class CProxyServer : public CIocpServer
{
public:
    CProxyServer();
    virtual ~CProxyServer();
    void OnClientConnected(DPID dpId);
    void OnClientDisconnected(DPID dpId);
    void OnClientDataReceived(DPID dpId, LPBYTE lpByte, DWORD dwSize);
    void OnSendCompleted(DPID dpId);
    void Process();

    inline map<DPID, ProxyClient*>& GetClients() { return m_clients; }

private:
    ProxyClient* AllocateClient(DPID dpId);
    void FreeClient(ProxyClient* client);

    ProxyClient* m_pFreeList;
    map<DPID, ProxyClient*> m_clients;
};