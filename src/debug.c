#include "debug.h"

#include "common.h"
#include <time.h>

// Custom logging function
void CustomLog(int msgType, const char *text, va_list args)
{
    char timeStr[64] = { 0 };
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] ", timeStr);

    switch (msgType)
    {
        case LOG_INFO: printf("[INFO] : "); break;
        case LOG_ERROR: printf("[ERROR]: "); break;
        case LOG_WARNING: printf("[WARN] : "); break;
        case LOG_DEBUG: printf("[DEBUG]: "); break;
        default: break;
    }

    vprintf(text, args);
    printf("\n");
}

void LogMessage(int msgType, const char *format, ...) {
    char timeStr[64] = { 0 };
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(stderr, "[%s] ", timeStr);

    switch (msgType)
    {
        case LOG_INFO: fprintf(stderr, "[INFO] : "); break;
        case LOG_ERROR: fprintf(stderr, "[ERROR]: "); break;
        case LOG_WARNING: fprintf(stderr, "[WARN] : "); break;
        case LOG_DEBUG: fprintf(stderr, "[DEBUG]: "); break;
        default: break;
    }

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fputs("\n", stderr);
}
