/*
 * Lightweight perft test driver.
 *
 * Runs the existing perftTest() entry point on a handful of well-known
 * positions and prints node counts. The numeric results can be diffed
 * against the original single-file engine to verify that move generation
 * and make/unmake behave identically after the refactor.
 *
 * This file replaces main() from the engine when linked, so build it
 * without src/main.cpp (see the Makefile's perft target).
 */

#include "../src/defs.h"
#include "../src/init.h"
#include "../src/position.h"
#include "../src/perft.h"
#include "../src/timeman.h"

int main(int argc, char **argv)
{
    initAll();

    int depth = (argc >= 2) ? atoi(argv[1]) : 4;

    const char* positions[] = {
        start_position,
        tricky_position,
        killer_position,
        cmk_position,
    };
    const char* names[] = {
        "startpos",
        "tricky_position",
        "killer_position",
        "cmk_position",
    };

    int n = (int)(sizeof(positions) / sizeof(positions[0]));
    for (int i = 0; i < n; i++)
    {
        printf("\n=== position: %s, depth: %d ===\n", names[i], depth);
        parseFEN(positions[i]);
        nodes = 0;
        counter = 0;
        perftTest(depth);
    }

    return 0;
}
