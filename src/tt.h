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

// read (probe) hash.
// out params expose the raw entry so callers (e.g., singular extensions) can
// look at the stored score/depth/flag even when the entry was too shallow to
// produce a cutoff, or when the caller wants the raw score rather than the
// alpha/beta-clamped cutoff value.
//   ttHit   - 1 if hashKey matched, 0 otherwise
//   ttDepth - stored search depth (valid only when ttHit)
//   ttFlag  - stored bound flag (valid only when ttHit)
//   ttScore - stored score, mate-adjusted to current ply (valid only when ttHit)
static inline int probeHash(int alpha, int beta, int depth, int* best,
                            int* ttHit, int* ttDepth, int* ttFlag, int* ttScore)
{
    *ttHit   = 0;
    *ttDepth = 0;
    *ttFlag  = 0;
    *ttScore = 0;

    // create TT pointer (points to the element of the tt)
    // since keys are stored using modulo, % should be used to avoid collisions
    tt *hashEntry = &transpositionTable[hashKey % hashEntries];

    // check if exact position
    if (hashEntry->hashKey == hashKey)
    {
        // raw entry info, mate-adjusted to current ply, exposed unconditionally
        int rawScore = hashEntry->score;
        if (rawScore < -mateScore) rawScore += ply;
        if (rawScore >  mateScore) rawScore -= ply;

        *ttHit   = 1;
        *ttDepth = hashEntry->depth;
        *ttFlag  = hashEntry->flag;
        *ttScore = rawScore;

        // store best move (overwrites caller's bestMove if hashKey matches)
        *best = hashEntry->best;

        // check if the same depth
        if (hashEntry->depth >= depth)
        {
            int score = rawScore;

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
