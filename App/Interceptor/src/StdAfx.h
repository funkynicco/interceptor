#pragma once

#include <Windows.h>

#include <map>
#include <list>
#include <unordered_set>

//#define SAFE_CLOSE_SOCKET(a)	if(a != INVALID_SOCKET) { shutdown(a, SD_BOTH); closesocket(a); (a) = INVALID_SOCKET; }
#define SAFE_FREE(a)			if(a) { free(a); (a) = nullptr; }
#define SAFE_DELETE(a)			if(a) { delete (a); (a) = nullptr; }
#define SAFE_DELETE_ARRAY(a)	if(a) { delete[] (a); (a) = nullptr; }

#include "Utilities.h"

// Interceptor

extern unsigned __int64 g_nTotalIn;
extern unsigned __int64 g_nTotalOut;
void UI_PrintConnections();