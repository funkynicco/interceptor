#include "StdAfx.h"

#include "Network\Shared\HostnameExtractor.inl"

#include "Buffer.h"
#include "Network\Socks\ProxyServer.h"
#include "Network\HttpProxy\Routes\HttpProxyRoutes.h"
#include "Network\HttpProxy\HttpProxyServer.h"

unsigned __int64 g_nTotalIn = 0;
unsigned __int64 g_nTotalOut = 0;

const int NumberOfConnectionsToShow = 17;

static CProxyServer* g_pProxyServer = nullptr;
static CHttpProxyServer* g_pHttpProxyServer = nullptr;
static BOOL g_bIsHttp = TRUE;

static HANDLE hConsole = nullptr;
inline void SCP(int x, int y)
{
    COORD pos = { x, y };
    SetConsoleCursorPosition(hConsole, pos);
}

inline void RenderFrame()
{
    system("cls");

    // row 1
    SCP(1, 1);
    printf("\xc9");
    for (int i = 0; i < 73; ++i)
        printf("\xcd");
    printf("\xbb");

    // row 2
    SCP(1, 2);
    printf("\xba");
    SCP(75, 2);
    printf("\xba");
    SCP(1, 3);
    printf("\xcc");

    for (int i = 0; i < 73; ++i)
    {
        switch (i)
        {
        case 17:
        case 35:
        case 43:
        case 49:
        case 61:
            printf("\xcb");
            break;
        default:
            printf("\xcd");
            break;
        }
    }
    printf("\xb9");

    SCP(8, 2);
    printf("Local");
    SCP(26, 2);
    printf("Remote");
    SCP(40, 2);
    printf("Port");
    SCP(46, 2);
    printf("State");
    SCP(56, 2);
    printf("In");
    SCP(68, 2);
    printf("Out");

    int y = 3;
    for (int i = 0; i < NumberOfConnectionsToShow; ++i)
    {
        SCP(1, ++y);
        //printf( "\xba 255.255.255.255 \xba 255.255.255.255 \xba 65535 \xba CON \xba 999.99 KB \xba 999.99 KB \xba" );
        printf("\xba                 \xba                 \xba       \xba     \xba           \xba           \xba");
    }

    SCP(1, ++y);
    printf("\xcc");
    for (int i = 0; i < 73; ++i)
    {
        switch (i)
        {
        case 17:
        case 35:
        case 43:
            printf("\xca");
            break;
        case 49:
        case 61:
            printf("\xce"); // 4 joint thing
            break;
        default:
            printf("\xcd");
            break;
        }
    }
    printf("\xb9");

    SCP(1, ++y);
    printf("\xba 6/13 active connections");

    //SCP( 19, y ); printf( "\xba" );
    //SCP( 37, y ); printf( "\xba" );
    //SCP( 45, y ); printf( "\xba" );

    SCP(51, y);
    printf("\xba 999.99 KB \xba 999.99 KB \xba");

    SCP(1, ++y);
    printf("\xc8");
    for (int i = 0; i < 73; ++i)
    {
        switch (i)
        {
            /*case 17:
            case 35:
            case 43:*/
        case 49:
        case 61:
            printf("\xca");
            break;
        default:
            printf("\xcd");
            break;
        }
    }
    printf("\xbc");
}

template <typename _TyServer, typename _TyClient>
inline void PrintConnections(_TyServer* pServer)
{
    static char temp[256];

    map<DPID, _TyClient*>& clients = pServer->GetClients();
    map<DPID, _TyClient*>::iterator it = clients.begin();

    int y = 3;
    int active = 0;

    for (int i = 0; i < NumberOfConnectionsToShow; ++i)
    {
        ConnectionInfo* con = nullptr;//i < connections.size() ? &connections[ i ] : nullptr;

        if (it != clients.end())
        {
            if (i > 0)
                ++it;

            if (it != clients.end())
                con = &it->second->connectionInfo;
        }

        if (con)
        {
            if (con->csState == CST_CONNECTED)
                ++active;

            WORD wAttributes = 0;
            switch (con->csState)
            {
            case CST_DISCONNECTED:
                wAttributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
                break;
            case CST_CONNECTED:
                wAttributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case CST_ESTABLISHING:
                wAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            }
            SetConsoleTextAttribute(hConsole, wAttributes);
        }

        ++y;
        SCP(3, y);
        printf("%-15s", con ? con->szLocalIp : "");
        SCP(21, y);
        printf("%-15s", con ? con->szRemoteIp : "");

        SCP(39, y);
        if (con && con->wPort > 0)
            printf("%5u", con->wPort);
        else
            printf("     ");

        // console attribute color
        SCP(47, y);
        printf("%s", con ? (con->csState == CST_CONNECTED ? "CON" : (con->csState == CST_ESTABLISHING ? "EST" : "DIS")) : "   ");

        *temp = 0;

        SCP(52, y);
        if (con)
            GetSize(con->dataIn, temp, sizeof(temp));
        printf("%10s", temp);

        SCP(64, y);
        if (con)
            GetSize(con->dataOut, temp, sizeof(temp));
        printf("%10s", temp);
    }

    // print status at bottom
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    SCP(3, 22);
    //sprintf( temp, "%d/%u active connection%s", active, clients.size(), active == 1 ? "" : "s" );
    sprintf(temp, "Showing %d/%u active connection%s", active, clients.size(), active == 1 ? "" : "s");
    printf("%-39s", temp);

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    SCP(52, 22);
    GetSize(g_nTotalIn, temp, sizeof(temp));
    printf("%10s", temp);

    SCP(64, 22);
    GetSize(g_nTotalOut, temp, sizeof(temp));
    printf("%10s", temp);

    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
}

inline void UI_PrintConnections()
{
    if (g_bIsHttp)
        PrintConnections<CHttpProxyServer, HttpProxyClient>(g_pHttpProxyServer);
    else
        PrintConnections<CProxyServer, ProxyClient>(g_pProxyServer);
}

template <typename _TyServer>
inline void RunServer(_TyServer* pServer, int port)
{
    if (pServer->Start(port))
    {
        WriteDebug("Server started.");
        WriteDebug("Press [ESC] to stop server.");

        RenderFrame();
        UI_PrintConnections();

        time_t nextCalculateSpeed = 0;
        char szSize1[64];
        char szSize2[64];

        while (1)
        {
            if (_kbhit() &&
                _getch() == VK_ESCAPE)
                break;

            pServer->ReceiveMessages();
            pServer->Process();

            time_t now = time(nullptr);
            if (now >= nextCalculateSpeed)
            {
                static unsigned __int64 oldIn = 0;
                static unsigned __int64 oldOut = 0;

                GetSize(g_nTotalIn - oldIn, szSize1, sizeof(szSize1));
                GetSize(g_nTotalOut - oldOut, szSize2, sizeof(szSize2));

                oldIn = g_nTotalIn;
                oldOut = g_nTotalOut;

                //                sprintf( temp, "In:%s Out:%s", szSize1, szSize2 );
                //SetConsoleTitle( temp );
                UI_PrintConnections();

                nextCalculateSpeed = now + 1;
            }

            Sleep(10);
        }

        pServer->Stop();
    }
    else
        WriteDebug("Failed to start server, check if port is in use.");
}

int main(int argc, char* argv[])
{
#error must initialize nativelib here...

#if 0
    const char* request_html = "GET /ca/lol?test=keke HTTP/1.1\r\nHost: lol.com\r\nContent-Length: blah\r\n\r\n";

    char hostname[128];
    if (ValidateAndExtractHost(request_html, strlen(request_html), hostname, sizeof(hostname)))
        printf("hostname: '%s'\n", hostname);
    else
        printf("Failed to validate request\n");

    Pause();
    return 0;
#endif

    DeleteFile("debug.txt");

    //CHttpProxyRoutes::GetInstance()->AddRoute( "meteorflyff.com", "192.168.1.64", 80 );
    if (!CHttpProxyRoutes::GetInstance()->LoadRoutes("routes.cfg"))
        MessageBox(nullptr, "Failed to load routes.cfg", "Routes", MB_OK | MB_ICONWARNING);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hConsole, &cci);
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cci);

    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);
    SetConsoleTitle("Interceptor Firephoenix");

    int port = 80;
    if (argc > 1)
    {
        if (sscanf(argv[1], "%d", &port) == 0)
            return 1;
    }
    for (int i = 1; i < argc; ++i)
    {
        if (_strcmpi(argv[i], "--socks") == 0)
        {
            g_bIsHttp = FALSE;
        }
        else if (_strcmpi(argv[i], "--http") == 0)
        {
            g_bIsHttp = TRUE;
        }
        else
        {
            int _num = 0;
            if (sscanf(argv[i], "%d", &_num) != 0)
                port = _num;
        }
    }

    WriteDebug("Using port: %d", port);
    WriteDebug("Mode: %s", g_bIsHttp ? "Http" : "Socks");

    CProxyServer server;
    CHttpProxyServer httpServer;
    g_pProxyServer = &server;
    g_pHttpProxyServer = &httpServer;

    if (g_bIsHttp)
        RunServer<CHttpProxyServer>(g_pHttpProxyServer, port);
    else
        RunServer<CProxyServer>(g_pProxyServer, port);

    WSACleanup();
    return 0;
}