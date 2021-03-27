#pragma once

typedef struct _DATA_BUF
{
    int nSize;
    LPBYTE lpBuf;

    _DATA_BUF* prev;
    _DATA_BUF* next;

} DATA_BUF, *LPDATA_BUF;

class CBuffer
{
public:
    CBuffer();
    virtual ~CBuffer();
    LPBYTE	Allocate(int nSize);
    map<DWORD_PTR, LPDATA_BUF>::iterator Free(LPBYTE lpBuf);
    void	Clear();

private:
    void AssignBuffer(LPDATA_BUF lpBuf);
    LPDATA_BUF m_pHead, m_pTail;
    LPDATA_BUF m_pExtHead, m_pExtTail;
    map<DWORD_PTR, LPDATA_BUF> m_mapAlloc;
};