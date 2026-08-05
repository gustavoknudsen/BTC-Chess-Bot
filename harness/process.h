#ifndef HARNESS_PROCESS_H
#define HARNESS_PROCESS_H

#include "platform.h"

/*
    Child process with pipes on its standard input and output.

    A dedicated reader thread per process does the blocking read and stamps
    every line with its arrival time. That matters for time control accuracy:
    the clock charged to an engine ends when its "bestmove" line actually
    arrived, not when the game thread got around to looking at it.
*/

#define PROC_LINE_MAX   4096
#define PROC_WRITE_MAX  16384
#define PROC_QUEUE_SIZE 256
#define PROC_RECENT     32
#define PROC_RECENT_MAX 512

typedef struct {
    char   text[PROC_LINE_MAX];
    double arrival;
} ProcLine;

typedef struct {
#ifdef _WIN32
    HANDLE process;
    HANDLE stdinWrite;
    HANDLE stdoutRead;
#else
    int pid;
    int stdinWrite;
    int stdoutRead;
#endif

    int started;
    int readerRunning;

    Thread reader;
    Mutex  lock;
    Cond   ready;

    ProcLine queue[PROC_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    int eof;
    int overflowed;

    // last few lines seen, kept for the forensics log
    char recent[PROC_RECENT][PROC_RECENT_MAX];
    int  recentCount;
    int  recentNext;
} Process;

// command is a full command line, run as given. workDir may be NULL.
int  processStart(Process *p, const char *command, const char *workDir);

int  processWriteLine(Process *p, const char *line);

// returns 1 and fills out on success, 0 on timeout, -1 when the pipe closed
// and the queue is empty
int  processReadLine(Process *p, int timeoutMs, char *out, int outSize, double *arrival);

int  processAlive(Process *p);

// returns 1 if the child exited within the timeout
int  processWaitExit(Process *p, int timeoutMs);

void processKill(Process *p);
void processClose(Process *p);

// most recent output lines, oldest first, newline separated
void processRecentOutput(Process *p, char *out, int outSize);

#endif // HARNESS_PROCESS_H
