#pragma once

void WriteDebug(const char* format, ...);

inline size_t StringViewToCharArray(char* dst, size_t size, std::string_view src)
{
    if (src.length() + 1 > size)
        return 0;

    memcpy(dst, src.data(), src.length());
    dst[src.length()] = 0;
    return src.length();
}

template <size_t _Size>
inline size_t StringViewToCharArray(char(&dst)[_Size], std::string_view src)
{
    return StringViewToCharArray(dst, _Size, src);
}