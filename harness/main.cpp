#include "options.h"
#include "match.h"
#include "board.h"
#include "book.h"
#include "pgn.h"
#include "log.h"
#include "sprt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>

static int failures = 0;

static void check(int condition, const char *what)
{
    if (condition)
        return;

    printf("  FAIL  %s\n", what);
    failures++;
}

typedef struct {
    const char *fen;
    int depth;
    unsigned long long nodes;
} PerftCase;

// canonical node counts. these validate the arbiter's move generation
// independently of the engine under test, which is the whole point of the
// arbiter having its own.
static const PerftCase perftCases[] = {
    { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281 },
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862 },
    { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624 },
    { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333 },
    { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379 },
    { "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890 }
};

// every legal move is written as SAN and read back, and the same for UCI. a
// disagreement means the PGN output or the PGN book reader would corrupt games.
static void notationRoundTrip(const Position *pos, int depth)
{
    Move list[MAX_MOVES];
    int count = boardGenerateLegal(pos, list);

    for (int i = 0; i < count; i++)
    {
        char san[16], uci[8];
        boardMoveToSan(pos, list[i], san, sizeof(san));
        boardMoveToUci(list[i], uci);

        Move fromSan, fromUci;

        if (!boardMoveFromSan(pos, san, &fromSan) ||
            fromSan.from != list[i].from || fromSan.to != list[i].to ||
            fromSan.promo != list[i].promo)
        {
            printf("  FAIL  san round trip on %s\n", san);
            failures++;
        }

        if (!boardMoveFromUci(pos, uci, &fromUci) ||
            fromUci.from != list[i].from || fromUci.to != list[i].to ||
            fromUci.promo != list[i].promo)
        {
            printf("  FAIL  uci round trip on %s\n", uci);
            failures++;
        }

        if (depth > 1)
        {
            Position next = *pos;
            boardMakeMove(&next, list[i]);
            notationRoundTrip(&next, depth - 1);
        }
    }
}

static void testBoard(void)
{
    printf("arbiter move generation\n");

    unsigned long long total = 0;
    double start = timeNowMs();

    for (int i = 0; i < (int)(sizeof(perftCases) / sizeof(perftCases[0])); i++)
    {
        Position pos;
        check(boardParseFen(&pos, perftCases[i].fen) == 1, "fen parses");

        unsigned long long nodes = boardPerft(&pos, perftCases[i].depth);
        total += nodes;

        printf("  perft depth %d  %12llu  %s\n", perftCases[i].depth, nodes,
               nodes == perftCases[i].nodes ? "ok" : "WRONG");

        if (nodes != perftCases[i].nodes)
        {
            printf("        expected %llu for %s\n", perftCases[i].nodes, perftCases[i].fen);
            failures++;
        }
    }

    double elapsed = timeNowMs() - start;
    printf("  %llu nodes in %.0f ms\n", total, elapsed);

    printf("fen round trip\n");
    for (int i = 0; i < (int)(sizeof(perftCases) / sizeof(perftCases[0])); i++)
    {
        Position pos, again;
        char fen[128];

        boardParseFen(&pos, perftCases[i].fen);
        boardWriteFen(&pos, fen, sizeof(fen));
        check(boardParseFen(&again, fen) == 1, "written fen parses");
        check(again.key == pos.key, "written fen gives the same position");
    }

    printf("notation round trip\n");
    for (int i = 0; i < 3; i++)
    {
        Position pos;
        boardParseFen(&pos, perftCases[i].fen);
        notationRoundTrip(&pos, 3);
    }

    printf("draw detection\n");
    {
        Position pos;

        boardParseFen(&pos, "8/8/8/4k3/8/8/4K3/8 w - - 0 1");
        check(boardInsufficientMaterial(&pos) == 1, "king against king is a draw");

        boardParseFen(&pos, "8/8/8/4k3/8/8/4KB2/8 w - - 0 1");
        check(boardInsufficientMaterial(&pos) == 1, "king and bishop is a draw");

        boardParseFen(&pos, "8/8/8/4k3/8/8/4KN2/8 w - - 0 1");
        check(boardInsufficientMaterial(&pos) == 1, "king and knight is a draw");

        boardParseFen(&pos, "8/8/8/4k3/8/8/4KR2/8 w - - 0 1");
        check(boardInsufficientMaterial(&pos) == 0, "king and rook is not a draw");

        boardParseFen(&pos, "8/4p3/8/4k3/8/8/4K3/8 w - - 0 1");
        check(boardInsufficientMaterial(&pos) == 0, "a pawn is not a draw");

        // fool's mate, with black to move and no legal reply
        boardParseFen(&pos, "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3");
        Move list[MAX_MOVES];
        check(boardGenerateLegal(&pos, list) == 0, "checkmate has no legal moves");
        check(boardInCheck(&pos, WHITE) == 1, "checkmated side is in check");

        boardParseFen(&pos, "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
        check(boardGenerateLegal(&pos, list) == 0, "stalemate has no legal moves");
        check(boardInCheck(&pos, BLACK) == 0, "stalemated side is not in check");
    }
}

static void testStatistics(void)
{
    printf("statistics\n");

    check(fabs(eloToScore(0.0) - 0.5) < 1e-12, "zero Elo is an even score");
    check(fabs(scoreToElo(eloToScore(37.5)) - 37.5) < 1e-9, "Elo and score invert");
    check(eloToScore(100.0) > eloToScore(0.0), "more Elo is a better score");

    SprtConfig config;
    sprtConfigure(&config, 0.0, 5.0, 0.05, 0.05);

    check(fabs(config.upperBound - log(0.95 / 0.05)) < 1e-12, "upper bound is Wald's");
    check(fabs(config.lowerBound - log(0.05 / 0.95)) < 1e-12, "lower bound is Wald's");

    // an even result sits just below the midpoint of the two hypotheses, so
    // the likelihood ratio must be slightly negative. this value is the closed
    // form worked out by hand for these counts.
    Stats even;
    statsInit(&even);
    even.pentanomial[0] = 1;
    even.pentanomial[2] = 8;
    even.pentanomial[4] = 1;
    even.wins = even.losses = 5;
    even.draws = 10;

    double llr = sprtLLR(&even, &config);
    check(fabs(llr + 0.005175) < 1e-4, "llr matches the hand computed value");

    // a clear win must drive the ratio up, a clear loss down
    Stats winning;
    statsInit(&winning);
    for (int i = 0; i < 40; i++)
        statsAddPair(&winning, 1, 0);

    Stats losing;
    statsInit(&losing);
    for (int i = 0; i < 40; i++)
        statsAddPair(&losing, -1, 0);

    check(sprtLLR(&winning, &config) > config.upperBound, "a one sided win accepts H1");
    check(sprtLLR(&losing, &config) < config.lowerBound, "a one sided loss accepts H0");

    check(sprtVerdict(config.upperBound + 0.1, &config) == SPRT_H1_ACCEPTED, "upper bound accepts H1");
    check(sprtVerdict(config.lowerBound - 0.1, &config) == SPRT_H0_ACCEPTED, "lower bound accepts H0");
    check(sprtVerdict(0.0, &config) == SPRT_CONTINUE, "an undecided ratio continues");

    // pairing is the reason for the pentanomial: the same games scored as
    // pairs carry less variance than scored one by one, so the test decides
    // sooner
    Stats paired;
    statsInit(&paired);
    for (int i = 0; i < 100; i++)
    {
        statsAddPair(&paired, 1, -1);   // balanced pairs, no information
        statsAddPair(&paired, 1, 0);
    }

    Summary summary;
    summarise(&paired, &config, &summary);
    check(summary.games == 400, "every game is counted");
    check(summary.pairs == 200, "pairs are counted");
    check(summary.elo > 0.0, "a winning score gives positive Elo");
    check(summary.los > 0.5, "a winning score is likely superior");
}

static void testBook(void)
{
    printf("opening book\n");

    const char *path = "harness_selftest_book.epd";
    FILE *file = fopen(path, "w");
    if (!file)
    {
        printf("  FAIL  cannot write %s\n", path);
        failures++;
        return;
    }

    fprintf(file, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -\n");
    fprintf(file, "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3\n");
    fprintf(file, "# a comment line\n");
    fprintf(file, "not a position at all\n");
    fclose(file);

    Book book;
    bookInit(&book);
    check(bookLoadEpd(&book, path, 1000, 0) == 2, "two usable openings are read");

    Opening opening;
    bookOpening(&book, 0, &opening);
    Position pos;
    check(boardParseFen(&pos, opening.fen) == 1, "the opening fen is usable");

    bookFree(&book);
    remove(path);
}

static int runSelfTest(void)
{
    printf("harness self test\n\n");

    testBoard();
    testStatistics();
    testBook();

    printf("\n%s\n", failures ? "SELF TEST: FAIL" : "SELF TEST: PASS");
    return failures ? 1 : 0;
}

static void handleInterrupt(int signal)
{
    (void)signal;
    matchInterrupt();
    printf("\ninterrupted, finishing the games in flight\n");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    boardInit();

    if (argc < 2)
    {
        optionsUsage();
        return 1;
    }

    Options options;
    if (!optionsParse(&options, argc, argv))
        return 1;

    if (options.selfTest)
        return runSelfTest();

    Book book;
    bookInit(&book);

    if (options.bookPath[0])
    {
        // a seed only matters when the book is bigger than the cap: it decides
        // whether the kept openings are a sample or simply the first ones
        unsigned long long sampleSeed = 0;
        if (options.bookRandom)
            sampleSeed = options.seed ? options.seed : 0x2545F4914F6CDD1DULL;

        printf("reading %s\n", options.bookPath);

        int loaded = options.bookFormat == BOOK_PGN
                   ? bookLoadPgn(&book, options.bookPath, options.bookPlies,
                                 options.bookLimit, sampleSeed)
                   : bookLoadEpd(&book, options.bookPath, options.bookLimit, sampleSeed);

        if (loaded < 0)
        {
            fprintf(stderr, "cannot read the opening book '%s'\n", options.bookPath);
            return 1;
        }

        if (loaded == 0)
        {
            fprintf(stderr, "the opening book '%s' held no usable positions\n", options.bookPath);
            return 1;
        }

        if (options.bookRandom)
            bookShuffle(&book, options.seed);
    }

    Logger logger;
    if (!logOpen(&logger, options.logPath))
    {
        fprintf(stderr, "cannot open the log file '%s'\n", options.logPath);
        return 1;
    }

    PgnWriter pgn;
    if (!pgnOpen(&pgn, options.pgnPath))
    {
        fprintf(stderr, "cannot open the pgn file '%s'\n", options.pgnPath);
        return 1;
    }

    Logger traceLog;
    if (!logOpen(&traceLog, options.tracePath))
    {
        fprintf(stderr, "cannot open the trace file '%s'\n", options.tracePath);
        return 1;
    }

    if (options.tracePath[0])
        engineSetTrace(&traceLog);

    Tournament tournament;
    memset(&tournament, 0, sizeof(Tournament));
    tournament.options = &options;
    tournament.book    = &book;
    tournament.pgn     = &pgn;
    tournament.logger  = &logger;

    signal(SIGINT, handleInterrupt);

    int status = matchRun(&tournament);

    pgnClose(&pgn);
    logClose(&traceLog);
    logClose(&logger);
    bookFree(&book);

    return status;
}
