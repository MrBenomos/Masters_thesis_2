#pragma once

#ifdef _WIN32

#define NEW_LINE "\r\n"
#define DOUBLE_NEW_LINE  "\r\n\r\n"
#define COUNT_SYMB_NEW_LINE 2

#else

#define NEW_LINE "\n"
#define DOUBLE_NEW_LINE  "\n\n"
#define COUNT_SYMB_NEW_LINE 1

#endif // _WIN32