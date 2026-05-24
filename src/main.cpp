#include "defs.h"
#include "position.h"
#include "search.h"
#include "tt.h"
#include "uci.h"
#include "init.h"
#include "timeman.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Main Function

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

int main()
{
    // init all variabkes
    initAll();

    clearTT();
    clearSearchHeuristics();

    int debug = 0;

    // debug
    if (debug)
    {
        parseFEN("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 25 ");
        printBoard();
        printf("MOVE NUMBER:%d \n", moveNumber);
        printf("FIFTY:%d \n", fifty);

        // Start timer
        int start = getTime();

        // Call the function you want to time
        search(3);

        // End timer
        int end = getTime();

        printf("Time taken by evaluate(): %d ms\n", end - start);

    }
    else
        // GUI
        uciLoop();

    return 0;
}
