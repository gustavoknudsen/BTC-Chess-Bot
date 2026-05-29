#include "tt.h"
#include "bitboard.h"

// number of hash entries
int hashEntries = 0;

// number of buckets (hashEntries / TT_BUCKET_SIZE)
int hashBuckets = 0;

// search generation, bumped per search by the search driver
int ttGeneration = 0;

// create transposition table
tt *transpositionTable = NULL;

// clear table
void clearTT()
{
    // get tt pointer to current hash entry
    tt *hashEntry;

    // loop over all entries
    for (hashEntry = transpositionTable; hashEntry < transpositionTable + hashEntries; hashEntry++)
    {
        // reset all tt fields
        hashEntry->hashKey = 0;
        hashEntry->depth = 0;
        hashEntry->flag = 0;
        hashEntry->score = 0;
        hashEntry->move = 0;
        hashEntry->best = 0;
        hashEntry->age = 0;
    }

    ttGeneration = 0;
}

// dynamically allocate memory for tt
void initTT(int mb)
{
    // hash size
    int hashSize = 0x100000 * mb;

    // get hash entries, rounded down to a whole number of buckets
    hashEntries = hashSize / sizeof(tt);
    hashEntries -= hashEntries % TT_BUCKET_SIZE;
    if (hashEntries < TT_BUCKET_SIZE) hashEntries = TT_BUCKET_SIZE;
    hashBuckets = hashEntries / TT_BUCKET_SIZE;

    // clear hash memory (initialised before)
    if (transpositionTable != NULL)
    {
        printf("    Clearing hash memory\n");

        // free memory
        free(transpositionTable);
    }

    // allocate memory
    transpositionTable = (tt *) malloc(hashEntries * sizeof(tt));

    // if allocation failed
    if (transpositionTable == NULL)
    {
        printf(" Failed to allocate memory, tryinr %dMB\n", mb/2);

        // try half size
        initTT(mb/2);
    }
    else // if allocation successful
    {
        // clear table
        clearTT();

    }
}

// hash function not for search and tt
static int getPositionHash() {
    // Simple hash function - you might want to use your existing hash if available
    int hash = 0;

    for (int piece = P; piece <= k; piece++) {
        U64 bb = bitboards[piece];
        while (bb) {
            int square = getLSFBIndex(bb);
            hash ^= (piece * 64 + square);
            popBit(bb, square);
        }
    }

    hash ^= side; // Include side to move in hash

    return hash;
}
