#pragma once

#ifdef _WIN32

constexpr const char* NEW_LINE = "\r\n";
constexpr const char* DOUBLE_NEW_LINE = "\r\n\r\n";
constexpr const int COUNT_SYMB_NEW_LINE = 2;

#else

constexpr const char* NEW_LINE = "\n";
constexpr const char* DOUBLE_NEW_LINE = "\n\n";
constexpr const int COUNT_SYMB_NEW_LINE = 1;

#endif // _WIN32