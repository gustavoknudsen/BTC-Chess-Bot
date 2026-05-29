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

// number of hash entries (total slots, a multiple of TT_BUCKET_SIZE)
extern int hashEntries;

// number of buckets. each bucket holds TT_BUCKET_SIZE entries.
extern int hashBuckets;

// search generation, bumped once per search. used by the bucket replacement
// policy so stale entries from earlier searches are preferred for eviction.
extern int ttGeneration;

// entries per bucket. probing scans all entries of one bucket; on a full
// bucket the lowest depth - 8 * (gen - age) entry is replaced.
#define TT_BUCKET_SIZE 4

// transposition table structure
typedef struct
{
    U64 hashKey;
    int depth; // depth of move
    int flag; // (if failed high, low or PV node)
    int score; // eval
    int move; // move
    int best; // best move in position
    int age; // ttGeneration at the time this entry was written
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

    // base of this position's bucket
    tt *bucket = &transpositionTable[(hashKey % hashBuckets) * TT_BUCKET_SIZE];

    // scan the bucket for a key match
    for (int i = 0; i < TT_BUCKET_SIZE; i++)
    {
        tt *hashEntry = &bucket[i];

        if (hashEntry->hashKey != hashKey)
            continue;

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
                return score;
            // alpha score (fail low)
            if ((hashEntry->flag == hashFlagAlpha) && (score <= alpha))
                return alpha;
            // beta score (fail high)
            if ((hashEntry->flag == hashFlagBeta) && (score >= beta))
                return beta;
        }

        return noHashEntry;
    }

    // if no match in the bucket
    return noHashEntry;
}

// write (record) hash
static inline void recordHash(int score, int depth, int hashFlag, int move, int best)
{
    // base of this position's bucket
    tt *bucket = &transpositionTable[(hashKey % hashBuckets) * TT_BUCKET_SIZE];

    // pick a slot: prefer a key match or an empty slot; otherwise evict the
    // entry that minimises depth - 8 * (gen - age) (shallow and/or stale).
    tt *slot = NULL;
    tt *replace = &bucket[0];
    for (int i = 0; i < TT_BUCKET_SIZE; i++)
    {
        tt *hashEntry = &bucket[i];
        if (hashEntry->hashKey == hashKey || hashEntry->hashKey == 0)
        {
            slot = hashEntry;
            break;
        }
        if (hashEntry->depth - 8 * (ttGeneration - hashEntry->age) <
            replace->depth - 8 * (ttGeneration - replace->age))
            replace = hashEntry;
    }
    if (slot == NULL)
        slot = replace;

    // depth-preferred: keep a deeper same-key entry unless this result is exact
    // or not much shallower. preserve at least the move in that case.
    if (slot->hashKey == hashKey && hashFlag != hashFlagExact && depth < slot->depth - 3)
    {
        if (move) { slot->move = move; slot->best = best; }
        slot->age = ttGeneration;
        return;
    }

    // adjust score (for mating scores)
    if (score < -mateScore) score -= ply;
    if (score > mateScore) score += ply;

    // write hash entry
    slot->hashKey = hashKey;
    slot->score = score;
    slot->depth = depth;
    slot->flag = hashFlag;
    slot->move = move;
    slot->best = best;
    slot->age = ttGeneration;
}

#endif // TT_H
