#pragma once

typedef struct _DATA_BUF
{
    int nSize;
    LPBYTE lpBuf;

    _DATA_BUF* prev;
    _DATA_BUF* next;

} DATA_BUF, *LPDATA_BUF;

class Buffer
{
public:
    Buffer();
    virtual ~Buffer();
    
    LPBYTE Allocate(int nSize);
    std::map<DWORD_PTR, LPDATA_BUF>::iterator Free(LPBYTE lpBuf);
    void Clear();

private:
    void AssignBuffer(LPDATA_BUF lpBuf);

    LPDATA_BUF m_pHead;
    LPDATA_BUF m_pTail;
    LPDATA_BUF m_pExtHead;
    LPDATA_BUF m_pExtTail;
    std::map<DWORD_PTR, LPDATA_BUF> m_mapAlloc;
};