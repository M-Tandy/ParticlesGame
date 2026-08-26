#ifndef ptest_debug_h
#define ptest_debug_h

#include <common.h>
#include <stdio.h>

void CustomLog(int msgType, const char *text, va_list args);
void LogMessage(int msgType, const char *text, ...);
void DebugDrawCameraTargetGuides(Camera2D camera);

#endif // ptest_debug_h
