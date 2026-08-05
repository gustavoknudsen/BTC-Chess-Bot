#include "options.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 64

typedef struct {
    char tokens[MAX_TOKENS][256];
    int count;
} TokenList;

static void tokenListAdd(TokenList *list, const char *token)
{
    if (list->count < MAX_TOKENS)
        snprintf(list->tokens[list->count++], 256, "%s", token);
}

// splits "key=value" and returns a pointer to the value, or NULL
static const char *splitKey(const char *token, const char *key)
{
    size_t length = strlen(key);

    if (strncmp(token, key, length) != 0)
        return NULL;
    if (token[length] != '=')
        return NULL;

    return token + length + 1;
}

/*
    Time control text, as used by chess GUIs:
        10+0.1        ten seconds plus a tenth of a second per move
        40/60         forty moves in sixty seconds, repeating
        2:30+2        two and a half minutes plus two seconds
*/
static int parseTimeControl(const char *text, TimeControl *tc)
{
    tc->type = TC_TIME;
    tc->movesToGo = 0;
    tc->incMs = 0.0;

    const char *c = text;

    const char *slash = strchr(c, '/');
    if (slash)
    {
        tc->movesToGo = atoi(c);
        if (tc->movesToGo <= 0)
            return 0;
        c = slash + 1;
    }

    double minutes = 0.0;
    double seconds = 0.0;

    const char *colon = strchr(c, ':');
    if (colon)
    {
        minutes = atof(c);
        c = colon + 1;
    }

    seconds = atof(c);

    const char *plus = strchr(c, '+');
    if (plus)
    {
        seconds = atof(c);           // atof stops at the plus sign
        tc->incMs = atof(plus + 1) * 1000.0;
    }

    tc->timeMs = (minutes * 60.0 + seconds) * 1000.0;

    return tc->timeMs > 0.0;
}

static int applyEngineToken(EngineConfig *config, const char *token)
{
    const char *value;

    if ((value = splitKey(token, "cmd")))
    {
        snprintf(config->command, sizeof(config->command), "%s", value);
        return 1;
    }

    if ((value = splitKey(token, "name")))
    {
        snprintf(config->name, sizeof(config->name), "%s", value);
        return 1;
    }

    if ((value = splitKey(token, "dir")))
    {
        snprintf(config->dir, sizeof(config->dir), "%s", value);
        return 1;
    }

    if ((value = splitKey(token, "restart")))
    {
        config->restartEveryGame = !strcmp(value, "on") || !strcmp(value, "1");
        return 1;
    }

    if ((value = splitKey(token, "tc")))
        return parseTimeControl(value, &config->tc);

    if ((value = splitKey(token, "st")))
    {
        config->tc.type = TC_MOVETIME;
        config->tc.movetimeMs = atof(value) * 1000.0;
        return config->tc.movetimeMs > 0.0;
    }

    if ((value = splitKey(token, "depth")))
    {
        config->tc.type = TC_DEPTH;
        config->tc.depth = atoi(value);
        return config->tc.depth > 0;
    }

    if ((value = splitKey(token, "nodes")))
    {
        config->tc.type = TC_NODES;
        config->tc.nodes = atoll(value);
        return config->tc.nodes > 0;
    }

    if (!strncmp(token, "option.", 7))
    {
        const char *rest = token + 7;
        const char *equals = strchr(rest, '=');
        if (!equals)
            return 0;

        if (config->optionCount >= ENGINE_OPTIONS_MAX)
            return 0;

        int length = (int)(equals - rest);
        if (length > 63)
            length = 63;

        memcpy(config->optionNames[config->optionCount], rest, (size_t)length);
        config->optionNames[config->optionCount][length] = '\0';
        snprintf(config->optionValues[config->optionCount], 64, "%s", equals + 1);
        config->optionCount++;
        return 1;
    }

    fprintf(stderr, "unknown engine option '%s'\n", token);
    return 0;
}

void optionsDefaults(Options *options)
{
    memset(options, 0, sizeof(Options));

    options->rounds = 0;
    options->concurrency = 1;
    options->bookFormat = BOOK_EPD;
    options->bookPlies = 8;
    options->bookLimit = 50000;
    options->seed = 0;

    options->rules.maxMoves = 0;
    options->rules.timeMarginMs = 100;

    snprintf(options->event, sizeof(options->event), "harness match");

    for (int i = 0; i < 2; i++)
    {
        options->engines[i].tc.type = TC_TIME;
        options->engines[i].tc.timeMs = 10000.0;
        options->engines[i].tc.incMs = 100.0;
    }
}

void optionsUsage(void)
{
    printf(
"usage: sprt -engine cmd=A [...] -engine cmd=B [...] -each tc=10+0.1 [options]\n"
"\n"
"engine settings (per -engine, or shared through -each)\n"
"  cmd=COMMAND            command line that starts the engine\n"
"  name=NAME              name used in reports and PGN tags\n"
"  dir=PATH               working directory for the engine\n"
"  tc=[MOVES/]TIME[+INC]  clock in seconds, for example 10+0.1 or 40/60\n"
"  st=SECONDS             fixed time per move\n"
"  depth=N                fixed depth per move\n"
"  nodes=N                fixed nodes per move, the most reproducible mode\n"
"  option.NAME=VALUE      sent as setoption before the first game\n"
"\n"
"match settings\n"
"  -openings file=PATH [format=epd|pgn] [plies=N] [order=sequential|random]\n"
"                       [start=N] [count=N]\n"
"  -rounds N              opening pairs to play, 0 means until the test decides\n"
"  -concurrency N         games in parallel, keep at or below half the cores\n"
"  -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 [minpairs=16]\n"
"  -resign movecount=N score=CP\n"
"  -draw movenumber=N movecount=N score=CP\n"
"  -maxmoves N            adjudicate a draw after N moves\n"
"  -timemargin MS         grace before a clock overrun loses the game\n"
"  -pgnout FILE           append every game, with score and depth comments\n"
"  -log FILE              append crashes, illegal moves and losses on time\n"
"  -event NAME            PGN event tag\n"
"  -seed N                seed for the opening order\n"
"  -selftest              run the internal checks and exit\n"
"\n"
"each opening is played twice with the colours reversed, and results are\n"
"scored as pairs, so an odd number of games is never reported.\n");
}

int optionsParse(Options *options, int argc, char **argv)
{
    optionsDefaults(options);

    TokenList each;
    TokenList perEngine[2];
    memset(&each, 0, sizeof(each));
    memset(perEngine, 0, sizeof(perEngine));

    int engineCount = 0;

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (!strcmp(arg, "-selftest"))
        {
            options->selfTest = 1;
            return 1;
        }

        if (!strcmp(arg, "-h") || !strcmp(arg, "-help") || !strcmp(arg, "--help"))
        {
            optionsUsage();
            return 0;
        }

        if (!strcmp(arg, "-engine") || !strcmp(arg, "-each"))
        {
            TokenList *target;

            if (!strcmp(arg, "-each"))
            {
                target = &each;
            }
            else
            {
                if (engineCount >= 2)
                {
                    fprintf(stderr, "only two engines are supported\n");
                    return 0;
                }
                target = &perEngine[engineCount++];
            }

            while (i + 1 < argc && argv[i + 1][0] != '-')
                tokenListAdd(target, argv[++i]);

            continue;
        }

        if (!strcmp(arg, "-openings"))
        {
            while (i + 1 < argc && argv[i + 1][0] != '-')
            {
                const char *token = argv[++i];
                const char *value;

                if ((value = splitKey(token, "file")))
                    snprintf(options->bookPath, sizeof(options->bookPath), "%s", value);
                else if ((value = splitKey(token, "format")))
                    options->bookFormat = !strcmp(value, "pgn") ? BOOK_PGN : BOOK_EPD;
                else if ((value = splitKey(token, "plies")))
                    options->bookPlies = atoi(value);
                else if ((value = splitKey(token, "order")))
                    options->bookRandom = !strcmp(value, "random");
                else if ((value = splitKey(token, "start")))
                    options->bookStart = atoi(value);
                else if ((value = splitKey(token, "count")))
                    options->bookLimit = atoi(value);
                else
                {
                    fprintf(stderr, "unknown openings option '%s'\n", token);
                    return 0;
                }
            }

            continue;
        }

        if (!strcmp(arg, "-sprt"))
        {
            double elo0 = 0.0, elo1 = 5.0, alpha = 0.05, beta = 0.05;
            long long minPairs = 16;

            while (i + 1 < argc && argv[i + 1][0] != '-')
            {
                const char *token = argv[++i];
                const char *value;

                if ((value = splitKey(token, "elo0")))       elo0  = atof(value);
                else if ((value = splitKey(token, "elo1")))  elo1  = atof(value);
                else if ((value = splitKey(token, "alpha"))) alpha = atof(value);
                else if ((value = splitKey(token, "beta")))  beta  = atof(value);
                else if ((value = splitKey(token, "minpairs"))) minPairs = atoll(value);
                else
                {
                    fprintf(stderr, "unknown sprt option '%s'\n", token);
                    return 0;
                }
            }

            if (elo1 <= elo0 || alpha <= 0.0 || alpha >= 1.0 || beta <= 0.0 || beta >= 1.0)
            {
                fprintf(stderr, "sprt bounds are not usable\n");
                return 0;
            }

            options->sprt.minPairs = minPairs;
            sprtConfigure(&options->sprt, elo0, elo1, alpha, beta);
            continue;
        }

        if (!strcmp(arg, "-resign"))
        {
            options->rules.resignEnabled = 1;
            options->rules.resignMoveCount = 4;
            options->rules.resignScore = 700;

            while (i + 1 < argc && argv[i + 1][0] != '-')
            {
                const char *token = argv[++i];
                const char *value;

                if ((value = splitKey(token, "movecount")))  options->rules.resignMoveCount = atoi(value);
                else if ((value = splitKey(token, "score"))) options->rules.resignScore = atoi(value);
                else
                {
                    fprintf(stderr, "unknown resign option '%s'\n", token);
                    return 0;
                }
            }

            continue;
        }

        if (!strcmp(arg, "-draw"))
        {
            options->rules.drawEnabled = 1;
            options->rules.drawMoveNumber = 40;
            options->rules.drawMoveCount = 8;
            options->rules.drawScore = 10;

            while (i + 1 < argc && argv[i + 1][0] != '-')
            {
                const char *token = argv[++i];
                const char *value;

                if ((value = splitKey(token, "movenumber")))      options->rules.drawMoveNumber = atoi(value);
                else if ((value = splitKey(token, "movecount")))  options->rules.drawMoveCount = atoi(value);
                else if ((value = splitKey(token, "score")))      options->rules.drawScore = atoi(value);
                else
                {
                    fprintf(stderr, "unknown draw option '%s'\n", token);
                    return 0;
                }
            }

            continue;
        }

        if (!strcmp(arg, "-rounds") && i + 1 < argc)      { options->rounds = atoi(argv[++i]); continue; }
        if (!strcmp(arg, "-concurrency") && i + 1 < argc) { options->concurrency = atoi(argv[++i]); continue; }
        if (!strcmp(arg, "-maxmoves") && i + 1 < argc)    { options->rules.maxMoves = atoi(argv[++i]); continue; }
        if (!strcmp(arg, "-timemargin") && i + 1 < argc)  { options->rules.timeMarginMs = atoi(argv[++i]); continue; }
        if (!strcmp(arg, "-seed") && i + 1 < argc)        { options->seed = strtoull(argv[++i], NULL, 10); continue; }

        if (!strcmp(arg, "-pgnout") && i + 1 < argc)
        {
            snprintf(options->pgnPath, sizeof(options->pgnPath), "%s", argv[++i]);
            continue;
        }

        if (!strcmp(arg, "-trace") && i + 1 < argc)
        {
            snprintf(options->tracePath, sizeof(options->tracePath), "%s", argv[++i]);
            continue;
        }

        if (!strcmp(arg, "-log") && i + 1 < argc)
        {
            snprintf(options->logPath, sizeof(options->logPath), "%s", argv[++i]);
            continue;
        }

        if (!strcmp(arg, "-event") && i + 1 < argc)
        {
            snprintf(options->event, sizeof(options->event), "%s", argv[++i]);
            continue;
        }

        fprintf(stderr, "unknown argument '%s'\n", arg);
        return 0;
    }

    if (engineCount != 2)
    {
        fprintf(stderr, "two engines are required, use -engine twice\n");
        return 0;
    }

    options->engineCount = 2;

    // shared settings are applied first so that per engine settings win
    for (int e = 0; e < 2; e++)
    {
        for (int t = 0; t < each.count; t++)
            if (!applyEngineToken(&options->engines[e], each.tokens[t]))
                return 0;

        for (int t = 0; t < perEngine[e].count; t++)
            if (!applyEngineToken(&options->engines[e], perEngine[e].tokens[t]))
                return 0;

        if (!options->engines[e].command[0])
        {
            fprintf(stderr, "engine %d has no cmd=\n", e + 1);
            return 0;
        }

        if (!options->engines[e].name[0])
            snprintf(options->engines[e].name, sizeof(options->engines[e].name),
                     "engine%d", e + 1);
    }

    if (!strcmp(options->engines[0].name, options->engines[1].name))
    {
        snprintf(options->engines[0].name, sizeof(options->engines[0].name), "%s (1)",
                 options->engines[1].name);
        snprintf(options->engines[1].name, sizeof(options->engines[1].name), "%s (2)",
                 options->engines[1].name);
    }

    if (options->concurrency < 1)
        options->concurrency = 1;

    if (options->concurrency > hardwareThreads())
        fprintf(stderr, "warning: concurrency %d is above the %d hardware threads, "
                        "time controls will not be honest\n",
                options->concurrency, hardwareThreads());

    if (options->rounds <= 0 && !options->sprt.enabled)
    {
        fprintf(stderr, "either -rounds or -sprt is required\n");
        return 0;
    }

    if (options->bookPlies <= 0 || options->bookPlies > BOOK_MOVES_MAX)
        options->bookPlies = 8;

    if (options->bookLimit <= 0)
        options->bookLimit = 50000;

    return 1;
}

void timeControlText(const TimeControl *tc, char *out, int outSize)
{
    switch (tc->type)
    {
        case TC_MOVETIME:
            snprintf(out, outSize, "%.3fs per move", tc->movetimeMs / 1000.0);
            break;

        case TC_DEPTH:
            snprintf(out, outSize, "depth %d", tc->depth);
            break;

        case TC_NODES:
            snprintf(out, outSize, "%lld nodes", tc->nodes);
            break;

        default:
            if (tc->movesToGo > 0)
                snprintf(out, outSize, "%d/%.1f+%.2f", tc->movesToGo,
                         tc->timeMs / 1000.0, tc->incMs / 1000.0);
            else
                snprintf(out, outSize, "%.1f+%.2f", tc->timeMs / 1000.0, tc->incMs / 1000.0);
            break;
    }
}
