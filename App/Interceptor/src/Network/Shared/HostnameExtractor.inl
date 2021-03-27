#pragma once

inline BOOL ValidateAndExtractHost(
    const char* request_html,
    size_t request_len,
    char* host,
    size_t sizeOfHost,
    const char* defaultHost = nullptr)
{
    BOOL bIsPost = (request_len >= 6 && memcmp(request_html, "POST /", 6) == 0);

    if (!bIsPost && (
        request_len < 5 ||
        memcmp(request_html, "GET /", 5) != 0))
        return FALSE;

    const char* ptr = request_html + (bIsPost ? 6 : 5);
    const char* end = request_html + request_len;
    BOOL bFirstLine = TRUE;

    char keyword[128];
    char* keyword_ptr = keyword;
    char value[512];
    char* value_ptr = value;
    BOOL bIsValue = FALSE;

    *host = 0;

    while (ptr < end)
    {
        if (*ptr == '\r')
        {
            ++ptr;
            continue;
        }

        if (*ptr == '\n')
        {
            if (!bFirstLine &&
                *keyword &&
                *value)
            {
                if (_strcmpi(keyword, "host") == 0)
                    strcpy(host, value);
            }

            keyword_ptr = keyword;
            value_ptr = value;
            bIsValue = FALSE;
            bFirstLine = FALSE;
            if (ptr + 1 < end &&
                (*(ptr + 1) == '\n' ||
                    (*(ptr + 1) == '\r' && ptr + 2 < end && *(ptr + 2) == '\n')))
            {
                if (*host == 0 &&
                    defaultHost != nullptr)
                    strcpy(host, defaultHost);

                return TRUE;
            }
        }
        else
        {
            if (!bFirstLine)
            {
                if (*ptr == ':' &&
                    ptr + 1 < end &&
                    *(ptr + 1) == ' ')
                {
                    bIsValue = TRUE;
                    ++ptr;
                }
                else
                {
                    char*& kv_ptr = bIsValue ? value_ptr : keyword_ptr;
                    *kv_ptr++ = *ptr;
                    *kv_ptr = 0;
                }
            }
        }

        ++ptr;
    }

    return FALSE;
}

inline BOOL ValidateAndExtractRequestHeader(
    const char* request_html,
    size_t request_len,
    std::map<std::string, std::string>& headers,
    size_t* pnHeaderLength = nullptr)
{
    BOOL bIsPost = (request_len >= 6 && memcmp(request_html, "POST /", 6) == 0);

    if (!bIsPost && (
        request_len < 5 ||
        memcmp(request_html, "GET /", 5) != 0))
        return FALSE;

    const char* ptr = request_html + (bIsPost ? 6 : 5);
    const char* end = request_html + request_len;
    BOOL bFirstLine = TRUE;

    char keyword[128];
    char* keyword_ptr = keyword;
    char value[512];
    char* value_ptr = value;
    BOOL bIsValue = FALSE;

    while (ptr < end)
    {
        if (*ptr == '\r')
        {
            ++ptr;
            continue;
        }

        if (*ptr == ' ' &&
            bFirstLine)
        {
            size_t val_len = ptr - (request_html + (bIsPost ? 5 : 4));
            memcpy(value, request_html + (bIsPost ? 5 : 4), val_len);
            value[val_len] = 0;

            headers.insert(std::pair<std::string, std::string>("_GET_QUERY_", value));
        }

        if (*ptr == '\n')
        {
            if (!bFirstLine &&
                *keyword &&
                *value)
            {
                headers.insert(std::pair<std::string, std::string>(keyword, value));
            }

            keyword_ptr = keyword;
            value_ptr = value;
            bIsValue = FALSE;
            bFirstLine = FALSE;
            if (ptr + 1 < end &&
                (*(ptr + 1) == '\n' ||
                    (*(ptr + 1) == '\r' && ptr + 2 < end && *(ptr + 2) == '\n')))
            {
                if (pnHeaderLength)
                {
                    if ((*ptr + 1) == '\n')
                        *pnHeaderLength = size_t((ptr + 1) - request_html);
                    else
                        *pnHeaderLength = size_t((ptr + 3) - request_html);
                }

                return TRUE;
            }
        }
        else
        {
            if (!bFirstLine)
            {
                if (*ptr == ':' &&
                    ptr + 1 < end &&
                    *(ptr + 1) == ' ')
                {
                    bIsValue = TRUE;
                    ++ptr;
                }
                else
                {
                    char*& kv_ptr = bIsValue ? value_ptr : keyword_ptr;
                    *kv_ptr++ = *ptr;
                    *kv_ptr = 0;
                }
            }
        }

        ++ptr;
    }

    return FALSE;
}