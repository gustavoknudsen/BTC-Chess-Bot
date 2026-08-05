#ifndef HARNESS_BOOK_H
#define HARNESS_BOOK_H

/*
    Opening book.

    An opening is a starting position plus the moves that lead to it. EPD books
    give the position directly; PGN books give a start position and the first
    few plies of a game.

    Books can be very large, so loading is capped. When a file holds more
    openings than the cap and a seed is given, the loader takes a uniform
    random sample of the whole file rather than its first lines, which keeps a
    truncated load representative.
*/

#define BOOK_MOVES_MAX 32

typedef struct {
    char fen[92];
    char moves[BOOK_MOVES_MAX][6];
    int  moveCount;
} Opening;

typedef struct {
    Opening *openings;
    int count;
    int capacity;
    long long seen;      // openings the file offered, before the cap
} Book;

void bookInit(Book *book);
void bookFree(Book *book);

// both return the number of openings kept, or -1 if the file cannot be read
int  bookLoadEpd(Book *book, const char *path, int limit, unsigned long long seed);
int  bookLoadPgn(Book *book, const char *path, int plies, int limit, unsigned long long seed);

// deterministic shuffle, so a seed reproduces a run exactly
void bookShuffle(Book *book, unsigned long long seed);

// falls back to the standard start position when the book is empty
void bookOpening(const Book *book, int index, Opening *out);

#endif // HARNESS_BOOK_H
