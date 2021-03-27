#pragma once

#if 1 // _DEBUG_MESSAGES
inline void WriteDebug(const char* format, ...)
{
    char text[8192];
    char* ptr = text;

    va_list l;
    va_start(l, format);

    int len = _vscprintf(format, l);
    if (len + 1 >= sizeof(text))
        ptr = (char*)malloc(len + 1);

    vsprintf(ptr, format, l);

    va_end(l);

    SYSTEMTIME st;
    GetLocalTime(&st);
    //printf( "[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, text );
    OUTPUTDEBUGSTRING("[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, text);

    HANDLE hFile = CreateFile("debug.txt", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        SetFilePointer(hFile, 0, 0, FILE_END);

        DWORD dw;
        char buf[16384];
        sprintf(buf, "[%02d:%02d:%02d]\r\n%s\r\n\r\n", st.wHour, st.wMinute, st.wSecond, text);
        WriteFile(hFile, buf, strlen(buf), &dw, NULL);

        CloseHandle(hFile);
    }

    if (ptr != text)
        free(ptr);
}
#else // _DEBUG_MESSAGES
#define WriteDebug( msg, ... ) ((void)0)
#endif // _DEBUG_MESSAGES