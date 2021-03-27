#pragma once

#include <NanoFramework.h>

#include <map>
#include <list>
#include <unordered_set>
using namespace std;

//#define SAFE_CLOSE_SOCKET( a )		if( a != INVALID_SOCKET ) { shutdown( a, SD_BOTH ); closesocket( a ); (a) = INVALID_SOCKET; }
#define SAFE_FREE( a )				if( a ) { free( a ); (a) = NULL; }
#define SAFE_DELETE( a )			if( a ) { delete (a); (a) = NULL; }
#define SAFE_DELETE_ARRAY( a )		if( a ) { delete[] (a); (a) = NULL; }

#include "Utilities.inl"

// Interceptor

extern unsigned __int64 g_nTotalIn;
extern unsigned __int64 g_nTotalOut;
void UI_PrintConnections();

#include "Buffer.h"
#include "Network\Socks\ProxyServer.h"
#include "Network\HttpProxy\Routes\HttpProxyRoutes.h"
#include "Network\HttpProxy\HttpProxyServer.h"