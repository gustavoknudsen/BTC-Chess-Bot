#include "pgn.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

int pgnOpen(PgnWriter *writer, const char *path)
{
    memset(writer, 0, sizeof(PgnWriter));

    if (!path || !path[0])
        return 1;

    FILE *file = fopen(path, "a");
    if (!file)
        return 0;

    writer->file = file;
    writer->open = 1;
    mutexInit(&writer->lock);
    return 1;
}

void pgnClose(PgnWriter *writer)
{
    if (!writer->open)
        return;

    fclose((FILE *)writer->file);
    mutexDestroy(&writer->lock);
    writer->open = 0;
    writer->file = NULL;
}

static const char *resultText(int result)
{
    if (result == RES_WHITE_WINS) return "1-0";
    if (result == RES_BLACK_WINS) return "0-1";
    return "1/2-1/2";
}

void pgnWriteGame(PgnWriter *writer, const GameRecord *record,
                  const char *whiteName, const char *blackName,
                  int round, const char *event, const char *timeControl)
{
    if (!writer->open)
        return;

    Position pos;
    boardParseFen(&pos, record->startFen);

    int moveNumber = pos.fullmove;
    int side = pos.side;

    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char date[16];
    strftime(date, sizeof(date), "%Y.%m.%d", local);

    mutexLock(&writer->lock);

    FILE *file = (FILE *)writer->file;

    fprintf(file, "[Event \"%s\"]\n", event);
    fprintf(file, "[Site \"local\"]\n");
    fprintf(file, "[Date \"%s\"]\n", date);
    fprintf(file, "[Round \"%d\"]\n", round);
    fprintf(file, "[White \"%s\"]\n", whiteName);
    fprintf(file, "[Black \"%s\"]\n", blackName);
    fprintf(file, "[Result \"%s\"]\n", resultText(record->result));

    if (strcmp(record->startFen, START_FEN) != 0)
    {
        fprintf(file, "[SetUp \"1\"]\n");
        fprintf(file, "[FEN \"%s\"]\n", record->startFen);
    }

    if (timeControl && timeControl[0])
        fprintf(file, "[TimeControl \"%s\"]\n", timeControl);

    fprintf(file, "[PlyCount \"%d\"]\n", record->moveCount);
    fprintf(file, "[Termination \"%s\"]\n", record->reason);
    fprintf(file, "\n");

    int column = 0;

    for (int i = 0; i < record->moveCount; i++)
    {
        const GameMove *move = &record->moves[i];
        char text[96];
        int length = 0;

        if (side == WHITE)
            length += snprintf(text + length, sizeof(text) - length, "%d. ", moveNumber);
        else if (i == 0)
            length += snprintf(text + length, sizeof(text) - length, "%d... ", moveNumber);

        length += snprintf(text + length, sizeof(text) - length, "%s", move->san);

        if (i >= record->openingPlies)
        {
            char score[24];

            if (!move->haveScore)
                snprintf(score, sizeof(score), "none");
            else if (move->scoreIsMate)
                snprintf(score, sizeof(score), "%cM%d", move->score >= 0 ? '+' : '-',
                         move->score >= 0 ? move->score : -move->score);
            else
                snprintf(score, sizeof(score), "%+.2f", move->score / 100.0);

            length += snprintf(text + length, sizeof(text) - length, " {%s/%d %.2fs}",
                               score, move->depth, move->timeMs / 1000.0);
        }

        if (column + length > 78)
        {
            fprintf(file, "\n");
            column = 0;
        }
        else if (column)
        {
            fprintf(file, " ");
            column++;
        }

        fprintf(file, "%s", text);
        column += length;

        if (side == BLACK)
            moveNumber++;

        side ^= 1;
    }

    if (column)
        fprintf(file, " ");

    fprintf(file, "%s\n\n", resultText(record->result));
    fflush(file);

    mutexUnlock(&writer->lock);
}
