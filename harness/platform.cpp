#include "platform.h"

#include <stdlib.h>

#ifndef _WIN32
    #include <time.h>
    #include <unistd.h>
    #include <errno.h>
#endif

void mutexInit(Mutex *m)
{
#ifdef _WIN32
    InitializeCriticalSection(&m->cs);
#else
    pthread_mutex_init(&m->mutex, NULL);
#endif
}

void mutexDestroy(Mutex *m)
{
#ifdef _WIN32
    DeleteCriticalSection(&m->cs);
#else
    pthread_mutex_destroy(&m->mutex);
#endif
}

void mutexLock(Mutex *m)
{
#ifdef _WIN32
    EnterCriticalSection(&m->cs);
#else
    pthread_mutex_lock(&m->mutex);
#endif
}

void mutexUnlock(Mutex *m)
{
#ifdef _WIN32
    LeaveCriticalSection(&m->cs);
#else
    pthread_mutex_unlock(&m->mutex);
#endif
}

void condInit(Cond *c)
{
#ifdef _WIN32
    InitializeConditionVariable(&c->cv);
#else
    pthread_cond_init(&c->cond, NULL);
#endif
}

void condDestroy(Cond *c)
{
#ifdef _WIN32
    (void)c;
#else
    pthread_cond_destroy(&c->cond);
#endif
}

int condWait(Cond *c, Mutex *m, int timeoutMs)
{
#ifdef _WIN32
    return SleepConditionVariableCS(&c->cv, &m->cs, (DWORD)timeoutMs) ? 1 : 0;
#else
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);

    deadline.tv_sec  += timeoutMs / 1000;
    deadline.tv_nsec += (long)(timeoutMs % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L)
    {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000L;
    }

    return pthread_cond_timedwait(&c->cond, &m->mutex, &deadline) == 0 ? 1 : 0;
#endif
}

void condSignal(Cond *c)
{
#ifdef _WIN32
    WakeConditionVariable(&c->cv);
#else
    pthread_cond_signal(&c->cond);
#endif
}

void condBroadcast(Cond *c)
{
#ifdef _WIN32
    WakeAllConditionVariable(&c->cv);
#else
    pthread_cond_broadcast(&c->cond);
#endif
}

typedef struct {
    ThreadEntry entry;
    void *arg;
} ThreadStart;

#ifdef _WIN32
static DWORD WINAPI threadTrampoline(LPVOID param)
{
    ThreadStart *start = (ThreadStart *)param;
    ThreadEntry entry = start->entry;
    void *arg = start->arg;
    free(start);
    entry(arg);
    return 0;
}
#else
static void *threadTrampoline(void *param)
{
    ThreadStart *start = (ThreadStart *)param;
    ThreadEntry entry = start->entry;
    void *arg = start->arg;
    free(start);
    entry(arg);
    return NULL;
}
#endif

int threadCreate(Thread *t, ThreadEntry entry, void *arg)
{
    ThreadStart *start = (ThreadStart *)malloc(sizeof(ThreadStart));
    if (!start)
        return 0;

    start->entry = entry;
    start->arg   = arg;

#ifdef _WIN32
    t->handle = CreateThread(NULL, 0, threadTrampoline, start, 0, NULL);
    if (!t->handle)
    {
        free(start);
        return 0;
    }
    return 1;
#else
    if (pthread_create(&t->thread, NULL, threadTrampoline, start) != 0)
    {
        free(start);
        return 0;
    }
    return 1;
#endif
}

void threadJoin(Thread *t)
{
#ifdef _WIN32
    if (!t->handle)
        return;
    WaitForSingleObject(t->handle, INFINITE);
    CloseHandle(t->handle);
    t->handle = NULL;
#else
    pthread_join(t->thread, NULL);
#endif
}

double timeNowMs(void)
{
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    static int initialised = 0;

    if (!initialised)
    {
        QueryPerformanceFrequency(&frequency);
        initialised = 1;
    }

    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
#endif
}

void sleepMs(int ms)
{
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
}

int hardwareThreads(void)
{
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return (int)info.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 1;
#endif
}
