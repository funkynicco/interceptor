#include "StdAfx.h"
#include "Buffer.h"

#define MIN_BUFSIZE 1024

CBuffer::CBuffer() :
    m_pHead(NULL),
    m_pTail(NULL),
    m_pExtHead(NULL),
    m_pExtTail(NULL)
{
}

CBuffer::~CBuffer()
{
    LPDATA_BUF ptr = m_pHead;
    LPDATA_BUF ptrNext;
    while (ptr)
    {
        ptrNext = ptr->next;
        free(ptr->lpBuf);
        delete ptr;
        ptr = ptrNext;
    }

    ptr = m_pExtHead;
    while (ptr)
    {
        ptrNext = ptr->next;
        free(ptr->lpBuf);
        delete ptr;
        ptr = ptrNext;
    }

    for (map<DWORD, LPDATA_BUF>::iterator it = m_mapAlloc.begin(); it != m_mapAlloc.end(); ++it)
    {
        free(it->second->lpBuf);
        delete it->second;
    }
}

LPBYTE CBuffer::Allocate(int nSize)
{
    LPDATA_BUF ptr = nSize > MIN_BUFSIZE ? m_pExtHead : m_pHead;

    while (ptr)
    {
        if (ptr->nSize >= nSize)
        {
            AssignBuffer(ptr);
            break;
        }
        ptr = ptr->next;
    }

    if (ptr == NULL)
    {
        nSize = nSize < MIN_BUFSIZE ? MIN_BUFSIZE : GetPrimeNumber(nSize);
        LPBYTE lpBuf = (LPBYTE)malloc(nSize);
        if (lpBuf == NULL)
            return NULL;

        ptr = new DATA_BUF;
        ptr->nSize = nSize;
        ptr->lpBuf = lpBuf;
        ptr->prev = NULL;
        ptr->next = NULL;
        m_mapAlloc.insert(pair<DWORD, LPDATA_BUF>((DWORD)lpBuf, ptr));
    }

    return ptr->lpBuf;
}

map<DWORD, LPDATA_BUF>::iterator CBuffer::Free(LPBYTE lpBuf)
{
    map<DWORD, LPDATA_BUF>::iterator it = m_mapAlloc.find((DWORD)lpBuf);
    if (it != m_mapAlloc.end())
    {
        it->second->prev = NULL;
        if (it->second->nSize > MIN_BUFSIZE)
        {
            if (m_pExtTail == NULL)
                m_pExtTail = it->second;
            it->second->next = m_pExtHead;
            m_pExtHead = it->second;
        }
        else
        {
            if (m_pTail == NULL)
                m_pTail = it->second;
            it->second->next = m_pHead;
            m_pHead = it->second;
        }
        if (it->second->next)
            it->second->next->prev = it->second;
        it = m_mapAlloc.erase(it);
    }

    return it;
}

void CBuffer::Clear()
{
    for (map<DWORD, LPDATA_BUF>::iterator it = m_mapAlloc.begin(); it != m_mapAlloc.end(); )
        it = Free(it->second->lpBuf);
}

void CBuffer::AssignBuffer(LPDATA_BUF lpBuf)
{
    if (lpBuf->next)
        lpBuf->next->prev = lpBuf->prev;
    if (lpBuf->prev)
        lpBuf->prev->next = lpBuf->next;
    if (lpBuf->nSize > MIN_BUFSIZE)
    {
        if (m_pExtHead == lpBuf)
            m_pExtHead = m_pExtHead->next;
        if (m_pExtTail == lpBuf)
            m_pExtTail = m_pExtTail->prev;
    }
    else
    {
        if (m_pHead == lpBuf)
            m_pHead = m_pHead->next;
        if (m_pTail == lpBuf)
            m_pTail = m_pTail->prev;
    }

    m_mapAlloc.insert(pair<DWORD, LPDATA_BUF>((DWORD)lpBuf->lpBuf, lpBuf));
}