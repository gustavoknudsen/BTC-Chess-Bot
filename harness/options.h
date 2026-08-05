#ifndef HARNESS_OPTIONS_H
#define HARNESS_OPTIONS_H

#include "engine.h"
#include "game.h"
#include "sprt.h"

enum { BOOK_EPD = 0, BOOK_PGN };

typedef struct {
    EngineConfig engines[2];
    int engineCount;

    char bookPath[ENGINE_TEXT_MAX];
    int  bookFormat;
    int  bookPlies;       // plies taken from each PGN game
    int  bookStart;       // index of the first opening used
    int  bookRandom;
    int  bookLimit;       // openings held in memory at most

    unsigned long long seed;

    int rounds;           // opening pairs to play at most
    int concurrency;

    SprtConfig sprt;
    GameRules  rules;

    char pgnPath[ENGINE_TEXT_MAX];
    char logPath[ENGINE_TEXT_MAX];
    char tracePath[ENGINE_TEXT_MAX];
    char event[128];

    int selfTest;
} Options;

void optionsDefaults(Options *options);

// returns 1 on success, 0 when the arguments are unusable. errors are printed.
int  optionsParse(Options *options, int argc, char **argv);

void optionsUsage(void);
void timeControlText(const TimeControl *tc, char *out, int outSize);

#endif // HARNESS_OPTIONS_H
