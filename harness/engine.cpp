#include "engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HANDSHAKE_TIMEOUT_MS 15000
#define READY_TIMEOUT_MS     30000

static Logger *traceLog = NULL;

void engineSetTrace(Logger *logger)
{
    traceLog = logger;
}

static void trace(Engine *e, const char *direction, const char *text)
{
    if (traceLog)
        logEvent(traceLog, "%s %s %s", e->config->name, direction, text);
}

// every command goes through here so the trace cannot miss one
static int sendCommand(Engine *e, const char *command)
{
    trace(e, ">", command);
    return processWriteLine(&e->proc, command);
}

// waits for a line starting with the given token, discarding anything else.
// returns 1 on success, 0 on timeout, -1 if the engine died.
static int waitForToken(Engine *e, const char *token, int timeoutMs)
{
    char line[PROC_LINE_MAX];
    double deadline = timeNowMs() + timeoutMs;

    while (1)
    {
        int remaining = (int)(deadline - timeNowMs());
        if (remaining <= 0)
            return 0;

        int status = processReadLine(&e->proc, remaining, line, sizeof(line), NULL);

        if (status < 0)
            return -1;
        if (status == 0)
            return 0;

        trace(e, "<", line);

        if (!strncmp(line, "id name", 7))
            snprintf(e->idName, sizeof(e->idName), "%s", line + 8);

        if (!strncmp(line, token, strlen(token)))
            return 1;
    }
}

int engineStart(Engine *e, const EngineConfig *config)
{
    memset(e, 0, sizeof(Engine));
    e->config = config;

    if (!processStart(&e->proc, config->command, config->dir[0] ? config->dir : NULL))
    {
        snprintf(e->lastError, sizeof(e->lastError), "failed to start '%s'", config->command);
        return 0;
    }

    e->started = 1;

    if (!sendCommand(e, "uci"))
    {
        snprintf(e->lastError, sizeof(e->lastError), "failed to write to engine");
        return 0;
    }

    if (waitForToken(e, "uciok", HANDSHAKE_TIMEOUT_MS) != 1)
    {
        snprintf(e->lastError, sizeof(e->lastError), "no uciok from '%s'", config->command);
        return 0;
    }

    for (int i = 0; i < config->optionCount; i++)
    {
        char command[256];
        snprintf(command, sizeof(command), "setoption name %s value %s",
                 config->optionNames[i], config->optionValues[i]);
        sendCommand(e, command);
    }

    if (!sendCommand(e, "isready") ||
        waitForToken(e, "readyok", READY_TIMEOUT_MS) != 1)
    {
        snprintf(e->lastError, sizeof(e->lastError), "no readyok from '%s'", config->command);
        return 0;
    }

    return 1;
}

int engineNewGame(Engine *e)
{
    if (!e->started)
        return 0;

    if (!sendCommand(e, "ucinewgame"))
    {
        snprintf(e->lastError, sizeof(e->lastError), "engine closed its input");
        return 0;
    }

    if (!sendCommand(e, "isready") ||
        waitForToken(e, "readyok", READY_TIMEOUT_MS) != 1)
    {
        snprintf(e->lastError, sizeof(e->lastError), "no readyok after ucinewgame");
        return 0;
    }

    return 1;
}

// pulls depth, nodes and score out of an info line. token order is not fixed
// by the protocol, so every token is inspected.
static void parseInfo(Engine *e, const char *line)
{
    if (!strncmp(line, "info string", 11))
        return;

    const char *c = line;

    while (*c)
    {
        while (*c == ' ') c++;
        if (!*c) break;

        if (!strncmp(c, "depth ", 6))
        {
            int value = atoi(c + 6);
            if (value > 0)
                e->depth = value;
        }
        else if (!strncmp(c, "nodes ", 6))
        {
            e->nodes = atoll(c + 6);
        }
        else if (!strncmp(c, "score cp ", 9))
        {
            e->score = atoi(c + 9);
            e->scoreIsMate = 0;
            e->haveScore = 1;
        }
        else if (!strncmp(c, "score mate ", 11))
        {
            e->score = atoi(c + 11);
            e->scoreIsMate = 1;
            e->haveScore = 1;
        }

        while (*c && *c != ' ') c++;
    }
}

int engineGo(Engine *e, const char *positionCommand, const char *goCommand, int hardTimeoutMs)
{
    e->bestmove[0] = '\0';
    e->haveScore   = 0;
    e->scoreIsMate = 0;
    e->score       = 0;
    e->depth       = 0;
    e->nodes       = 0;
    e->elapsedMs   = 0.0;

    if (!e->started)
    {
        snprintf(e->lastError, sizeof(e->lastError), "engine is not running");
        return 0;
    }

    if (!sendCommand(e, positionCommand))
    {
        snprintf(e->lastError, sizeof(e->lastError), "engine closed its input");
        return 0;
    }

    // the clock starts here: the engine begins its own timing when it reads go
    double sent = timeNowMs();

    if (!sendCommand(e, goCommand))
    {
        snprintf(e->lastError, sizeof(e->lastError), "engine closed its input");
        return 0;
    }

    double deadline = sent + hardTimeoutMs;
    char line[PROC_LINE_MAX];

    while (1)
    {
        int remaining = (int)(deadline - timeNowMs());
        if (remaining <= 0)
        {
            snprintf(e->lastError, sizeof(e->lastError),
                     "no bestmove within %d ms", hardTimeoutMs);
            return 0;
        }

        double arrival = 0.0;
        int status = processReadLine(&e->proc, remaining, line, sizeof(line), &arrival);

        if (status < 0)
        {
            snprintf(e->lastError, sizeof(e->lastError), "engine exited during search");
            return 0;
        }

        if (status == 0)
            continue;

        trace(e, "<", line);

        if (!strncmp(line, "bestmove", 8))
        {
            e->elapsedMs = arrival - sent;
            if (e->elapsedMs < 0.0)
                e->elapsedMs = 0.0;

            const char *c = line + 8;
            while (*c == ' ') c++;

            int n = 0;
            while (*c && *c != ' ' && n < (int)sizeof(e->bestmove) - 1)
                e->bestmove[n++] = *c++;
            e->bestmove[n] = '\0';

            if (!n)
            {
                snprintf(e->lastError, sizeof(e->lastError), "empty bestmove");
                return 0;
            }

            return 1;
        }

        if (!strncmp(line, "info", 4))
            parseInfo(e, line);
    }
}

void engineQuit(Engine *e)
{
    if (!e->started)
        return;

    sendCommand(e, "quit");
    processClose(&e->proc);
    e->started = 0;
}

int engineAlive(Engine *e)
{
    return e->started && processAlive(&e->proc);
}

void engineRecentOutput(Engine *e, char *out, int outSize)
{
    if (!e->started)
    {
        snprintf(out, outSize, "    (engine not running)\n");
        return;
    }

    processRecentOutput(&e->proc, out, outSize);
}
