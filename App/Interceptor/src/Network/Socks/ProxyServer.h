#pragma once

#include <NativeLib/Network/AsynchronousTcpServer.h>
#include <NativeLib/IO/MemoryStream.h>

#include "../Shared/NetworkDefines.h"

enum SocksAuthentication : BYTE
{
    SOCKSAUTH_NONE = 0x00,
    SOCKSAUTH_GSSAPI = 0x01,
    SOCKSAUTH_LOGIN = 0x02
};

struct ProxyClient
{
    class ProxyServer* pServer;
    BOOL bQueryDisconnect;
    nl::network::DPID dpId;
    char szIP[16];
    DWORD dwIP;
    // Socket that is connected to the target host
    SOCKET Socket;
    nl::io::MemoryStream buffer;
    AuthenticationStage stage;
    fd_set fd;
    TIMEVAL tv;
    ConnectionInfo connectionInfo;

    ProxyClient* next;

    ProxyClient(class ProxyServer* pServer) :
        pServer(pServer),
        next(nullptr),
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

    void Init(nl::network::DPID dpId)
    {
        this->dpId = dpId;
        dwIP = 0;
        *szIP = 0;
        bQueryDisconnect = FALSE;
        stage = STAGE_INITIAL;
        if (Socket != INVALID_SOCKET)
        {
            closesocket(Socket);
            Socket = INVALID_SOCKET;
        }
        buffer.Flush();
        ZeroMemory(&connectionInfo, sizeof(connectionInfo));
    }

    void Free()
    {
        if (Socket != INVALID_SOCKET)
        {
            closesocket(Socket);
            Socket = INVALID_SOCKET;
        }
    }

    void Disconnect();
    inline BOOL IsDisconnect() const { return bQueryDisconnect; }

    void SendRequestError(BYTE byError, DWORD dwAddress, WORD wPort);
    void Process();
};

class ProxyServer : public nl::network::AsynchronousTcpServer
{
public:
    ProxyServer();
    virtual ~ProxyServer();
    void OnClientConnected(nl::network::DPID dpId);
    void OnClientDisconnected(nl::network::DPID dpId);
    void OnClientDataReceived(nl::network::DPID dpId, LPBYTE lpByte, DWORD dwSize);
    void OnSendCompleted(nl::network::DPID dpId);
    void Process();

    inline std::map<nl::network::DPID, ProxyClient*>& GetClients() { return m_clients; }

private:
    ProxyClient* AllocateClient(nl::network::DPID dpId);
    void FreeClient(ProxyClient* client);

    ProxyClient* m_pFreeList;
    std::map<nl::network::DPID, ProxyClient*> m_clients;
};