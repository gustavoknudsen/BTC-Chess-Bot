#ifndef COPY_MAKE_H
#define COPY_MAKE_H

#include "defs.h"
#include "position.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

               Copy Make Functions
                    & Macros

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

#define copyBoard()                                                     \
U64 bitboardsCopy[12], occupanciesCopy[3], pieceAttackTablesCopy[2][7], pawnDoubleTablesCopy[2];           \
U64 weakCopy[2], stronglyProtectedCopy[2], safeCopy[2], defendedCopy[2], attackedBy2Copy[2], nonPawnEnemiesCopy[2], pawnSpansCopy[2];                                           \
U64 mobilityAreaWhiteCopy, mobilityAreaBlackCopy;                       \
int sideCopy, enpassantCopy, castleCopy, fiftyCopy, moveNumberCopy;     \
memcpy(bitboardsCopy, bitboards, 96);                                   \
memcpy(occupanciesCopy, occupancies, 24);                               \
memcpy(pieceAttackTablesCopy, pieceAttackTables, 112);                  \
memcpy(pawnDoubleTablesCopy, pawnDoubleTables, 16);                     \
memcpy(weakCopy, weak, 16);                                            \
memcpy(stronglyProtectedCopy, stronglyProtected, 16);                   \
memcpy(safeCopy, safe, 16);                                            \
memcpy(defendedCopy, defended, 16);                                    \
memcpy(attackedBy2Copy, attackedBy2, 16);                              \
memcpy(nonPawnEnemiesCopy, nonPawnEnemies, 16);                        \
memcpy(pawnSpansCopy, pawnSpans, 16);                                  \
mobilityAreaWhiteCopy = mobilityAreaWhite;                              \
mobilityAreaBlackCopy = mobilityAreaBlack;                              \
sideCopy = side, enpassantCopy = enpassant, castleCopy = castle;        \
fiftyCopy = fifty;                                                      \
moveNumberCopy = moveNumber;                                            \
U64 hashKeyCopy = hashKey;                                              \
U64 positionCacheHashCopy = positionCache.positionHash;

#define undoBoard()                                                      \
memcpy(bitboards, bitboardsCopy, 96);                                    \
memcpy(occupancies, occupanciesCopy, 24);                                \
memcpy(pieceAttackTables, pieceAttackTablesCopy, 112);                   \
memcpy(pawnDoubleTables, pawnDoubleTablesCopy, 16);                      \
memcpy(weak, weakCopy, 16);                                             \
memcpy(stronglyProtected, stronglyProtectedCopy, 16);                    \
memcpy(safe, safeCopy, 16);                                             \
memcpy(defended, defendedCopy, 16);                                     \
memcpy(attackedBy2, attackedBy2Copy, 16);                               \
memcpy(nonPawnEnemies, nonPawnEnemiesCopy, 16);                         \
memcpy(pawnSpans, pawnSpansCopy, 16);                                   \
mobilityAreaWhite = mobilityAreaWhiteCopy;                               \
mobilityAreaBlack = mobilityAreaBlackCopy;                               \
side = sideCopy; enpassant = enpassantCopy, castle = castleCopy;         \
fifty = fiftyCopy;                                                       \
moveNumber = moveNumberCopy;                                             \
hashKey = hashKeyCopy;                                                   \
positionCache.positionHash = positionCacheHashCopy;

#endif // COPY_MAKE_H
