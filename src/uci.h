#ifndef UCI_H
#define UCI_H

#include "defs.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                UCI Protocol
          Thanks to Richard Allbert

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// adjust for delay (like in lichess)
extern int moveOverhead;

extern int slowMover;   // percentage (100 = normal speed)

// parse user or GUI string input (like e4e5 or h7h8q for example)
int parseMove(const char* moveString);

/*
    UCI 'position' protocols that have to be accounted for

    start position
    position startpos

    start position + moves:
    position startpos moves e2e4 e7e5

    position from FEN string:
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1

    position from fen string + moves:
    position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 moves e2a6 e8g8
*/
// parse the UCI 'position' protocol
void parsePosition(const char* command);

// reset time variables
void resetTimeControl();

// parse the UCI 'go' protocol
void parseGo(const char *command);

// UCI loop for communication
void uciLoop();

#endif // UCI_H
