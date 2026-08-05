#ifndef HARNESS_PGN_H
#define HARNESS_PGN_H

#include "game.h"
#include "platform.h"

/*
    PGN output. Every move an engine chose carries a comment with the score it
    reported, the depth it reached and the time it spent, which is what makes a
    lost game diagnosable afterwards. Book moves carry no comment.
*/

typedef struct {
    void *file;
    Mutex lock;
    int open;
} PgnWriter;

int  pgnOpen(PgnWriter *writer, const char *path);
void pgnClose(PgnWriter *writer);

void pgnWriteGame(PgnWriter *writer, const GameRecord *record,
                  const char *whiteName, const char *blackName,
                  int round, const char *event, const char *timeControl);

#endif // HARNESS_PGN_H
