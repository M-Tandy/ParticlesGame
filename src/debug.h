#ifndef ptest_debug_h
#define ptest_debug_h

#include <stdio.h>

void CustomLog(int msgType, const char *text, va_list args);
void LogMessage(int msgType, const char *text, ...);

#endif // ptest_debug_h
