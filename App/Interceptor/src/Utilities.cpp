#include "StdAfx.h"

#define __ENABLE_WRITEDEBUG

void WriteDebug(const char* format, ...)
{
#ifdef __ENABLE_WRITEDEBUG
    char text[8192];
    char* ptr = text;

    va_list l;
    va_start(l, format);

    int len = _vscprintf(format, l);
    if (len + 1 >= sizeof(text))
        ptr = (char*)malloc(len + 1);

    vsprintf(ptr, format, l);

    va_end(l);
    
    char buf[16384];

    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf_s(buf, "[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, text);
    OutputDebugStringA(buf);

    HANDLE hFile = CreateFile(L"debug.txt", GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hFile, 0, 0, FILE_END);

        DWORD dw;
        sprintf_s(buf, "[%02d:%02d:%02d]\r\n%s\r\n\r\n", st.wHour, st.wMinute, st.wSecond, text);
        WriteFile(hFile, buf, (DWORD)strlen(buf), &dw, nullptr);

        CloseHandle(hFile);
    }

    if (ptr != text)
        free(ptr);
#endif
}