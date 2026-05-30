#ifndef EVALUATION_H
#define EVALUATION_H

#include "defs.h"
#include "bitboard.h"
#include "attacks.h"
#include "position.h"
#include "eval_constants.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                   Evaluation

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// create attack info struct
struct AttackInfo {
    int numberAttackers;
    int valueAttacks;
    int numberAttacks;
};

// create attack info struct
struct kingShelter {
    int mgBonus;
    int egBonus;
};

// set file or rank mask
U64 setFileOrRankMask(int file, int rank);

void initAdjacentFilesMasks();

// initialise evaluation masks
void initEvalMasks();

static inline void initAttacksTotal()
{
    // reset current tables
    memset(pieceAttackTables, 0, 112);
    memset(pawnDoubleTables, 0, 16);  // Clear pawnDoubleTables
    memset(attackedBy2, 0, 16);
    memset(defended, 0, 16);
    memset(safe, 0, 16);
    memset(weak, 0, 16);
    memset(stronglyProtected, 0, 16);
    memset(nonPawnEnemies, 0, 16);

    // pass bitboards for double+ attacks [side][pawn or total = 1]
    U64 firstPass[2][2] = {0};
    U64 secondPass[2][2] = {0};

    // Track squares attacked by pawns to detect double attacks
    U64 pawnFirstAttack[2] = {0, 0};      // Squares attacked once by pawns
    U64 pawnDoubleAttack[2] = {0, 0};     // Squares attacked twice+ by pawns

    // Calculate pawn spans (potential pawn attacks)
    // For white pawns
    U64 whitePawns = bitboards[P];
    while (whitePawns) {
        int sq = getLSFBIndex(whitePawns);

        // Get all squares in front of this pawn
        U64 forward = 0ULL;
        int rank = sq / 8;
        int file = sq % 8;

        // Generate all squares in front of this pawn
        for (int r = rank - 1; r >= 0; r--) {
            int forwardSq = r * 8 + file;

            // Add potential attack squares from this position
            if (file > 0) // Can attack to the left
                pawnSpans[white] |= (1ULL << (forwardSq - 1));
            if (file < 7) // Can attack to the right
                pawnSpans[white] |= (1ULL << (forwardSq + 1));
        }

        popBit(whitePawns, sq);
    }

    // For black pawns
    U64 blackPawns = bitboards[p];
    while (blackPawns) {
        int sq = getLSFBIndex(blackPawns);

        // Get all squares in front of this pawn
        U64 forward = 0ULL;
        int rank = sq / 8;
        int file = sq % 8;

        // Generate all squares in front of this pawn
        for (int r = rank + 1; r <= 7; r++) {
            int forwardSq = r * 8 + file;

            // Add potential attack squares from this position
            if (file > 0) // Can attack to the left
                pawnSpans[black] |= (1ULL << (forwardSq - 1));
            if (file < 7) // Can attack to the right
                pawnSpans[black] |= (1ULL << (forwardSq + 1));
        }

        popBit(blackPawns, sq);
    }

    // loop over pieces
    for (int piece = P; piece <= k; piece++)
    {
        int side = (piece <= K) ? white : black;

        U64 bitboard = bitboards[piece];
        while (bitboard)
        {
            // get square value
            int square = getLSFBIndex(bitboard);

            // if pawn
            if (piece == P || piece == p)
            {
                int side = (piece == P) ? white : black;

                U64 attacks = pawnAttacks[side][square];

                // Check for double pawn attacks specifically for pawnDoubleTables
                // Squares already attacked once by pawns that are attacked again
                pawnDoubleAttack[side] |= pawnFirstAttack[side] & attacks;
                // Add current attacks to first-attack set
                pawnFirstAttack[side] |= attacks;

                // update piece attack tables
                pieceAttackTables[side][P] |= attacks;

                // Check for double attacks by pawns
                secondPass[side][P] |= firstPass[side][P] & attacks;
                firstPass[side][P] |= attacks;
            }
            // if knight
            else if (piece == N || piece == n)
            {
                int side = (piece == N) ? white : black;

                U64 attacks = knightAttacks[square];

                pieceAttackTables[side][N] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else if (piece == B || piece == b)
            {
                int side = (piece == B) ? white : black;

                U64 attacks = getBishopAttacks(square, occupancies[both]);

                pieceAttackTables[side][B] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else if (piece == R || piece == r)
            {
                int side = (piece == R) ? white : black;

                U64 attacks = getRookAttacks(square, occupancies[both]);

                pieceAttackTables[side][R] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else if (piece == Q || piece == q)
            {
                int side = (piece == Q) ? white : black;

                U64 attacks = getQueenAttacks(square, occupancies[both]);

                pieceAttackTables[side][Q] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }
            else // king
            {
                int side = (piece == K) ? white : black;

                U64 attacks = kingAttacks[square];

                pieceAttackTables[side][K] |= attacks;

                secondPass[side][1] |= firstPass[side][1] & attacks;
                firstPass[side][1] |= attacks;
            }

            // pop current piece/bit
            popBit(bitboard, square);
        }
    }

    // Save pawnDoubleTables - squares attacked by 2+ pawns
    pawnDoubleTables[white] = pawnDoubleAttack[white];
    pawnDoubleTables[black] = pawnDoubleAttack[black];

    // Calculate all pieces attacks for each side
    pieceAttackTables[white][allPieces] = pieceAttackTables[white][P] | pieceAttackTables[white][N] | pieceAttackTables[white][B]
                                        | pieceAttackTables[white][R] | pieceAttackTables[white][Q] | pieceAttackTables[white][K];

    pieceAttackTables[black][allPieces] = pieceAttackTables[black][P] | pieceAttackTables[black][N] | pieceAttackTables[black][B]
                                        | pieceAttackTables[black][R] | pieceAttackTables[black][Q] | pieceAttackTables[black][K];

    // Calculate squares attacked by 2 or more pieces
    attackedBy2[white] = secondPass[white][1] | (pieceAttackTables[white][P] & firstPass[white][1]) | secondPass[white][P];
    attackedBy2[black] = secondPass[black][1] | (pieceAttackTables[black][P] & firstPass[black][1]) | secondPass[black][P];

    // Non-pawn enemies for each side
    nonPawnEnemies[white] = occupancies[black] & ~bitboards[p];
    nonPawnEnemies[black] = occupancies[white] & ~bitboards[P];

    // Strongly protected squares (by pawns or double-attacked and not double-attacked by opponent)
    stronglyProtected[white] = pieceAttackTables[white][P] | (attackedBy2[white] & ~attackedBy2[black]);
    stronglyProtected[black] = pieceAttackTables[black][P] | (attackedBy2[black] & ~attackedBy2[white]);

    // Defended pieces (non-pawn pieces that are strongly protected)
    defended[white] = nonPawnEnemies[white] & stronglyProtected[white];
    defended[black] = nonPawnEnemies[black] & stronglyProtected[black];

    // Weak pieces (enemy pieces not strongly protected and under our attack)
    weak[white] = occupancies[black] & ~stronglyProtected[black] & pieceAttackTables[white][allPieces];
    weak[black] = occupancies[white] & ~stronglyProtected[white] & pieceAttackTables[black][allPieces];

    // Safe squares (not attacked by enemy or defended by us)
    safe[white] = ~pieceAttackTables[black][allPieces] | stronglyProtected[white];
    safe[black] = ~pieceAttackTables[white][allPieces] | stronglyProtected[black];
}

// scale attacks on king based off of attack value and number of attackers
int scaleAttacks(AttackInfo info);

// get number of attackers and attack value on king zone for side (disregarding their own pieces except for their pawns)
AttackInfo getAttackInfo(int kingSide);

// function that evaluates the pawn penalty [file][white or black] (does both pawn storms and pawn shield)
int KingPawnPenalty(int file, int side);

// game phase calculator function
static inline int getGameStageScore()
{
    // white & black game phase scores
    int whitePieceScores = 0; int blackPieceScores = 0;

    // loop over white pieces (except for pawns and kings)
    for (int piece = N; piece <= Q; piece++)
        whitePieceScores += countBits(bitboards[piece]) * materialScore[opening][piece];

    // loop over black pieces (except for pawns and kings)
    for (int piece = n; piece <= q; piece++)
        blackPieceScores += countBits(bitboards[piece]) * -materialScore[opening][piece];

    // return game phase score
    return whitePieceScores + blackPieceScores;
}

// Interpolation function
static inline int interpolate(int openingValue, int endgameValue, int stageScore) {
    return (openingValue * stageScore + endgameValue * (openingPhaseScore - stageScore)) / openingPhaseScore;
}

void initPositionCache();

void calculateExclusionMasks(U64 *regularMask, U64 *queenMask);

void updatePositionCache();

// Get all pinned pieces for the current position
U64 getPinnedPieces(int side);

// Get the pin ray for a specific pinned piece
U64 getPinRay(int kingSquare, int pinnedSquare);

U64 getRayBetween(int square1, int square2);

// get piece mobility
static inline int getMobility(int piece, int squarePiece) {
    // Update cache if needed
    updatePositionCache();

    // Get basic attack bitboard
    U64 mobilityBB = 0;

    // Choose the appropriate exclusion mask
    U64 excludeMask = (piece == Q || piece == q) ?
                      positionCache.queenExcludeMask :
                      positionCache.regularExcludeMask;

    switch (piece) {
        case N: case n:
            mobilityBB = knightAttacks[squarePiece] & ~occupancies[side];
            break;

        case B: case b: {
            // Bishop can look through friendly queen
            U64 friendlyQueens = bitboards[(side == white) ? Q : q];
            mobilityBB = getBishopAttacks(squarePiece, occupancies[both] & ~friendlyQueens) & ~occupancies[side];
            break;
        }

        case R: case r: {
            // Rook can look through friendly queen and other rooks
            U64 friendlyQueensRooks = bitboards[(side == white) ? Q : q] | bitboards[(side == white) ? R : r];
            mobilityBB = getRookAttacks(squarePiece, occupancies[both] & ~friendlyQueensRooks) & ~occupancies[side];
            break;
        }

        case Q: case q:
            // Queen mobility with enemy piece attacks excluded
            mobilityBB = getQueenAttacks(squarePiece, occupancies[both]) & ~occupancies[side];
            break;
    }

    // Apply exclusion mask
    mobilityBB &= ~excludeMask;

    // If piece is pinned, restrict mobility to pin ray
    if (positionCache.pinnedPieces & (1ULL << squarePiece)) {
        mobilityBB &= positionCache.pinnedRays[squarePiece];
    }

    return countBits(mobilityBB);
}

// Function to update mobility areas (call once before evaluation)
static inline void updateMobilityAreas() {
    // Calculate blocked pawns
    U64 whitePawnBlocked = bitboards[P] & (occupancies[both] << 8);
    U64 blackPawnBlocked = bitboards[p] & (occupancies[both] >> 8);

    // Pawns in rank 2 and 3 (for white)
    U64 whitePawnsRank23 = bitboards[P] & (rankMask[a2] | rankMask[a3]);

    // Pawns in rank 6 and 7 (for black)
    U64 blackPawnsRank67 = bitboards[p] & (rankMask[a6] | rankMask[a7]);

    // White mobility area excludes:
    // 1. Squares attacked by enemy pawns
    // 2. Our blocked pawns
    // 3. Our pawns in ranks 2 and 3
    // 4. Our king
    mobilityAreaWhite = ~(pieceAttackTables[black][P] | whitePawnBlocked |
                         whitePawnsRank23 | bitboards[K]);

    // Black mobility area excludes:
    // 1. Squares attacked by enemy pawns
    // 2. Our blocked pawns
    // 3. Our pawns in ranks 6 and 7
    // 4. Our king
    mobilityAreaBlack = ~(pieceAttackTables[white][P] | blackPawnBlocked |
                         blackPawnsRank67 | bitboards[k]);
}


// Ultra-fast simplified mobility calculation with basic mobility areas
static inline int getSimpleMobility(int piece, int squarePiece) {
    U64 mobilityBB = 0;
    int color = (piece >= p) ? black : white;
    U64 relevantArea = (color == white) ? mobilityAreaWhite : mobilityAreaBlack;

    switch (piece) {
        case N: case n:
            mobilityBB = knightAttacks[squarePiece] & relevantArea;
            break;

        case B: case b:
            mobilityBB = getBishopAttacks(squarePiece, occupancies[both]) & relevantArea;
            break;

        case R: case r:
            mobilityBB = getRookAttacks(squarePiece, occupancies[both]) & relevantArea;
            break;

        case Q: case q:
            mobilityBB = getQueenAttacks(squarePiece, occupancies[both]) & relevantArea;
            break;
    }

    return countBits(mobilityBB);
}

// evaluate material only for a side
static inline int evaluateMaterial(int side)
{
    // evaluation score
    int score = 0;

    // copy of the current pieces
    U64 bitboard;

    // get piece and square
    int piece, square, pawn, king;

    if (side == white)
    {
        pawn = P;
        king = K;
    }
    else
    {
        pawn = p;
        king = k;
    }

    // loop over all bits in bitboard
    for (int bitPiece = pawn; bitPiece <= king; bitPiece++)
    {
        // get bitboard copy of the current piece
        bitboard = bitboards[bitPiece];

        // loop over bits in bitboard
        while (bitboard)
        {
            // get piece
            piece = bitPiece;

            // get square
            square = getLSFBIndex(bitboard);

            // score material of the piece
            score += defaultMaterialScore[piece];

            // pop current bit
            popBit(bitboard, square);
        }
    }

    // return final eval based on side
    return (side == white) ? score : -score;
}

// get manhattan distance in chessboard
inline int distance(int s1, int s2)
{
    int file1 = getFile[s1];
    int rank1 = getRank[s1];
    int file2 = getFile[s2];
    int rank2 = getRank[s2];
    return abs(file1 - file2) + abs(rank1 - rank2);
}

// clamp file (return input file unless outside of bounds of the clamp)
inline int clamp(int sq, int file1, int file2)
{
    int sqFile = getFile[sq];
    int file1File = getFile[file1];
    int file2File = getFile[file2];

    if (sqFile < file1File) {
        return file1File;
    } else if (sqFile > file2File) {
        return file2File;
    } else {
        return sqFile;
    }
}

// frontmost square returns the most advanced bit relative to the colour
inline int frontMostSquare(int side, U64 bb)
{
    return (side == white) ? getLSFBIndex(bb) : getMSBIndex(bb);
}

// get relative rank for side
int relativeRank(int side, int rank);

// get king shelter bonus for side
static inline kingShelter getKingShelter(int currentSide, int ksq, kingShelter shelterScore)
{
    // get our pawns and their pawns (not counting pawns behind our king)
    U64 ourPawns = bitboards[(currentSide == white) ? P : p] & ~forwardRanksMasks[!currentSide][ksq];
    U64 theirPawns = bitboards[(currentSide == white) ? p : P] & ~forwardRanksMasks[!currentSide][ksq];

    // initial bonus
    kingShelter bonus;
    bonus.mgBonus = 5;
    bonus.egBonus = 5;

    int fileCenter = getFile[clamp(ksq, b1, g1)];

    U64 b;

    // loop over the file to the left, center, and right
    for (int f = fileCenter - 1; f <= fileCenter + 1; f++)
    {
        // get bb of our pawns and the current file
        b = ourPawns & fileMask[f];
        // get our rank
        int ourRank = b ? relativeRank(currentSide, getRank[frontMostSquare(!currentSide, b)]) : 0;

        // get bb of enemy pawns and the current file
        b = theirPawns & fileMask[f];
        int theirRank = b ? relativeRank(currentSide, getRank[frontMostSquare(!currentSide, b)]) : 0;

        //printf("f: %d, not f: %d\n", f, 7 - f);

        int d = std::min(f, 7 - f);
        //printf("d: %d\n", d);
        bonus.mgBonus += ShelterStrength[d][ourRank];

        if (ourRank && (ourRank == theirRank - 1))
        {
            bonus.mgBonus -= 82 * (theirRank == 2);
            bonus.egBonus -= 82 * (theirRank == 2);
        }
        else
        {
            bonus.mgBonus -= UnblockedStorm[d][theirRank];
        }
    }

    if (bonus.mgBonus > shelterScore.mgBonus)
    {
        return bonus;
    }
    else
    {
        return shelterScore;
    }
}

// get jubg safety
inline kingShelter getKingSafety(int currentSide)
{
    // get king square for side
    int ksq = getLSFBIndex(bitboards[(currentSide == white) ? K : k]);

    // get current castling rights
    int cRights = castle;

    // set min pawn distance to 8 if pawn is non-zero, 0 if no pawns
    U64 pawns = bitboards[(currentSide == white) ? P : p];
    int minPawnDist = pawns ? 8 : 0;

    // if pawn 1 square away from king
    if (pawns & kingAttacks[ksq])
    {
        // min pawn distance is 1
        minPawnDist = 1;
    }

    // else, find min pawn distance
    else while (pawns)
    {
        int sq = getLSFBIndex(pawns);
        int dist = distance(ksq, sq);

        // replace min distance if dist is smaller
        if (dist < minPawnDist)
        {
            minPawnDist = dist;
        }

        // pop bit
        popBit(pawns, sq);
    }

    // evaluate shelter
    kingShelter shelter;
    shelter.mgBonus = -infinity;
    shelter.egBonus = 0;
    // get first shelter score
    shelter = getKingShelter(currentSide, ksq, shelter);

    /*
    // if we can castle use the bonus after the castling
    if (currentSide == white)
    {
        // check king castle is possible
        if (cRights & wk)
        {
            shelter = getKingShelter(currentSide, g1, shelter);
        }
        if (cRights & wq)
        {
            shelter = getKingShelter(currentSide, c1, shelter);
        }
    }
    else // for black
    {
        // check king castle is possible
        if (cRights & bk)
        {
            shelter = getKingShelter(currentSide, g8, shelter);
        }
        if (cRights & bq)
        {
            shelter = getKingShelter(currentSide, c8, shelter);
        }
    }
    */

    shelter.egBonus -= 16 * minPawnDist;

    return shelter;
}

// Stockfish-style king danger. Builds kingDanger as a sum of small contributions
// (attacker weight, weak king-ring squares, safe and unsafe checks, pinned pieces,
// king-flank pressure, no-enemy-queen bonus, knight defense, shelter feedback) and
// transforms it into a quadratic penalty. Returns the mg/eg penalty to subtract from
// side `us`. shelterMg is the mg shelter score already computed by getKingSafety.
// The mobility-differential contribution is omitted until v2.8 mobility lands.
static inline kingShelter getKingDanger(int us, int shelterMg)
{
    int them = 1 - us;
    int ksq = getLSFBIndex(bitboards[(us == white) ? K : k]);
    U64 ownQueen = bitboards[(us == white) ? Q : q];

    U64 ourAll   = pieceAttackTables[us][allPieces];
    U64 theirAll = pieceAttackTables[them][allPieces];

    // Squares around our king the enemy attacks and we defend at most once (king/queen only).
    U64 weakSquares = theirAll
                    & ~attackedBy2[us]
                    & (~ourAll | pieceAttackTables[us][K] | pieceAttackTables[us][Q]);

    // Squares from which an enemy check would be safe.
    U64 safeSquares = ~occupancies[them]
                    & (~ourAll | (weakSquares & attackedBy2[them]));

    // Slider check rays from our king, treating our own queen as transparent.
    U64 occ = occupancies[both] ^ ownQueen;
    U64 rookRays   = getRookAttacks(ksq, occ);
    U64 bishopRays = getBishopAttacks(ksq, occ);

    U64 unsafeChecks = 0ULL;
    int kingDanger = 0;

    // Enemy rook checks
    U64 rookChecks = rookRays & pieceAttackTables[them][R] & safeSquares;
    if (rookChecks)
        kingDanger += SafeCheck[R][countBits(rookChecks) > 1];
    else
        unsafeChecks |= rookRays & pieceAttackTables[them][R];

    // Enemy queen checks, but only from squares that cannot give a rook check
    U64 queenChecks = (rookRays | bishopRays) & pieceAttackTables[them][Q] & safeSquares
                    & ~(pieceAttackTables[us][Q] | rookChecks);
    if (queenChecks)
        kingDanger += SafeCheck[Q][countBits(queenChecks) > 1];

    // Enemy bishop checks, but only from squares that cannot give a queen check
    U64 bishopChecks = bishopRays & pieceAttackTables[them][B] & safeSquares & ~queenChecks;
    if (bishopChecks)
        kingDanger += SafeCheck[B][countBits(bishopChecks) > 1];
    else
        unsafeChecks |= bishopRays & pieceAttackTables[them][B];

    // Enemy knight checks
    U64 knightChecks = knightAttacks[ksq] & pieceAttackTables[them][N];
    if (knightChecks & safeSquares)
        kingDanger += SafeCheck[N][countBits(knightChecks & safeSquares) > 1];
    else
        unsafeChecks |= knightChecks;

    // King ring: king area clamped to b2-g7, minus squares our own two pawns defend.
    int kf = clamp(ksq, b1, g1);
    int krIdx = getRank[ksq];
    if (krIdx < 1) krIdx = 1; else if (krIdx > 6) krIdx = 6;
    int ringSq = (7 - krIdx) * 8 + kf;
    U64 kingRing = (kingAttacks[ringSq] | (1ULL << ringSq)) & ~pawnDoubleTables[us];

    // King-flank pressure
    U64 flank = kingFlankMask[getFile[ksq]] & campMask[us];
    U64 flankAtk = theirAll & flank;
    int kingFlankAttack  = countBits(flankAtk) + countBits(flankAtk & attackedBy2[them]);
    int kingFlankDefense = countBits(ourAll & flank);

    AttackInfo info = getAttackInfo(us);
    int hasEnemyQueen = countBits(bitboards[(us == white) ? q : Q]) > 0;
    int knightDefense = bool(pieceAttackTables[us][N] & pieceAttackTables[us][K]);

    // Pieces (either color) blocking an enemy slider attack on our king (slider-blocker
    // scan inspired by Stockfish: snipers see the king with the board cleared, then a
    // single piece on the squares between king and sniper is a blocker).
    U64 enemyRooksQueens   = bitboards[(us == white) ? r : R] | bitboards[(us == white) ? q : Q];
    U64 enemyBishopsQueens = bitboards[(us == white) ? b : B] | bitboards[(us == white) ? q : Q];
    U64 snipers = (getRookAttacks(ksq, 0ULL) & enemyRooksQueens)
                | (getBishopAttacks(ksq, 0ULL) & enemyBishopsQueens);
    U64 sniperOcc = occupancies[both] ^ snipers;
    U64 blockersBB = 0ULL;
    while (snipers)
    {
        int sniperSq = getLSFBIndex(snipers);
        U64 b = betweenMask[ksq][sniperSq] & sniperOcc;
        if (b && (b & (b - 1)) == 0)
            blockersBB |= b;
        popBit(snipers, sniperSq);
    }
    int blockers = countBits(blockersBB);

    kingDanger += info.numberAttackers * info.valueAttacks
                + 183 * countBits(kingRing & weakSquares)
                + 148 * countBits(unsafeChecks)
                +  98 * blockers
                +  69 * info.numberAttacks
                +   3 * kingFlankAttack * kingFlankAttack / 8
                - 873 * !hasEnemyQueen
                - 100 * knightDefense
                -   6 * shelterMg / 8
                -   4 * kingFlankDefense
                +  37;

    kingShelter penalty;
    penalty.mgBonus = 0;
    penalty.egBonus = 0;

    if (kingDanger > 100)
    {
        penalty.mgBonus = kingDanger * kingDanger / 4096;
        penalty.egBonus = kingDanger / 16;
    }

    // Pawnless king flank (penalty applies regardless of the kingDanger threshold)
    if (!((bitboards[P] | bitboards[p]) & kingFlankMask[getFile[ksq]]))
    {
        penalty.mgBonus += PawnlessFlank[0];
        penalty.egBonus += PawnlessFlank[1];
    }

    // Linear penalty for attacks in the king flank
    penalty.mgBonus += FlankAttacks[0] * kingFlankAttack;
    penalty.egBonus += FlankAttacks[1] * kingFlankAttack;

    return penalty;
}

// Helper function to get piece type at a square
int getPieceType(int square, int side);

static inline int evaluateThreats(int us, int stageScore) {
    int them = 1 - us;  // Opponent side
    int score = 0;
    U64 b;

    // Get non-pawn enemies
    U64 NonPawnEnemies = nonPawnEnemies[them];

    // Squares strongly protected by the enemy
    U64 stronglyProtectedThem = stronglyProtected[them];

    // Non-pawn enemies, strongly protected
    U64 Defended = defended[them];

    // Enemies not strongly protected and under our attack
    U64 Weak = weak[them];

    // Bonus according to the kind of attacking pieces
    if (Defended | Weak) {

    /*
        // Minor pieces attacking defended or weak pieces
        b = (Defended | Weak) & (pieceAttackTables[us][N] | pieceAttackTables[us][B]);
        while (b) {
            int square = getLSFBIndex(b);
            int pieceType = getPieceType(square, them);
            if (pieceType != -1) {
                score += interpolate(ThreatByMinor_MG[pieceType], ThreatByMinor_EG[pieceType], stageScore);
            }
            popBit(b, square);
        }



        // Rooks attacking weak pieces
        b = Weak & pieceAttackTables[us][R];
        while (b) {
            int square = getLSFBIndex(b);
            int pieceType = getPieceType(square, them);
            if (pieceType != -1) {
                score += interpolate(ThreatByRook_MG[pieceType], ThreatByRook_EG[pieceType], stageScore);
            }
            popBit(b, square);
        }


        // King attacking weak pieces
        if (Weak & pieceAttackTables[us][K])
            score += interpolate(ThreatByKing_MG, ThreatByKing_EG, stageScore);


        // Hanging pieces (weak pieces not attacked by opponent)
        b = Weak & (~pieceAttackTables[them][allPieces] |
                   (NonPawnEnemies & attackedBy2[us]));
        score += interpolate(Hanging_MG, Hanging_EG, stageScore) * countBits(b);
    */


        // Weak pieces only protected by queen
        b = Weak & pieceAttackTables[them][Q];
        score += interpolate(WeakQueenProtection[0], WeakQueenProtection[1], stageScore) * countBits(b);

    }

    /*
    // Bonus for restricting opponent piece moves
    b = pieceAttackTables[them][allPieces] &
        ~stronglyProtectedThem &
        pieceAttackTables[us][allPieces];
    score += interpolate(RestrictedPiece_MG, RestrictedPiece_EG, stageScore) * countBits(b);
    */

    /*
    // Protected or unattacked squares
    U64 safe = ~pieceAttackTables[them][allPieces] | pieceAttackTables[us][allPieces];

    // Bonus for attacking enemy pieces with our relatively safe pawns
    U64 pawns = (us == white) ? bitboards[P] : bitboards[p];
    b = pawns & safe;
    U64 pawnAttacks = 0;

    // Calculate pawn attacks
    if (us == white) {
        U64 leftAttacks = (b << 7) & ~fileMask[0]; // Not A file
        U64 rightAttacks = (b << 9) & ~fileMask[7]; // Not H file
        pawnAttacks = leftAttacks | rightAttacks;
    } else {
        U64 leftAttacks = (b >> 9) & ~fileMask[0]; // Not A file
        U64 rightAttacks = (b >> 7) & ~fileMask[7]; // Not H file
        pawnAttacks = leftAttacks | rightAttacks;
    }

    b = pawnAttacks & NonPawnEnemies;
    score += interpolate(ThreatBySafePawn_MG, ThreatBySafePawn_EG, stageScore) * countBits(b);

    // Find squares where our pawns can push on the next move
    U64 pawnPushes = 0;
    if (us == white) {
        pawnPushes = (pawns << 8) & ~occupancies[both];
        // Pawns on rank 2 can push two squares
        U64 pawnsOnRank2 = pawns & rankMask[1]; // Rank 2
        U64 doublePushes = ((pawnsOnRank2 << 8) & ~occupancies[both]) << 8;
        doublePushes &= ~occupancies[both];
        pawnPushes |= doublePushes;
    } else {
        pawnPushes = (pawns >> 8) & ~occupancies[both];
        // Pawns on rank 7 can push two squares
        U64 pawnsOnRank7 = pawns & rankMask[6]; // Rank 7
        U64 doublePushes = ((pawnsOnRank7 >> 8) & ~occupancies[both]) >> 8;
        doublePushes &= ~occupancies[both];
        pawnPushes |= doublePushes;
    }

    // Keep only the squares which are relatively safe
    pawnPushes &= ~pieceAttackTables[them][P] & safe;


    // Calculate future pawn attacks
    U64 futurePawnAttacks = 0;
    if (us == white) {
        U64 leftAttacks = (pawnPushes << 7) & ~fileMask[0]; // Not A file
        U64 rightAttacks = (pawnPushes << 9) & ~fileMask[7]; // Not H file
        futurePawnAttacks = leftAttacks | rightAttacks;
    } else {
        U64 leftAttacks = (pawnPushes >> 9) & ~fileMask[0]; // Not A file
        U64 rightAttacks = (pawnPushes >> 7) & ~fileMask[7]; // Not H file
        futurePawnAttacks = leftAttacks | rightAttacks;
    }

    // Bonus for safe pawn threats on the next move
    b = futurePawnAttacks & NonPawnEnemies;
    score += interpolate(ThreatByPawnPush_MG, ThreatByPawnPush_EG, stageScore) * countBits(b);

    */
    /*
    // Bonus for threats against enemy queen
    U64 enemyQueen = (us == white) ? bitboards[q] : bitboards[Q];
    if (countBits(enemyQueen) == 1) {
        bool queenImbalance = countBits(bitboards[Q] | bitboards[q]) == 1;
        int queenSquare = getLSFBIndex(enemyQueen);

        // Define mobility area (you might need to adjust this based on your implementation)
        U64 mobilityArea = ~(us == white ? bitboards[P] : bitboards[p]) & ~stronglyProtectedThem;

        // Knight threats to queen
        U64 knightAttacksOnQueen = knightAttacks[queenSquare];
        b = pieceAttackTables[us][N] & knightAttacksOnQueen;
        score += interpolate(KnightOnQueen_MG, KnightOnQueen_EG, stageScore) *
                 countBits(b & mobilityArea) * (1 + queenImbalance);

        // Bishop and Rook threats to queen
        U64 bishopAttacksOnQueen = getBishopAttacks(queenSquare, occupancies[both]);
        U64 rookAttacksOnQueen = getRookAttacks(queenSquare, occupancies[both]);

        b = (pieceAttackTables[us][B] & bishopAttacksOnQueen) |
            (pieceAttackTables[us][R] & rookAttacksOnQueen);

        score += interpolate(SliderOnQueen_MG, SliderOnQueen_EG, stageScore) *
                 countBits(b & mobilityArea & attackedBy2[us]) * (1 + queenImbalance);
    }
    */
    return score;
}

static inline int getPositionScore(int piece, int square, int stage, int stageScore) {
    if (stage == middlegame) {
        return interpolate(PieceTables[opening][piece][square],
                         PieceTables[endgame][piece][square],
                         stageScore);
    }
    return PieceTables[stage][piece][square];
}

static inline int evaluateWhitePawn(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score += interpolate(materialScore[opening][P], materialScore[endgame][P], stageScore);
    } else {
        score += materialScore[stage][P];
    }

    // Double pawns evaluation
    int doublePawns = countBits(bitboards[P] & fileMask[square]);
    if (doublePawns > 1) {
        if (stage == middlegame) {
            score += interpolate(doublePawnPenalty[opening], doublePawnPenalty[endgame], stageScore);
        } else {
            score += doublePawnPenalty[stage];
        }
    }

    // Position score
    score += getPositionScore(P, square, stage, stageScore);

    // Get if pawn is weak and unopposed
    int isUnopposed = ((whiteOpposedMask[square] & bitboards[p]) == 0) ? 1 : 0;

    // Passed pawn evaluation
    if ((whitePassedMask[square] & bitboards[p]) == 0) {
        score += interpolate(passedPawnRankBonus[opening][getRank[square]],
                           passedPawnRankBonus[endgame][getRank[square]],
                           stageScore);
        score += interpolate(passedPawnFileBonus[opening][getFile[square]],
                           passedPawnFileBonus[endgame][getFile[square]],
                           stageScore);
    }

    // Connected pawn calculations
    int phalanx = ((phalanxMask[square] & bitboards[P]) != 0) ? 1 : 0;
    int supported = ((whiteSupportMask[square] & bitboards[P]) != 0) ? 1 : 0;
    int supportedCount = countBits(whiteSupportMask[square] & bitboards[P]);
    // pawns can never legally be on rank 8 (square < 8) but guard the index
    // anyway so a corrupt fen cannot read pawnAttacks out of bounds
    int backwardPawns = ((supported == 0) && square >= 8 && (pawnAttacks[white][square - 8] & bitboards[p])) ? 1 : 0;

    // Connected pawn bonus
    if (supported | phalanx) {
        score += connectedPawnBonus[getRank[square]] * (phalanx ? 3 : 2) /
                 (!isUnopposed ? 2 : 1) + 17 * supportedCount;
    }
    // Isolated pawn penalty
    else if ((bitboards[P] & isolatedMask[square]) == 0) {
        int isolatedPen = 0;
        int weakPen = 0;

        if (stage == middlegame) {
            isolatedPen = interpolate(isolatedPawnPenalty[opening], isolatedPawnPenalty[endgame], stageScore);
            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
        } else {
            isolatedPen = isolatedPawnPenalty[stage];
            weakPen = weakUnopposed[stage];
        }

        score += isUnopposed ? (isolatedPen + weakPen) : isolatedPen;
    }
    // Backward pawn penalty
    else if (backwardPawns) {
        int backwardPen = 0;
        int weakPen = 0;

        if (stage == middlegame) {
            backwardPen = interpolate(backwardPawnPenalty[opening], backwardPawnPenalty[endgame], stageScore);
            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
        } else {
            backwardPen = backwardPawnPenalty[stage];
            weakPen = weakUnopposed[stage];
        }

        score += backwardPen + weakPen;
    }
    /*
    // Calculate lever pawns (black pawns that attack this pawn)
    U64 leverPawns = pawnAttacks[white][square] & bitboards[p];
    int leverCount = countBits(leverPawns);

    // Penalize weak levers (when pawn isn't supported by another pawn)
    if (!supported && leverCount > 1) {
        if (stage == middlegame) {
            score += interpolate(WeakLever[opening], WeakLever[endgame], stageScore);
        } else {
            score += WeakLever[stage];
        }
    }
    */
    return score;
}

static inline int evaluateBlackPawn(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score -= interpolate(materialScore[opening][P], materialScore[endgame][P], stageScore);
    } else {
        score -= materialScore[stage][P];
    }

    // Double pawns evaluation
    int doublePawns = countBits(bitboards[p] & fileMask[square]);
    if (doublePawns > 1) {
        if (stage == middlegame) {
            score -= interpolate(doublePawnPenalty[opening], doublePawnPenalty[endgame], stageScore);
        } else {
            score -= doublePawnPenalty[stage];
        }
    }

    // Position score
    score -= getPositionScore(P, mirrorScore[square], stage, stageScore);

    // Get if pawn is weak and unopposed
    int isUnopposed = ((blackOpposedMask[square] & bitboards[P]) == 0) ? 1 : 0;

    // Passed pawn evaluation
    if ((blackPassedMask[square] & bitboards[P]) == 0) {
        score -= interpolate(passedPawnRankBonus[opening][getRank[mirrorScore[square]]],
                           passedPawnRankBonus[endgame][getRank[mirrorScore[square]]],
                           stageScore);
        score -= interpolate(passedPawnFileBonus[opening][getFile[mirrorScore[square]]],
                           passedPawnFileBonus[endgame][getFile[mirrorScore[square]]],
                           stageScore);
    }

    // Connected pawn calculations
    int phalanx = ((phalanxMask[square] & bitboards[p]) != 0) ? 1 : 0;
    int supported = ((blackSupportMask[square] & bitboards[p]) != 0) ? 1 : 0;
    int supportedCount = countBits(blackSupportMask[square] & bitboards[p]);
    // pawns can never legally be on rank 1 (square > 55) but guard the index
    // anyway so a corrupt fen cannot read pawnAttacks out of bounds
    int backwardPawns = ((supported == 0) && square <= 55 && (pawnAttacks[black][square + 8] & bitboards[P])) ? 1 : 0;

    // Connected pawn bonus
    if (supported | phalanx) {
        score -= connectedPawnBonus[getRank[mirrorScore[square]]] * (phalanx ? 3 : 2) /
                 (!isUnopposed ? 2 : 1) + 17 * supportedCount;
    }
    // Isolated pawn penalty
    else if ((bitboards[p] & isolatedMask[square]) == 0) {
        int isolatedPen = 0;
        int weakPen = 0;

        if (stage == middlegame) {
            isolatedPen = interpolate(isolatedPawnPenalty[opening], isolatedPawnPenalty[endgame], stageScore);
            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
        } else {
            isolatedPen = isolatedPawnPenalty[stage];
            weakPen = weakUnopposed[stage];
        }

        if (isUnopposed) {
            score -= isolatedPen + weakPen;
        } else {
            score -= isolatedPen;
        }
    }
    // Backward pawn penalty
    else if (backwardPawns) {
        int backwardPen = 0;
        int weakPen = 0;

        if (stage == middlegame) {
            backwardPen = interpolate(backwardPawnPenalty[opening], backwardPawnPenalty[endgame], stageScore);
            weakPen = interpolate(weakUnopposed[opening], weakUnopposed[endgame], stageScore);
        } else {
            backwardPen = backwardPawnPenalty[stage];
            weakPen = weakUnopposed[stage];
        }

        score -= backwardPen + weakPen;
    }
    /*
    // Calculate lever pawns (white pawns that attack this pawn)
    U64 leverPawns = pawnAttacks[black][square] & bitboards[P];
    int leverCount = countBits(leverPawns);

    // Penalize weak levers (when pawn isn't supported by another pawn)
    if (!supported && leverCount > 1) {
        if (stage == middlegame) {
            score -= interpolate(WeakLever[opening], WeakLever[endgame], stageScore);
        } else {
            score -= WeakLever[stage];
        }
    }
    */
    return score;
}

static inline int edgeDistance(int file) {
    return std::min(file, 7 - file);
}

static inline int evaluateWhiteKnight(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score += interpolate(materialScore[opening][N], materialScore[endgame][N], stageScore);
    } else {
        score += materialScore[stage][N];
    }

    // Base position score
    score += getPositionScore(N, square, stage, stageScore);

    // Mobility with improved bonus
    int mobility = getSimpleMobility(N, square);
    if (stage == middlegame) {
        score += interpolate(mobilityBonus[N][mobility][opening], mobilityBonus[N][mobility][endgame], stageScore);
    } else {
        score += mobilityBonus[N][mobility][stage];
    }

    // Knight adjustment based on pawn count
    score += knightAdj[countBits(bitboards[P])];

    // Outpost bonus - knight on rank 4-6 protected by pawn and safe from enemy pawn attacks
    // Ranks 4-6
    U64 outpostRanks = rankMask[a4] | rankMask[a5] | rankMask[a6];
    U64 outpostSquare = (1ULL << square) & outpostRanks;

    if (outpostSquare && (pieceAttackTables[white][P] & outpostSquare) &&
        !(pieceAttackTables[black][P] & outpostSquare)) {
        score += interpolate(OutpostBonusKnight[0], OutpostBonusKnight[1], stageScore);

        // Additional bonus for uncontested outpost on the flank
        if (!(fileMask[square] & 0x3C)) { // Not on center files (c,d,e,f)
            U64 targets = nonPawnEnemies[black];
            U64 KnightAttacks = knightAttacks[square];

            if (!(KnightAttacks & targets)) {
                // Uncontested outpost bonus
                score += interpolate(UncontestedOutpost[0], UncontestedOutpost[1], stageScore);
            }
        }
    }
    else {
        // Reachable outpost bonus - FIXED: Check if knight can reach outpost squares in one move
        U64 outpostCandidates = outpostRanks & pieceAttackTables[white][P] &
                                ~pieceAttackTables[black][P];
        U64 reachableOutposts = knightAttacks[square] & outpostCandidates & ~occupancies[white];

        if (reachableOutposts) {
            score += interpolate(ReachableOutpost[0], ReachableOutpost[1], stageScore);
        }
    }

    // King protection bonus - FIXED: Bonus for being close to own king
    int kingSquare = getLSFBIndex(bitboards[K]);
    int kingDistance = std::max(abs(getRank[square] - getRank[kingSquare]),
                          abs(getFile[square] - getFile[kingSquare]));
    // Closer knights get higher bonus (7 - distance ensures closer = higher value)
    score += interpolate(KingProtectorKnight[0], KingProtectorKnight[1], stageScore) * (7 - kingDistance);

    // Knight behind pawn bonus - FIXED: Correct shift direction for white pawns
    U64 pawnAhead = (bitboards[P] << 8) & (1ULL << square);
    if (pawnAhead) {
        score += interpolate(MinorBehindPawn[0], MinorBehindPawn[1], stageScore);
    }

    // Knight attacking queen bonus
    if (pieceAttackTables[white][N] & bitboards[q]) {
        score += interpolate(KnightOnQueen[0], KnightOnQueen[1], stageScore);
    }

    // Threat bonuses for attacking enemy pieces
    U64 attacks = knightAttacks[square];
    U64 threatenedPieces = attacks & nonPawnEnemies[black] & ~defended[black];

    if (threatenedPieces) {
        // Apply threat bonuses based on piece type using ThreatByMinor array
        if (threatenedPieces & bitboards[p])
            score += interpolate(ThreatByMinor[0][0], ThreatByMinor[0][1], stageScore);  // Pawn
        if (threatenedPieces & bitboards[n])
            score += interpolate(ThreatByMinor[1][0], ThreatByMinor[1][1], stageScore);  // Knight
        if (threatenedPieces & bitboards[b])
            score += interpolate(ThreatByMinor[2][0], ThreatByMinor[2][1], stageScore);  // Bishop
        if (threatenedPieces & bitboards[r])
            score += interpolate(ThreatByMinor[3][0], ThreatByMinor[3][1], stageScore);  // Rook
        if (threatenedPieces & bitboards[q])
            score += interpolate(ThreatByMinor[4][0], ThreatByMinor[4][1], stageScore);  // Queen
        // King threats are in index 5 but typically not used directly
    }

    // Hanging piece bonus (attacking undefended pieces)
    if (attacks & weak[black]) {
        score += interpolate(Hanging[0], Hanging[1], stageScore);
    }

    return score;
}

static inline int evaluateBlackKnight(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score -= interpolate(materialScore[opening][N], materialScore[endgame][N], stageScore);
    } else {
        score -= materialScore[stage][N];
    }

    // Use the correct piece index (N) and mirror the square
    score -= getPositionScore(N, mirrorScore[square], stage, stageScore);

    // Use the black knight (n) for mobility calculation
    int mobility = getSimpleMobility(n, square);
    if (stage == middlegame) {
        score -= interpolate(mobilityBonus[N][mobility][opening], mobilityBonus[N][mobility][endgame], stageScore);
    } else {
        score -= mobilityBonus[N][mobility][stage];
    }

    // Knight adjustment based on pawn count
    score -= knightAdj[countBits(bitboards[p])];

    // Outpost bonus - knight on rank 3-5 protected by pawn and safe from enemy pawn attacks
    // Ranks 3-5 for black
    U64 outpostRanks = rankMask[a3] | rankMask[a4] | rankMask[a5];
    U64 outpostSquare = (1ULL << square) & outpostRanks;

    if (outpostSquare && (pieceAttackTables[black][P] & outpostSquare) &&
        !(pieceAttackTables[white][P] & outpostSquare)) {
        score -= interpolate(OutpostBonusKnight[0], OutpostBonusKnight[1], stageScore);

        // Additional bonus for uncontested outpost on the flank
        if (!(fileMask[square] & 0x3C)) { // Not on center files (c,d,e,f)
            U64 targets = nonPawnEnemies[white];
            U64 KnightAttacks = knightAttacks[square];

            if (!(KnightAttacks & targets)) {
                // Uncontested outpost bonus
                score -= interpolate(UncontestedOutpost[0], UncontestedOutpost[1], stageScore);
            }
        }
    }
    else {
        // Reachable outpost bonus - FIXED: Check if knight can reach outpost squares in one move
        U64 outpostCandidates = outpostRanks & pieceAttackTables[black][P] &
                                ~pieceAttackTables[white][P];
        U64 reachableOutposts = knightAttacks[square] & outpostCandidates & ~occupancies[black];

        if (reachableOutposts) {
            score -= interpolate(ReachableOutpost[0], ReachableOutpost[1], stageScore);
        }
    }

    // King protection bonus - FIXED: Bonus for being close to own king
    int kingSquare = getLSFBIndex(bitboards[k]);
    int kingDistance = std::max(abs(getRank[square] - getRank[kingSquare]),
                          abs(getFile[square] - getFile[kingSquare]));
    // Closer knights get higher bonus (7 - distance ensures closer = higher value)
    score -= interpolate(KingProtectorKnight[0], KingProtectorKnight[1], stageScore) * (7 - kingDistance);

    // Knight behind pawn bonus - FIXED: Correct shift direction for black pawns
    U64 pawnAhead = (bitboards[p] >> 8) & (1ULL << square);
    if (pawnAhead) {
        score -= interpolate(MinorBehindPawn[0], MinorBehindPawn[1], stageScore);
    }

    // Knight attacking queen bonus
    // pieceAttackTables is indexed by piece type [P, N, B, R, Q, K], not piece colour+type
    // so the black knight attack set is pieceAttackTables[black][N], not [black][P]
    if (pieceAttackTables[black][N] & bitboards[Q]) {
        score -= interpolate(KnightOnQueen[0], KnightOnQueen[1], stageScore);
    }

    // Threat bonuses for attacking enemy pieces
    U64 attacks = knightAttacks[square];
    U64 threatenedPieces = attacks & nonPawnEnemies[white] & ~defended[white];

    if (threatenedPieces) {
        // Apply threat bonuses based on piece type using ThreatByMinor array
        if (threatenedPieces & bitboards[P])
            score -= interpolate(ThreatByMinor[0][0], ThreatByMinor[0][1], stageScore);  // Pawn
        if (threatenedPieces & bitboards[N])
            score -= interpolate(ThreatByMinor[1][0], ThreatByMinor[1][1], stageScore);  // Knight
        if (threatenedPieces & bitboards[B])
            score -= interpolate(ThreatByMinor[2][0], ThreatByMinor[2][1], stageScore);  // Bishop
        if (threatenedPieces & bitboards[R])
            score -= interpolate(ThreatByMinor[3][0], ThreatByMinor[3][1], stageScore);  // Rook
        if (threatenedPieces & bitboards[Q])
            score -= interpolate(ThreatByMinor[4][0], ThreatByMinor[4][1], stageScore);  // Queen
    }

    // Hanging piece bonus (attacking undefended pieces)
    if (attacks & weak[white]) {
        score -= interpolate(Hanging[0], Hanging[1], stageScore);
    }

    return score;
}

// Improved white bishop evaluation
static inline int evaluateWhiteBishop(int square, int stage, int stageScore) {
    int score = 0;

    // 1. Material and position score
    if (stage == middlegame) {
        score += interpolate(materialScore[opening][B], materialScore[endgame][B], stageScore);
    } else {
        score += materialScore[stage][B];
    }

    // 2. Base position score
    score += getPositionScore(B, square, stage, stageScore);

    // Get bishop's attacks and position as bitboard
    U64 bishopBB = 1ULL << square;
    U64 bishopAttacks = getBishopAttacks(square, occupancies[both]);

    // Calculate x-ray attacks (through queens)
    U64 bishopAttacksXRay = getBishopAttacks(square, occupancies[both] ^ (bitboards[Q] | bitboards[q]));

    // 3. Mobility - critical for bishops
    int mobility = getSimpleMobility(B, square);
    if (stage == middlegame) {
        score += interpolate(mobilityBonus[B][mobility][opening], mobilityBonus[B][mobility][endgame], stageScore);
    } else {
        score += mobilityBonus[B][mobility][stage];
    }

    // 4. Pawns on same color squares as bishop - CRITICAL weakness
    int bishopColor = (square & 1) ^ ((square >> 3) & 1); // 0 for light, 1 for dark
    U64 sameColorSquares = bishopColor ? darkSquares : lightSquares;
    int pawnsOnSameColor = countBits(bitboards[P] & sameColorSquares);

    // Consider if bishop is protected by pawns
    bool protectedByPawn = (pieceAttackTables[white][P] & bishopBB) != 0;

    // Consider blocked center pawns
    U64 blockedPawns = bitboards[P] & (occupancies[both] << 8) & centerFiles;

    // get edge distance
    int edgeDist = edgeDistance(getFile[square]);

    // Calculate penalty with nuance
    score -= interpolate(
        BishopPawnsPenalty[edgeDist][0],
        BishopPawnsPenalty[edgeDist][1],
        stageScore
    ) * pawnsOnSameColor * ((!protectedByPawn) + countBits(blockedPawns));

    // 5. Long diagonal bonus - important positional factor
    U64 bishopAttacksThruPawns = getBishopAttacks(square, occupancies[both] ^ bitboards[P]);
    if (countBits(bishopAttacksThruPawns & centerSquares) >= 2) {
        score += interpolate(LongDiagonalBishop[0], LongDiagonalBishop[1], stageScore);
    }

    // 6. King distance penalty
    int kingSquare = getLSFBIndex(bitboards[K]);
    int distance = std::max(abs(getFile[square] - getFile[kingSquare]),
                      abs(getRank[square] - getRank[kingSquare]));
    score -= interpolate(KingProtectorBishop[0], KingProtectorBishop[1], stageScore) * distance;

    // 7. X-Ray pawns penalty
    int enemyPawnsXRayed = countBits(bishopAttacksXRay & bitboards[p]);
    score -= interpolate(BishopXRayPawns[0], BishopXRayPawns[1], stageScore) * enemyPawnsXRayed;

    // 8. Outpost bonus
    U64 outpostSquares = outpostRanksWhite & ~pawnSpans[black];
    if ((bishopBB & outpostSquares) && (pieceAttackTables[white][P] & bishopBB)) {
        score += interpolate(OutpostBonusBishop[0], OutpostBonusBishop[1], stageScore);
    }

    // 9. Bishop behind pawn bonus
    if (bishopBB & (bitboards[P] << 8)) {
        score += interpolate(MinorBehindPawn[0], MinorBehindPawn[1], stageScore);
    }

    // 10. Threat bonuses for attacking enemy pieces (like in knight evaluation)
    U64 threatenedPieces = bishopAttacks & nonPawnEnemies[black] & ~defended[black];

    if (threatenedPieces) {
        // Apply threat bonuses based on piece type using ThreatByMinor array
        if (threatenedPieces & bitboards[p])
            score += interpolate(ThreatByMinor[0][0], ThreatByMinor[0][1], stageScore);  // Pawn
        if (threatenedPieces & bitboards[n])
            score += interpolate(ThreatByMinor[1][0], ThreatByMinor[1][1], stageScore);  // Knight
        if (threatenedPieces & bitboards[b])
            score += interpolate(ThreatByMinor[2][0], ThreatByMinor[2][1], stageScore);  // Bishop
        if (threatenedPieces & bitboards[r])
            score += interpolate(ThreatByMinor[3][0], ThreatByMinor[3][1], stageScore);  // Rook
        if (threatenedPieces & bitboards[q])
            score += interpolate(ThreatByMinor[4][0], ThreatByMinor[4][1], stageScore);  // Queen
        // King threats are in index 5 but typically not used directly
    }

    // 11. Hanging piece bonus (attacking undefended pieces)
    if (bishopAttacks & weak[black]) {
        score += interpolate(Hanging[0], Hanging[1], stageScore);
    }

    return score;
}

// Improved black bishop evaluation
static inline int evaluateBlackBishop(int square, int stage, int stageScore) {
    int score = 0;

    // 1. Material and position score
    if (stage == middlegame) {
        score -= interpolate(materialScore[opening][B], materialScore[endgame][B], stageScore);
    } else {
        score -= materialScore[stage][B];
    }

    // 2. Base position score (using mirror for black)
    score -= getPositionScore(B, mirrorScore[square], stage, stageScore);

    // Get bishop's attacks and position as bitboard
    U64 bishopBB = 1ULL << square;
    U64 bishopAttacks = getBishopAttacks(square, occupancies[both]);

    // Calculate x-ray attacks (through queens)
    U64 bishopAttacksXRay = getBishopAttacks(square, occupancies[both] ^ (bitboards[Q] | bitboards[q]));

    // 3. Mobility - critical for bishops
    int mobility = getSimpleMobility(b, square);
    if (stage == middlegame) {
        score -= interpolate(mobilityBonus[B][mobility][opening], mobilityBonus[B][mobility][endgame], stageScore);
    } else {
        score -= mobilityBonus[B][mobility][stage];
    }

    // 4. Pawns on same color squares as bishop - CRITICAL weakness
    int bishopColor = (square & 1) ^ ((square >> 3) & 1); // 0 for light, 1 for dark
    U64 sameColorSquares = bishopColor ? darkSquares : lightSquares;
    int pawnsOnSameColor = countBits(bitboards[p] & sameColorSquares);

    // Consider if bishop is protected by pawns
    // pieceAttackTables is indexed by piece type [P, N, B, R, Q, K]
    // the [black][p] form was reading [black][allPieces] since p == 6
    bool protectedByPawn = (pieceAttackTables[black][P] & bishopBB) != 0;

    // Consider blocked center pawns
    U64 blockedPawns = bitboards[p] & (occupancies[both] >> 8) & centerFiles;

    // get edge distance
    int edgeDist = edgeDistance(getFile[square]);

    // Calculate penalty with nuance
    score += interpolate(
        BishopPawnsPenalty[edgeDist][0],
        BishopPawnsPenalty[edgeDist][1],
        stageScore
    ) * pawnsOnSameColor * ((!protectedByPawn) + countBits(blockedPawns));

    // 5. Long diagonal bonus - important positional factor
    U64 bishopAttacksThruPawns = getBishopAttacks(square, occupancies[both] ^ bitboards[p]);
    if (countBits(bishopAttacksThruPawns & centerSquares) >= 2) {
        score -= interpolate(LongDiagonalBishop[0], LongDiagonalBishop[1], stageScore);
    }

    // 6. King distance penalty
    int kingSquare = getLSFBIndex(bitboards[k]);
    int distance = std::max(abs(getFile[square] - getFile[kingSquare]),
                      abs(getRank[square] - getRank[kingSquare]));
    score += interpolate(KingProtectorBishop[0], KingProtectorBishop[1], stageScore) * distance;

    // 7. X-Ray pawns penalty
    int enemyPawnsXRayed = countBits(bishopAttacksXRay & bitboards[P]);
    score += interpolate(BishopXRayPawns[0], BishopXRayPawns[1], stageScore) * enemyPawnsXRayed;

    // 8. Outpost bonus
    U64 outpostSquares = outpostRanksBlack & ~pawnSpans[white];
    if ((bishopBB & outpostSquares) && (pieceAttackTables[black][P] & bishopBB)) {
        score -= interpolate(OutpostBonusBishop[0], OutpostBonusBishop[1], stageScore);
    }

    // 9. Bishop behind pawn bonus
    if (bishopBB & (bitboards[p] >> 8)) {
        score -= interpolate(MinorBehindPawn[0], MinorBehindPawn[1], stageScore);
    }

    // 10. Threat bonuses for attacking enemy pieces (like in knight evaluation)
    U64 threatenedPieces = bishopAttacks & nonPawnEnemies[white] & ~defended[white];

    if (threatenedPieces) {
        // Apply threat bonuses based on piece type using ThreatByMinor array
        if (threatenedPieces & bitboards[P])
            score -= interpolate(ThreatByMinor[0][0], ThreatByMinor[0][1], stageScore);  // Pawn
        if (threatenedPieces & bitboards[N])
            score -= interpolate(ThreatByMinor[1][0], ThreatByMinor[1][1], stageScore);  // Knight
        if (threatenedPieces & bitboards[B])
            score -= interpolate(ThreatByMinor[2][0], ThreatByMinor[2][1], stageScore);  // Bishop
        if (threatenedPieces & bitboards[R])
            score -= interpolate(ThreatByMinor[3][0], ThreatByMinor[3][1], stageScore);  // Rook
        if (threatenedPieces & bitboards[Q])
            score -= interpolate(ThreatByMinor[4][0], ThreatByMinor[4][1], stageScore);  // Queen
        // King threats are in index 5 but typically not used directly
    }

    // 11. Hanging piece bonus (attacking undefended pieces)
    if (bishopAttacks & weak[white]) {
        score -= interpolate(Hanging[0], Hanging[1], stageScore);
    }

    return score;
}

// Bishop pair bonus (call this in your main evaluation function)
static inline int evaluateBishopPair(int stage, int stageScore) {
    int score = 0;

    // Check if white has bishop pair
    if (countBits(bitboards[B]) >= 2) {
        score += interpolate(BishopPairBonus[0], BishopPairBonus[1], stageScore);
    }

    // Check if black has bishop pair
    if (countBits(bitboards[b]) >= 2) {
        score -= interpolate(BishopPairBonus[0], BishopPairBonus[1], stageScore);
    }

    return score;
}

static inline int evaluateWhiteRook(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score += interpolate(materialScore[opening][R], materialScore[endgame][R], stageScore);
    } else {
        score += materialScore[stage][R];
    }

    score += getPositionScore(R, square, stage, stageScore);
    score += rookAdj[countBits(bitboards[P])];

    // Get mobility and attacks
    int mobility = getSimpleMobility(R, square);
    U64 rookAttacks = getRookAttacks(square, occupancies[both]);

    if (stage == middlegame) {
        score += interpolate(mobilityBonus[R][mobility][opening], mobilityBonus[R][mobility][endgame], stageScore);
    } else {
        score += mobilityBonus[R][mobility][stage];
    }

    // File scoring following Stockfish's approach more closely
    bool ourPawnsOnFile = (bitboards[P] & fileMask[square]) != 0;
    bool theirPawnsOnFile = (bitboards[p] & fileMask[square]) != 0;

    // Check if rook is on semi-open file (no own pawns)
    if (!ourPawnsOnFile) {
        // Bonus depends on whether opponent has pawns on the file
        // Stockfish gives more bonus for completely open files
        int openFileIndex = !theirPawnsOnFile ? 1 : 0;  // 1 = fully open, 0 = semi-open
        score += interpolate(RookOnOpenFile[openFileIndex][0], RookOnOpenFile[openFileIndex][1], stageScore);
    }
    else {
        // If our pawn on this file is blocked, apply penalty for rook on closed file
        U64 ourPawnsOnFileBlocked = bitboards[P] & fileMask[square] &
                                   (occupancies[both] << 8);
        if (ourPawnsOnFileBlocked) {
            score -= interpolate(RookOnClosedFile[0], RookOnClosedFile[1], stageScore);
        }
        /*
        // Add trapped rook penalty (from Stockfish)
        if (mobility <= 3) {
            int kingFile = getFile[getLSFBIndex(bitboards[K])];
            if ((kingFile < 4) == (getFile[square] < kingFile)) {
                bool canCastle = (castle & (wk | wq)) != 0;
                // If you have TrappedRook constants, use them instead of these values
                int penalty = 100 * (1 + !canCastle);
                score -= interpolate(penalty, penalty / 2, stageScore);
            }
        }
        */
    }

    // Threat bonuses for attacking enemy pieces
    U64 weak_pieces = weak[black];
    if (weak_pieces) {
        // Rooks attacking weak pieces
        U64 threatenedPieces = rookAttacks & weak_pieces;
        while (threatenedPieces) {
            int targetSquare = getLSFBIndex(threatenedPieces);
            int pieceType = getPieceType(targetSquare, black);
            if (pieceType >= 0 && pieceType < 6) { // Valid piece type index
                score += interpolate(ThreatByRook[pieceType][0], ThreatByRook[pieceType][1], stageScore);
            }
            popBit(threatenedPieces, targetSquare);
        }
    }

    return score;
}

static inline int evaluateBlackRook(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score -= interpolate(materialScore[opening][R], materialScore[endgame][R], stageScore);
    } else {
        score -= materialScore[stage][R];
    }

    // Use the correct piece index (R) and mirror the square
    score -= getPositionScore(R, mirrorScore[square], stage, stageScore);
    score -= rookAdj[countBits(bitboards[p])];

    // Get mobility and attacks
    int mobility = getSimpleMobility(r, square);
    U64 rookAttacks = getRookAttacks(square, occupancies[both]);

    if (stage == middlegame) {
        score -= interpolate(mobilityBonus[R][mobility][opening], mobilityBonus[R][mobility][endgame], stageScore);
    } else {
        score -= mobilityBonus[R][mobility][stage];
    }

    // File scoring following Stockfish's approach more closely
    bool ourPawnsOnFile = (bitboards[p] & fileMask[square]) != 0;
    bool theirPawnsOnFile = (bitboards[P] & fileMask[square]) != 0;

    // Check if rook is on semi-open file (no own pawns)
    if (!ourPawnsOnFile) {
        // Bonus depends on whether opponent has pawns on the file
        // Stockfish gives more bonus for completely open files
        int openFileIndex = !theirPawnsOnFile ? 1 : 0;  // 1 = fully open, 0 = semi-open
        score -= interpolate(RookOnOpenFile[openFileIndex][0], RookOnOpenFile[openFileIndex][1], stageScore);
    }
    else {
        // If our pawn on this file is blocked, apply penalty for rook on closed file
        U64 ourPawnsOnFileBlocked = bitboards[p] & fileMask[square] &
                                   (occupancies[both] >> 8);
        if (ourPawnsOnFileBlocked) {
            score += interpolate(RookOnClosedFile[0], RookOnClosedFile[1], stageScore);
        }
        /*
        // Add trapped rook penalty (from Stockfish)
        if (mobility <= 3) {
            int kingFile = getFile[getLSFBIndex(bitboards[k])];
            if ((kingFile < 4) == (getFile[square] < kingFile)) {
                bool canCastle = (castle & (bk | bq)) != 0;
                // If you have TrappedRook constants, use them instead of these values
                int penalty = 100 * (1 + !canCastle);
                score += interpolate(penalty, penalty / 2, stageScore);
            }
        }
        */
    }

    // Threat bonuses for attacking enemy pieces
    U64 weak_pieces = weak[white];
    if (weak_pieces) {
        // Rooks attacking weak pieces
        U64 threatenedPieces = rookAttacks & weak_pieces;
        while (threatenedPieces) {
            int targetSquare = getLSFBIndex(threatenedPieces);
            int pieceType = getPieceType(targetSquare, white);
            if (pieceType >= 0 && pieceType < 6) { // Valid piece type index
                score -= interpolate(ThreatByRook[pieceType][0], ThreatByRook[pieceType][1], stageScore);
            }
            popBit(threatenedPieces, targetSquare);
        }
    }

    return score;
}


static inline int evaluateWhiteQueen(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score += interpolate(materialScore[opening][Q], materialScore[endgame][Q], stageScore);
    } else {
        score += materialScore[stage][Q];
    }

    score += getPositionScore(Q, square, stage, stageScore);

    int queenMobility = getSimpleMobility(Q, square);
    if (stage == middlegame) {
        score += interpolate(mobilityBonus[Q][queenMobility][opening], mobilityBonus[Q][queenMobility][endgame], stageScore);
    } else {
        score += mobilityBonus[Q][queenMobility][stage];
    }

    return score;
}

static inline int evaluateWhiteKing(int square, int stage, int stageScore)
{
    int score = 0;

    // Position score
    score += getPositionScore(K, square, stage, stageScore);

    // King shelter and pawn storm
    kingShelter bonus = getKingSafety(white);
    score += interpolate(bonus.mgBonus, bonus.egBonus, stageScore);

    // King danger penalty (sum-of-contributions, Stockfish style)
    kingShelter danger = getKingDanger(white, bonus.mgBonus);
    score -= interpolate(danger.mgBonus, danger.egBonus, stageScore);
    /*
    // Bonus for king attacking weak enemy pieces
    U64 KingAttacks = kingAttacks[square];
    if (KingAttacks & weak[black]) {
        score += interpolate(ThreatByKing[0], ThreatByKing[1], stageScore);
    }
    */
    return score;
}

static inline int evaluateBlackQueen(int square, int stage, int stageScore)
{
    int score = 0;

    // Material and position score
    if (stage == middlegame) {
        score -= interpolate(materialScore[opening][Q], materialScore[endgame][Q], stageScore);
    } else {
        score -= materialScore[stage][Q];
    }

    // Use the correct piece index (Q) and mirror the square
    score -= getPositionScore(Q, mirrorScore[square], stage, stageScore);

    // Use the black queen (q) for mobility calculation
    int queenMobility = getSimpleMobility(q, square);
    if (stage == middlegame) {
        score -= interpolate(mobilityBonus[Q][queenMobility][opening], mobilityBonus[Q][queenMobility][endgame], stageScore);
    } else {
        score -= mobilityBonus[Q][queenMobility][stage];
    }

    return score;
}

static inline int evaluateBlackKing(int square, int stage, int stageScore)
{
    int score = 0;

    // Position score
    score -= getPositionScore(K, mirrorScore[square], stage, stageScore);

    // King shelter and pawn storm
    kingShelter bonus = getKingSafety(black);
    score -= interpolate(bonus.mgBonus, bonus.egBonus, stageScore);

    // King danger penalty (sum-of-contributions, Stockfish style)
    kingShelter danger = getKingDanger(black, bonus.mgBonus);
    score += interpolate(danger.mgBonus, danger.egBonus, stageScore);
    /*
    // Bonus for king attacking weak enemy pieces
    U64 KingAttacks = kingAttacks[square];
    if (KingAttacks & weak[white]) {
        score -= interpolate(ThreatByKing[0], ThreatByKing[1], stageScore);
    }
    */
    return score;
}

static inline int adjustEndgameEvaluation(int score) {
    // Don't adjust mate scores
    if (score > mateScore || score < -mateScore) {
        return score;
    }

    // Count material for each side
    int whitePawns = countBits(bitboards[P]);
    int blackPawns = countBits(bitboards[p]);
    int whiteRooks = countBits(bitboards[R]);
    int blackRooks = countBits(bitboards[r]);
    int whitePieces = countBits(occupancies[white]) - whitePawns - 1; // -1 for king
    int blackPieces = countBits(occupancies[black]) - blackPawns - 1; // -1 for king

    // K+R+P vs K+R - adjust score based on pawn advancement
    if (whiteRooks == 1 && blackRooks == 1 &&
        whitePawns == 1 && blackPawns == 0 &&
        whitePieces == 1 && blackPieces == 1) {

        // Find the white pawn
        int pawnSq = getLSFBIndex(bitboards[P]);
        int rank = getRank[pawnSq];

        // Calculate a score based on pawn advancement
        // The closer to promotion, the higher the score
        int pawnAdvancement = (7 - rank) * 10; // 0-60 points

        // Scale down the original evaluation to reflect drawing tendency
        // but preserve the advantage of advanced pawns
        return (score * 30 / 100) + pawnAdvancement;
    }

    if (whiteRooks == 1 && blackRooks == 1 &&
        whitePawns == 0 && blackPawns == 1 &&
        whitePieces == 1 && blackPieces == 1) {

        // Find the black pawn
        int pawnSq = getLSFBIndex(bitboards[p]);
        int rank = getRank[pawnSq];

        // Calculate a score based on pawn advancement
        // The closer to promotion, the higher the score
        int pawnAdvancement = rank * 10; // 0-60 points

        // Scale down the original evaluation but preserve pawn advancement advantage
        return (score * 30 / 100) - pawnAdvancement;
    }

    // K+N+N vs K (technically winnable but very difficult)
    if (whitePawns == 0 && blackPawns == 0) {
        if (whitePieces == 2 && blackPieces == 0 &&
            countBits(bitboards[N]) == 2) {
            // Scale down the evaluation significantly
            return score * 20 / 100;
        }

        if (whitePieces == 0 && blackPieces == 2 &&
            countBits(bitboards[n]) == 2) {
            // Scale down the evaluation significantly
            return score * 20 / 100;
        }
    }

    // No adjustment needed
    return score;
}

static inline bool isInsufficientMaterial() {
    // Count material for each side
    int whitePieceCount = countBits(occupancies[white]);
    int blackPieceCount = countBits(occupancies[black]);

    // King vs King
    if (whitePieceCount == 1 && blackPieceCount == 1)
        return true;

    // King + minor piece vs King
    if ((whitePieceCount == 2 && blackPieceCount == 1) ||
        (whitePieceCount == 1 && blackPieceCount == 2)) {

        // Check if the side with 2 pieces has only a king and a minor piece
        if (whitePieceCount == 2) {
            // White has 2 pieces - check if it's K+N or K+B
            if (countBits(bitboards[N]) == 1 || countBits(bitboards[B]) == 1)
                return true;
        } else {
            // Black has 2 pieces - check if it's K+N or K+B
            if (countBits(bitboards[n]) == 1 || countBits(bitboards[b]) == 1)
                return true;
        }
    }

    // King + Bishop vs King + Bishop (same color bishops)
    if (whitePieceCount == 2 && blackPieceCount == 2 &&
        countBits(bitboards[B]) == 1 && countBits(bitboards[b]) == 1) {

        // Get bishop squares
        int whiteBishopSq = getLSFBIndex(bitboards[B]);
        int blackBishopSq = getLSFBIndex(bitboards[b]);

        // Check if bishops are on same color squares
        bool whiteBishopOnLight = ((whiteBishopSq & 1) ^ ((whiteBishopSq >> 3) & 1)) == 0;
        bool blackBishopOnLight = ((blackBishopSq & 1) ^ ((blackBishopSq >> 3) & 1)) == 0;

        if (whiteBishopOnLight == blackBishopOnLight)
            return true;
    }

    return false;
}

// evaluate a position
static inline int evaluate()
{
    //printf("Evaluating position...\n");

    // game phase score
    int stageScore = getGameStageScore();
    //printf("Stage score: %d\n", stageScore);

    // game stage (opening, mg, eg)
    int stage = 0;

    // Set game stage
    if (stageScore > openingPhaseScore) {
        stage = opening;
    } else if (stageScore < endgamePhaseScore) {
        stage = endgame;

        // Only check for endgame-specific evaluations in endgame
        // Check for insufficient material first
        if (isInsufficientMaterial()) {
            return 0; // Return draw score
        }
    } else {
        stage = middlegame;
    }

    // evaluation score
    int score = 0;

    // add tempo bonus
    if (side == white) {
        score += tempoBonus;
        //printf("Added tempo bonus for white: %d\n", tempoBonus);
    } else {
        score -= tempoBonus;
        //printf("Subtracted tempo bonus for black: %d\n", tempoBonus);
    }

    // copy of the current pieces
    U64 bitboard;

    // Loop over all pieces
    for (int bitPiece = P; bitPiece <= k; bitPiece++)
    {
        bitboard = bitboards[bitPiece];
        //printf("Evaluating piece type: %c\n", ASCIIpieces[bitPiece]);

        while (bitboard)
        {
            int square = getLSFBIndex(bitboard);
            int pieceScore = 0;

            switch (bitPiece)
            {
                case P: pieceScore = evaluateWhitePawn(square, stage, stageScore); break;
                case N: pieceScore = evaluateWhiteKnight(square, stage, stageScore); break;
                case B: pieceScore = evaluateWhiteBishop(square, stage, stageScore); break;
                case R: pieceScore = evaluateWhiteRook(square, stage, stageScore); break;
                case Q: pieceScore = evaluateWhiteQueen(square, stage, stageScore); break;
                case K: pieceScore = evaluateWhiteKing(square, stage, stageScore); break;
                case p: pieceScore = evaluateBlackPawn(square, stage, stageScore); break;
                case n: pieceScore = evaluateBlackKnight(square, stage, stageScore); break;
                case b: pieceScore = evaluateBlackBishop(square, stage, stageScore); break;
                case r: pieceScore = evaluateBlackRook(square, stage, stageScore); break;
                case q: pieceScore = evaluateBlackQueen(square, stage, stageScore); break;
                case k: pieceScore = evaluateBlackKing(square, stage, stageScore); break;
            }

            score += pieceScore;
            //printf("Piece %c at %s evaluated to %d (total score now %d)\n",
            //       ASCIIpieces[bitPiece], squareNames[square], pieceScore, score);

            popBit(bitboard, square);
        }
    }

    // Add threat score
    //int whiteThreats = evaluateThreats(white, stageScore);
    //int blackThreats = evaluateThreats(black, stageScore);
    //score += whiteThreats - blackThreats;
    //printf("Threats - White: %d, Black: %d, Net: %d\n", whiteThreats, blackThreats, whiteThreats - blackThreats);

    // bishop pair bonus
    int bishopPair = evaluateBishopPair(stage, stageScore);
    score += bishopPair;

    // After calculating the full evaluation, adjust for near-draws
    // Only if it's in endgame and not a mate score
    /*
    if (stage == endgame && !(score > mateScore || score < -mateScore)) {
        score = adjustEndgameEvaluation(score);
    }
    */
    // Return final eval based on side
    int finalScore = (side == white) ? score : -score;
    //printf("Final evaluation score: %d\n", finalScore);

    return finalScore;
}

// test king eval
void testKingEval();

#endif // EVALUATION_H
