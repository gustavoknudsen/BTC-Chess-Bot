#include "see.h"

// SEE piece values. king is huge so the swap loop never breaks on king
// material; the dedicated king branch in seeGe handles legality. these
// values mirror typical midgame piece values and are deliberately independent
// of the tunable eval material tables so SEE stays stable across eval tuning.
const int seePieceValue[12] = {
    100,    // P
    305,    // N
    333,    // B
    563,    // R
    950,    // Q
    32000,  // K
    100,    // p
    305,    // n
    333,    // b
    563,    // r
    950,    // q
    32000   // k
};
