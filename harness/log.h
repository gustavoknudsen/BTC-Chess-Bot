#ifndef HARNESS_LOG_H
#define HARNESS_LOG_H

#include "platform.h"

/*
    Forensics log. Only abnormal events are written here: crashes, illegal
    moves, engines that lost on time or never answered. A self play score hides
    those; this file is where they become visible.
*/

typedef struct {
    void *file;
    Mutex lock;
    int open;
    long long events;
} Logger;

int  logOpen(Logger *logger, const char *path);
void logClose(Logger *logger);
void logEvent(Logger *logger, const char *format, ...);

#endif // HARNESS_LOG_H
