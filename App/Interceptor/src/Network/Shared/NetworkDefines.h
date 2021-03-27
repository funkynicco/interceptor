#pragma once

enum AuthenticationStage
{
    STAGE_INITIAL,
    STAGE_LOGIN,
    STAGE_REQUEST,
    STAGE_CONNECTING,
    STAGE_ESTABLISHED
};

enum ConnectionState
{
    CST_DISCONNECTED,
    CST_CONNECTED,
    CST_ESTABLISHING
};

struct ConnectionInfo
{
    char szLocalIp[16];
    char szRemoteIp[16];
    WORD wPort;
    ConnectionState csState;
    unsigned __int64 dataIn;
    unsigned __int64 dataOut;
};