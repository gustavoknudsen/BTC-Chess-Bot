#ifndef HARNESS_PLATFORM_H
#define HARNESS_PLATFORM_H

/*
    Threads, locks and a monotonic clock.

    Everything the harness needs from the operating system that is not process
    handling lives here, so the rest of the code stays platform free.
*/

#ifdef _WIN32
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601
    #endif
    #include <windows.h>
#else
    #include <pthread.h>
#endif

typedef struct {
#ifdef _WIN32
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t mutex;
#endif
} Mutex;

typedef struct {
#ifdef _WIN32
    CONDITION_VARIABLE cv;
#else
    pthread_cond_t cond;
#endif
} Cond;

typedef struct {
#ifdef _WIN32
    HANDLE handle;
#else
    pthread_t thread;
#endif
} Thread;

typedef void (*ThreadEntry)(void *arg);

void mutexInit(Mutex *m);
void mutexDestroy(Mutex *m);
void mutexLock(Mutex *m);
void mutexUnlock(Mutex *m);

void condInit(Cond *c);
void condDestroy(Cond *c);

// waits until signalled or the timeout expires. returns 1 when signalled,
// 0 on timeout. the mutex must be held and is held again on return.
int  condWait(Cond *c, Mutex *m, int timeoutMs);
void condSignal(Cond *c);
void condBroadcast(Cond *c);

int  threadCreate(Thread *t, ThreadEntry entry, void *arg);
void threadJoin(Thread *t);

// monotonic milliseconds, fractional. not tied to wall clock time.
double timeNowMs(void);
void   sleepMs(int ms);

// number of hardware threads, used only as a default for concurrency
int hardwareThreads(void);

#endif // HARNESS_PLATFORM_H
