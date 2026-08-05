#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

int logOpen(Logger *logger, const char *path)
{
    memset(logger, 0, sizeof(Logger));

    if (!path || !path[0])
        return 1;

    FILE *file = fopen(path, "a");
    if (!file)
        return 0;

    logger->file = file;
    logger->open = 1;
    mutexInit(&logger->lock);
    return 1;
}

void logClose(Logger *logger)
{
    if (!logger->open)
        return;

    fclose((FILE *)logger->file);
    mutexDestroy(&logger->lock);
    logger->open = 0;
    logger->file = NULL;
}

void logEvent(Logger *logger, const char *format, ...)
{
    if (!logger->open)
        return;

    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    char stamp[32];
    strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", local);

    mutexLock(&logger->lock);

    FILE *file = (FILE *)logger->file;
    fprintf(file, "[%s] ", stamp);

    va_list args;
    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fputc('\n', file);
    fflush(file);

    logger->events++;

    mutexUnlock(&logger->lock);
}
