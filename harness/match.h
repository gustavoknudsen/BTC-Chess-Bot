#ifndef HARNESS_MATCH_H
#define HARNESS_MATCH_H

#include "options.h"
#include "book.h"
#include "pgn.h"
#include "log.h"
#include "sprt.h"

/*
    Tournament driver. Worker threads claim opening pairs, play both games of
    the pair, and fold the result into the shared statistics. Only complete
    pairs are counted, so the pentanomial totals always describe every game
    that was scored.
*/

typedef struct {
    const Options *options;
    Book      *book;
    PgnWriter *pgn;
    Logger    *logger;

    Mutex lock;
    int   nextRound;
    int   stop;
    int   verdict;
    int   fatal;

    Stats  stats;
    double startTime;

    long long crashes;
    long long timeLosses;
    long long illegalMoves;
    long long adjudications;
    long long abortedPairs;
} Tournament;

// asks a running tournament to finish after the games in flight
void matchInterrupt(void);

int matchRun(Tournament *tournament);

#endif // HARNESS_MATCH_H
