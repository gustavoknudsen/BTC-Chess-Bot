#include "process.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
    #include <unistd.h>
    #include <signal.h>
    #ifdef __linux__
        #include <sys/prctl.h>
    #endif
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <errno.h>
#endif

// spawning is serialised: on Windows a concurrent CreateProcess can leak an
// unrelated inheritable handle into a child, which then keeps a pipe alive and
// hides the end of file from its sibling
static Mutex spawnLock;
static int spawnLockReady = 0;

#ifdef _WIN32
/*
    Every engine is assigned to one job object whose limits say "kill everything
    in this job when the last handle to it closes". The harness holds the only
    handle, so if the harness exits, crashes, or is killed outright, the
    operating system terminates the engines with it.

    Without this, killing the harness orphans its engines. An orphan does not
    necessarily die quietly: BetterThanCris spins on its input rather than
    exiting at end of file, so orphans sit at full CPU forever, and enough of
    them starve the next run badly enough to decide most of its games on time.
*/
static HANDLE childJob = NULL;

static void ensureChildJob(void)
{
    if (childJob)
        return;

    childJob = CreateJobObjectA(NULL, NULL);
    if (!childJob)
        return;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
    memset(&limits, 0, sizeof(limits));
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

    if (!SetInformationJobObject(childJob, JobObjectExtendedLimitInformation,
                                 &limits, sizeof(limits)))
    {
        CloseHandle(childJob);
        childJob = NULL;
    }
}
#endif

static void ensureSpawnLock(void)
{
    if (!spawnLockReady)
    {
        mutexInit(&spawnLock);
        spawnLockReady = 1;
#ifdef _WIN32
        ensureChildJob();
#endif
    }
}

static void pushLine(Process *p, const char *text, double arrival)
{
    mutexLock(&p->lock);

    if (p->count < PROC_QUEUE_SIZE)
    {
        ProcLine *line = &p->queue[p->tail];
        snprintf(line->text, PROC_LINE_MAX, "%s", text);
        line->arrival = arrival;
        p->tail = (p->tail + 1) % PROC_QUEUE_SIZE;
        p->count++;
    }
    else
    {
        p->overflowed = 1;
    }

    snprintf(p->recent[p->recentNext], PROC_RECENT_MAX, "%s", text);
    p->recentNext = (p->recentNext + 1) % PROC_RECENT;
    if (p->recentCount < PROC_RECENT)
        p->recentCount++;

    condBroadcast(&p->ready);
    mutexUnlock(&p->lock);
}

static void readerThread(void *arg)
{
    Process *p = (Process *)arg;

    char partial[PROC_LINE_MAX];
    int partialLen = 0;
    char buffer[4096];

    while (1)
    {
        int bytes = 0;

#ifdef _WIN32
        DWORD read = 0;
        if (!ReadFile(p->stdoutRead, buffer, sizeof(buffer), &read, NULL) || read == 0)
            break;
        bytes = (int)read;
#else
        bytes = (int)read(p->stdoutRead, buffer, sizeof(buffer));
        if (bytes <= 0)
        {
            if (bytes < 0 && errno == EINTR)
                continue;
            break;
        }
#endif

        double arrival = timeNowMs();

        for (int i = 0; i < bytes; i++)
        {
            char ch = buffer[i];

            if (ch == '\r')
                continue;

            if (ch == '\n')
            {
                partial[partialLen] = '\0';
                if (partialLen)
                    pushLine(p, partial, arrival);
                partialLen = 0;
                continue;
            }

            if (partialLen < PROC_LINE_MAX - 1)
                partial[partialLen++] = ch;
        }
    }

    mutexLock(&p->lock);
    p->eof = 1;
    condBroadcast(&p->ready);
    mutexUnlock(&p->lock);
}

int processStart(Process *p, const char *command, const char *workDir)
{
    memset(p, 0, sizeof(Process));
    mutexInit(&p->lock);
    condInit(&p->ready);
    ensureSpawnLock();

#ifdef _WIN32
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE childStdinRead = NULL, parentStdinWrite = NULL;
    HANDLE parentStdoutRead = NULL, childStdoutWrite = NULL;

    mutexLock(&spawnLock);

    if (!CreatePipe(&childStdinRead, &parentStdinWrite, &sa, 0) ||
        !CreatePipe(&parentStdoutRead, &childStdoutWrite, &sa, 0))
    {
        mutexUnlock(&spawnLock);
        return 0;
    }

    // the parent ends must not reach the child
    SetHandleInformation(parentStdinWrite, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parentStdoutRead, HANDLE_FLAG_INHERIT, 0);

    // inherit exactly the two pipe ends the child needs, so a process spawned
    // on another thread cannot hand this child a stray handle
    HANDLE inherit[2] = { childStdinRead, childStdoutWrite };

    STARTUPINFOEXA startup;
    memset(&startup, 0, sizeof(startup));
    startup.StartupInfo.cb = sizeof(STARTUPINFOEXA);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput  = childStdinRead;
    startup.StartupInfo.hStdOutput = childStdoutWrite;
    startup.StartupInfo.hStdError  = childStdoutWrite;

    SIZE_T attributeSize = 0;
    InitializeProcThreadAttributeList(NULL, 1, 0, &attributeSize);

    startup.lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)malloc(attributeSize);

    int haveAttributes = 0;
    if (startup.lpAttributeList &&
        InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attributeSize) &&
        UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                  inherit, sizeof(inherit), NULL, NULL))
    {
        haveAttributes = 1;
    }

    char *commandLine = (char *)malloc(strlen(command) + 1);
    strcpy(commandLine, command);

    PROCESS_INFORMATION info;
    memset(&info, 0, sizeof(info));

    // suspended, so the child cannot spawn anything of its own before it has
    // been placed in the job
    DWORD flags = CREATE_NO_WINDOW | CREATE_SUSPENDED;
    if (haveAttributes)
        flags |= EXTENDED_STARTUPINFO_PRESENT;

    BOOL ok = CreateProcessA(NULL, commandLine, NULL, NULL, TRUE, flags, NULL,
                             workDir, &startup.StartupInfo, &info);

    free(commandLine);

    if (haveAttributes)
        DeleteProcThreadAttributeList(startup.lpAttributeList);
    free(startup.lpAttributeList);

    CloseHandle(childStdinRead);
    CloseHandle(childStdoutWrite);

    mutexUnlock(&spawnLock);

    if (!ok)
    {
        CloseHandle(parentStdinWrite);
        CloseHandle(parentStdoutRead);
        return 0;
    }

    if (childJob)
        AssignProcessToJobObject(childJob, info.hProcess);

    ResumeThread(info.hThread);
    CloseHandle(info.hThread);

    p->process    = info.hProcess;
    p->stdinWrite = parentStdinWrite;
    p->stdoutRead = parentStdoutRead;
#else
    int inPipe[2], outPipe[2];

    mutexLock(&spawnLock);

    if (pipe(inPipe) != 0)
    {
        mutexUnlock(&spawnLock);
        return 0;
    }

    if (pipe(outPipe) != 0)
    {
        close(inPipe[0]);
        close(inPipe[1]);
        mutexUnlock(&spawnLock);
        return 0;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        close(inPipe[0]);  close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        mutexUnlock(&spawnLock);
        return 0;
    }

    if (pid == 0)
    {
        dup2(inPipe[0], 0);
        dup2(outPipe[1], 1);
        dup2(outPipe[1], 2);

        close(inPipe[0]);  close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);

        if (workDir && workDir[0])
        {
            if (chdir(workDir) != 0)
                _exit(127);
        }

#ifdef __linux__
        // the same guarantee the job object gives on Windows
        prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif

        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }

    close(inPipe[0]);
    close(outPipe[1]);

    mutexUnlock(&spawnLock);

    p->pid        = pid;
    p->stdinWrite = inPipe[1];
    p->stdoutRead = outPipe[0];
#endif

    p->started = 1;

    if (!threadCreate(&p->reader, readerThread, p))
    {
        processKill(p);
        return 0;
    }

    p->readerRunning = 1;
    return 1;
}

int processWriteLine(Process *p, const char *line)
{
    if (!p->started)
        return 0;

    // a position command carrying a long game is far longer than any line an
    // engine sends back, so writes get their own larger buffer
    char buffer[PROC_WRITE_MAX];
    int length = snprintf(buffer, sizeof(buffer), "%s\n", line);
    if (length < 0)
        return 0;
    if (length >= (int)sizeof(buffer))
        length = (int)sizeof(buffer) - 1;

#ifdef _WIN32
    DWORD written = 0;
    if (!WriteFile(p->stdinWrite, buffer, (DWORD)length, &written, NULL))
        return 0;
    return written == (DWORD)length;
#else
    int offset = 0;
    while (offset < length)
    {
        int written = (int)write(p->stdinWrite, buffer + offset, length - offset);
        if (written <= 0)
        {
            if (errno == EINTR)
                continue;
            return 0;
        }
        offset += written;
    }
    return 1;
#endif
}

int processReadLine(Process *p, int timeoutMs, char *out, int outSize, double *arrival)
{
    double deadline = timeNowMs() + timeoutMs;
    int result = 0;

    mutexLock(&p->lock);

    while (1)
    {
        if (p->count > 0)
        {
            ProcLine *line = &p->queue[p->head];
            snprintf(out, outSize, "%s", line->text);
            if (arrival)
                *arrival = line->arrival;
            p->head = (p->head + 1) % PROC_QUEUE_SIZE;
            p->count--;
            result = 1;
            break;
        }

        if (p->eof)
        {
            result = -1;
            break;
        }

        int remaining = (int)(deadline - timeNowMs());
        if (remaining <= 0)
        {
            result = 0;
            break;
        }

        condWait(&p->ready, &p->lock, remaining);
    }

    mutexUnlock(&p->lock);
    return result;
}

int processAlive(Process *p)
{
    if (!p->started)
        return 0;

#ifdef _WIN32
    return WaitForSingleObject(p->process, 0) == WAIT_TIMEOUT;
#else
    int status = 0;
    pid_t result = waitpid(p->pid, &status, WNOHANG);
    return result == 0;
#endif
}

int processWaitExit(Process *p, int timeoutMs)
{
    if (!p->started)
        return 1;

#ifdef _WIN32
    return WaitForSingleObject(p->process, (DWORD)timeoutMs) == WAIT_OBJECT_0;
#else
    double deadline = timeNowMs() + timeoutMs;
    while (timeNowMs() < deadline)
    {
        int status = 0;
        if (waitpid(p->pid, &status, WNOHANG) != 0)
            return 1;
        sleepMs(2);
    }
    return 0;
#endif
}

void processKill(Process *p)
{
    if (!p->started)
        return;

#ifdef _WIN32
    TerminateProcess(p->process, 1);
#else
    kill(p->pid, SIGKILL);
    int status = 0;
    waitpid(p->pid, &status, 0);
#endif
}

void processClose(Process *p)
{
    if (!p->started)
        return;

    // closing the child's stdin ends the engine's input, which is how a well
    // behaved engine notices it should exit after "quit"
#ifdef _WIN32
    if (p->stdinWrite)
    {
        CloseHandle(p->stdinWrite);
        p->stdinWrite = NULL;
    }
#else
    if (p->stdinWrite >= 0)
    {
        close(p->stdinWrite);
        p->stdinWrite = -1;
    }
#endif

    if (!processWaitExit(p, 500))
        processKill(p);

    // the reader thread returns once the write end of the output pipe is gone,
    // which the dead child guarantees
    if (p->readerRunning)
    {
        threadJoin(&p->reader);
        p->readerRunning = 0;
    }

#ifdef _WIN32
    if (p->stdoutRead)
    {
        CloseHandle(p->stdoutRead);
        p->stdoutRead = NULL;
    }
    if (p->process)
    {
        CloseHandle(p->process);
        p->process = NULL;
    }
#else
    if (p->stdoutRead >= 0)
    {
        close(p->stdoutRead);
        p->stdoutRead = -1;
    }
#endif

    p->started = 0;

    mutexDestroy(&p->lock);
    condDestroy(&p->ready);
}

void processRecentOutput(Process *p, char *out, int outSize)
{
    mutexLock(&p->lock);

    int offset = 0;
    out[0] = '\0';

    int start = (p->recentNext - p->recentCount + PROC_RECENT) % PROC_RECENT;

    for (int i = 0; i < p->recentCount && offset < outSize - 1; i++)
    {
        int index = (start + i) % PROC_RECENT;
        offset += snprintf(out + offset, outSize - offset, "    %s\n", p->recent[index]);
        if (offset >= outSize - 1)
            break;
    }

    mutexUnlock(&p->lock);
}
