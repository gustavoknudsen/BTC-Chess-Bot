#include "init.h"
#include "defs.h"
#include "attacks.h"
#include "position.h"
#include "move.h"
#include "zobrist.h"
#include "evaluation.h"
#include "tt.h"

// initialise all necessary variables
void initAll()
{
    // init leapers attacks
    initLeapersAttacks();

    // init slider attacks
    initSlidersAttacks(1);
    initSlidersAttacks(0);

    // init char pieces
    initCharPieces();

    // init piece promotion keys
    initPromotedPieces();

    // init evaluation masks

    initEvalMasks();
    printf("init eval done\n");

    initPositionCache();
    printf("initPositionCache done\n");


    // init random keys (for hashing)
    initRandomKeys();

    // init transposition table with 12MB
    initTT(12);

    /*
    init magic numbers (naturally not on but can be used as well if
    you want to generate them from scratch)
    */
    // initMagicNumbers();

}
