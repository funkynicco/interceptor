#include "StdAfx.h"
#include "Buffer.h"

#include <NativeLib/Util.h>

#define MIN_BUFSIZE 1024

Buffer::Buffer() :
    m_pHead(nullptr),
    m_pTail(nullptr),
    m_pExtHead(nullptr),
    m_pExtTail(nullptr)
{
}

Buffer::~Buffer()
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

    for (std::map<DWORD_PTR, LPDATA_BUF>::iterator it = m_mapAlloc.begin(); it != m_mapAlloc.end(); ++it)
    {
        free(it->second->lpBuf);
        delete it->second;
    }
}

LPBYTE Buffer::Allocate(int nSize)
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

    if (ptr == nullptr)
    {
        nSize = nSize < MIN_BUFSIZE ? MIN_BUFSIZE : nl::util::GetClosestSquared(nSize);
        LPBYTE lpBuf = (LPBYTE)malloc(nSize);
        if (lpBuf == nullptr)
            return nullptr;

        ptr = new DATA_BUF;
        ptr->nSize = nSize;
        ptr->lpBuf = lpBuf;
        ptr->prev = nullptr;
        ptr->next = nullptr;
        m_mapAlloc.insert(std::pair<DWORD_PTR, LPDATA_BUF>((DWORD_PTR)lpBuf, ptr));
    }

    return ptr->lpBuf;
}

std::map<DWORD_PTR, LPDATA_BUF>::iterator Buffer::Free(LPBYTE lpBuf)
{
    std::map<DWORD_PTR, LPDATA_BUF>::iterator it = m_mapAlloc.find((DWORD_PTR)lpBuf);
    if (it != m_mapAlloc.end())
    {
        it->second->prev = nullptr;
        if (it->second->nSize > MIN_BUFSIZE)
        {
            if (m_pExtTail == nullptr)
                m_pExtTail = it->second;
            it->second->next = m_pExtHead;
            m_pExtHead = it->second;
        }
        else
        {
            if (m_pTail == nullptr)
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

void Buffer::Clear()
{
    for (std::map<DWORD_PTR, LPDATA_BUF>::iterator it = m_mapAlloc.begin(); it != m_mapAlloc.end(); )
        it = Free(it->second->lpBuf);
}

void Buffer::AssignBuffer(LPDATA_BUF lpBuf)
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

    m_mapAlloc.insert(std::pair<DWORD_PTR, LPDATA_BUF>((DWORD_PTR)lpBuf->lpBuf, lpBuf));
}