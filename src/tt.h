#ifndef TT_H
#define TT_H

#include "defs.h"
#include "position.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

             Transposition Table
           (using Zobrist Hashing)

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// number of hash entries
extern int hashEntries;

// transposition table structure
typedef struct
{
    U64 hashKey;
    int depth; // depth of move
    int flag; // (if failed high, low or PV node)
    int score; // eval
    int move; // move
    int best; // best move in position
} tt;

// create transposition table
extern tt *transpositionTable;

// clear table
void clearTT();

// dynamically allocate memory for tt
void initTT(int mb);

// PLY variable used by search/tt is in position.h (ply)

// read (probe) hash
static inline int probeHash(int alpha, int beta, int depth, int* best)
{
    // create TT pointer (points to the element of the tt)
    // since keys are stored using modulo, % should be used to avoid collisions
    tt *hashEntry = &transpositionTable[hashKey % hashEntries];

    // check if exact position
    if (hashEntry->hashKey == hashKey)
    {
        // check if the same depth
        if (hashEntry->depth >= depth)
        {
            // get stored score from TT
            int score = hashEntry->score;

            // adjust score for mate scores
            if (score < -mateScore) score += ply;
            if (score > mateScore) score -= ply;

            // match the flag
            // PV node
            if (hashEntry->flag == hashFlagExact)
            {
                // return pv score
                return score;
            }
            // alpha score (fail low)
            if ((hashEntry->flag == hashFlagAlpha) && (score <= alpha))
            {
                // return alpha
                return alpha;
            }
            // beta score (fail high)
            if ((hashEntry->flag == hashFlagBeta) && (score >= beta))
            {
                // return beta
                return beta;
            }
        }

        // store best move
        *best = hashEntry->best;
    }
    // if no match
    return noHashEntry;
}

// write (record) hash
static inline void recordHash(int score, int depth, int hashFlag, int move, int best)
{
    // create TT pointer (points to the element of the tt)
    // since keys are stored using modulo, % should be used to avoid collisions
    tt *hashEntry = &transpositionTable[hashKey % hashEntries];

    // adjust score (for mating scores)
    if (score < -mateScore) score -= ply;
    if (score > mateScore) score += ply;

    // write hash entry
    hashEntry->hashKey = hashKey;
    hashEntry->score = score;
    hashEntry->depth = depth;
    hashEntry->flag = hashFlag;
    hashEntry->move = move;
    hashEntry->best = best;
}

#endif // TT_H
