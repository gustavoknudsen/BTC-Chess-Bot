#include "book.h"
#include "board.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define START_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

typedef struct {
    int limit;
    unsigned long long state;   // zero means keep the first openings, no sampling
} Sampler;

void bookInit(Book *book)
{
    book->openings = NULL;
    book->count    = 0;
    book->capacity = 0;
    book->seen     = 0;
}

void bookFree(Book *book)
{
    free(book->openings);
    bookInit(book);
}

static unsigned long long nextRandom(unsigned long long *state)
{
    *state ^= *state << 13;
    *state ^= *state >> 7;
    *state ^= *state << 17;
    return *state;
}

/*
    Returns the slot the next opening should be written to, or NULL when it is
    not kept. Below the cap every opening is kept. Above it, opening number n
    replaces a random earlier slot with probability limit/n, which leaves the
    kept set a uniform sample of everything the file offered.
*/
static Opening *bookSlot(Book *book, Sampler *sampler)
{
    book->seen++;

    if (book->count < sampler->limit)
    {
        if (book->count == book->capacity)
        {
            int capacity = book->capacity ? book->capacity * 2 : 1024;
            if (capacity > sampler->limit)
                capacity = sampler->limit;

            Opening *grown = (Opening *)realloc(book->openings,
                                                (size_t)capacity * sizeof(Opening));
            if (!grown)
                return NULL;

            book->openings = grown;
            book->capacity = capacity;
        }

        Opening *opening = &book->openings[book->count++];
        memset(opening, 0, sizeof(Opening));
        return opening;
    }

    if (!sampler->state)
        return NULL;

    unsigned long long pick = nextRandom(&sampler->state) % (unsigned long long)book->seen;
    if (pick >= (unsigned long long)sampler->limit)
        return NULL;

    Opening *opening = &book->openings[pick];
    memset(opening, 0, sizeof(Opening));
    return opening;
}

// EPD carries four fields and then operations; FEN carries six. Normalise to a
// six field FEN so the position command is always well formed.
static int normaliseFen(const char *line, char *out, int outSize)
{
    char fields[6][80];
    int count = 0;

    const char *c = line;

    while (*c && count < 6)
    {
        while (*c == ' ' || *c == '\t') c++;
        if (!*c || *c == ';')
            break;

        int n = 0;
        while (*c && *c != ' ' && *c != '\t' && *c != ';' && n < 79)
            fields[count][n++] = *c++;
        fields[count][n] = '\0';

        if (n)
            count++;
    }

    if (count < 4)
        return 0;

    if (count == 4)
        snprintf(out, outSize, "%s %s %s %s 0 1", fields[0], fields[1], fields[2], fields[3]);
    else if (count == 5)
        snprintf(out, outSize, "%s %s %s %s %s 1", fields[0], fields[1], fields[2],
                 fields[3], fields[4]);
    else
        snprintf(out, outSize, "%s %s %s %s %s %s", fields[0], fields[1], fields[2],
                 fields[3], fields[4], fields[5]);

    return 1;
}

int bookLoadEpd(Book *book, const char *path, int limit, unsigned long long seed)
{
    FILE *file = fopen(path, "r");
    if (!file)
        return -1;

    Sampler sampler;
    sampler.limit = limit > 0 ? limit : 1;
    sampler.state = seed;

    char line[1024];

    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (!line[0] || line[0] == '#')
            continue;

        char fen[92];
        if (!normaliseFen(line, fen, sizeof(fen)))
            continue;

        Position pos;
        if (!boardParseFen(&pos, fen))
            continue;

        Opening *opening = bookSlot(book, &sampler);
        if (!opening)
            continue;

        snprintf(opening->fen, sizeof(opening->fen), "%s", fen);
        opening->moveCount = 0;
    }

    fclose(file);
    return book->count;
}

// strips PGN decorations that are not moves: comments, variations, numeric
// annotation glyphs, move numbers and the result token
static void addPgnGame(Book *book, Sampler *sampler, const char *fen,
                       const char *movetext, int plies)
{
    Position pos;
    if (!boardParseFen(&pos, fen))
        return;

    Opening staged;
    memset(&staged, 0, sizeof(staged));
    snprintf(staged.fen, sizeof(staged.fen), "%s", fen);

    const char *c = movetext;
    int depth = 0;

    while (*c && staged.moveCount < plies && staged.moveCount < BOOK_MOVES_MAX)
    {
        if (*c == '{')
        {
            while (*c && *c != '}') c++;
            if (*c) c++;
            continue;
        }

        if (*c == '(')
        {
            depth++;
            c++;
            continue;
        }

        if (*c == ')')
        {
            if (depth) depth--;
            c++;
            continue;
        }

        if (depth || isspace((unsigned char)*c) || *c == '.' || *c == '$')
        {
            if (*c == '$')
                while (*c && !isspace((unsigned char)*c)) c++;
            else
                c++;
            continue;
        }

        if (isdigit((unsigned char)*c) || *c == '*')
        {
            while (*c && !isspace((unsigned char)*c)) c++;
            continue;
        }

        char token[32];
        int n = 0;
        while (*c && !isspace((unsigned char)*c) && n < (int)sizeof(token) - 1)
            token[n++] = *c++;
        token[n] = '\0';

        Move move;
        if (!boardMoveFromSan(&pos, token, &move))
            break;

        boardMoveToUci(move, staged.moves[staged.moveCount]);
        staged.moveCount++;
        boardMakeMove(&pos, move);
    }

    // a game that gave us no moves is not an opening
    if (!staged.moveCount)
        return;

    Opening *opening = bookSlot(book, sampler);
    if (opening)
        *opening = staged;
}

int bookLoadPgn(Book *book, const char *path, int plies, int limit, unsigned long long seed)
{
    FILE *file = fopen(path, "r");
    if (!file)
        return -1;

    if (plies <= 0 || plies > BOOK_MOVES_MAX)
        plies = 16;

    Sampler sampler;
    sampler.limit = limit > 0 ? limit : 1;
    sampler.state = seed;

    char line[4096];
    char fen[92];
    char *movetext = NULL;
    int movetextLength = 0;
    int movetextCapacity = 0;
    int inMovetext = 0;

    snprintf(fen, sizeof(fen), "%s", START_FEN);

    while (fgets(line, sizeof(line), file))
    {
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '[')
        {
            // a tag after movetext means the previous game is complete
            if (inMovetext)
            {
                if (movetextLength)
                    addPgnGame(book, &sampler, fen, movetext, plies);
                movetextLength = 0;
                inMovetext = 0;
                snprintf(fen, sizeof(fen), "%s", START_FEN);
            }

            if (!strncmp(line, "[FEN ", 5))
            {
                const char *start = strchr(line, '"');
                if (start)
                {
                    start++;
                    const char *end = strchr(start, '"');
                    int length = end ? (int)(end - start) : 0;
                    if (length > 0 && length < (int)sizeof(fen))
                    {
                        memcpy(fen, start, (size_t)length);
                        fen[length] = '\0';
                    }
                }
            }

            continue;
        }

        if (!line[0])
            continue;

        inMovetext = 1;

        int needed = movetextLength + (int)strlen(line) + 2;
        if (needed > movetextCapacity)
        {
            int capacity = needed * 2;
            char *grown = (char *)realloc(movetext, (size_t)capacity);
            if (!grown)
                break;
            movetext = grown;
            movetextCapacity = capacity;
        }

        movetextLength += snprintf(movetext + movetextLength,
                                   (size_t)(movetextCapacity - movetextLength), "%s ", line);
    }

    if (inMovetext && movetextLength)
        addPgnGame(book, &sampler, fen, movetext, plies);

    free(movetext);
    fclose(file);
    return book->count;
}

void bookShuffle(Book *book, unsigned long long seed)
{
    if (book->count < 2)
        return;

    unsigned long long state = seed ? seed : 0x123456789ABCDEFULL;

    for (int i = book->count - 1; i > 0; i--)
    {
        int j = (int)(nextRandom(&state) % (unsigned long long)(i + 1));

        Opening temp = book->openings[i];
        book->openings[i] = book->openings[j];
        book->openings[j] = temp;
    }
}

void bookOpening(const Book *book, int index, Opening *out)
{
    if (!book->count)
    {
        memset(out, 0, sizeof(Opening));
        snprintf(out->fen, sizeof(out->fen), "%s", START_FEN);
        return;
    }

    *out = book->openings[index % book->count];
}
