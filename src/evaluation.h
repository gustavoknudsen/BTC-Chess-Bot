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

// Pieces (either colour) pinned to side `us`'s king: a single piece on the squares
// strictly between the king and an enemy slider that x-rays the king. Computed once per
// node from precomputed betweenMask; no per-square ray walking.
static inline U64 computeKingBlockers(int us) {
    int ksq = getLSFBIndex(bitboards[(us == white) ? K : k]);
    U64 enemyRooksQueens   = bitboards[(us == white) ? r : R] | bitboards[(us == white) ? q : Q];
    U64 enemyBishopsQueens = bitboards[(us == white) ? b : B] | bitboards[(us == white) ? q : Q];

    U64 snipers = (getRookAttacks(ksq, 0ULL) & enemyRooksQueens)
                | (getBishopAttacks(ksq, 0ULL) & enemyBishopsQueens);
    U64 occ = occupancies[both] ^ snipers;

    U64 blockers = 0ULL;
    while (snipers) {
        int sniperSq = getLSFBIndex(snipers);
        U64 between = betweenMask[ksq][sniperSq] & occ;
        if (between && (between & (between - 1)) == 0)
            blockers |= between;
        popBit(snipers, sniperSq);
    }
    return blockers;
}

// Build the per-side mobility areas and king-blocker sets once before evaluation,
// following Stockfish. A square is excluded from a side's mobility area if it holds a
// blocked or low-rank pawn, its own king or queen, a piece pinned to its king, or is
// attacked by an enemy pawn. Must run after initAttacksTotal (uses enemy pawn attacks).
static inline void updateMobilityAreas() {
    kingBlockers[white] = computeKingBlockers(white);
    kingBlockers[black] = computeKingBlockers(black);

    // pawns that are blocked or on the first two ranks (relative to the side)
    U64 whitePawnExcl = bitboards[P] & ((occupancies[both] << 8) | rankMask[a2] | rankMask[a3]);
    U64 blackPawnExcl = bitboards[p] & ((occupancies[both] >> 8) | rankMask[a6] | rankMask[a7]);

    mobilityAreaWhite = ~(whitePawnExcl | bitboards[K] | bitboards[Q] |
                          kingBlockers[white] | pieceAttackTables[black][P]);

    mobilityAreaBlack = ~(blackPawnExcl | bitboards[k] | bitboards[q] |
                          kingBlockers[black] | pieceAttackTables[white][P]);
}

// Stockfish-style mobility. Sliders x-ray through both queens (and rooks also through
// friendly rooks) by removing those pieces from the occupancy passed to the magic
// lookup. A piece pinned to its own king is restricted to the pin line. The result is
// the count of attacked squares inside the side's mobility area.
static inline int getMobility(int piece, int sq) {
    int us = (piece >= p) ? black : white;
    U64 area = (us == white) ? mobilityAreaWhite : mobilityAreaBlack;
    U64 occ = occupancies[both];
    U64 queens = bitboards[Q] | bitboards[q];
    U64 att = 0;

    switch (piece) {
        case N: case n:
            att = knightAttacks[sq];
            break;

        case B: case b:
            att = getBishopAttacks(sq, occ ^ queens);
            break;

        case R: case r:
            att = getRookAttacks(sq, occ ^ queens ^ bitboards[(us == white) ? R : r]);
            break;

        case Q: case q:
            att = getQueenAttacks(sq, occ);
            break;
    }

    // a piece pinned to its own king may only move along the pin line
    if (kingBlockers[us] & (1ULL << sq)) {
        int ksq = getLSFBIndex(bitboards[(us == white) ? K : k]);
        att &= lineBB[ksq][sq];
    }

    return countBits(att & area);
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

    int hasEnemyQueen = countBits(bitboards[(us == white) ? q : Q]) > 0;
    int knightDefense = bool(pieceAttackTables[us][N] & pieceAttackTables[us][K]);

    // King-ring attacker accumulation (Stockfish-faithful): enemy pieces whose attacks
    // reach our king ring count toward the attacker weight; attacks on squares directly
    // adjacent to the king count toward kingAttacksCount. This replaces the old wide
    // directional zone, which over-counted and inflated kingDanger.
    U64 kingAdj = kingAttacks[ksq];
    int attackersCount   = countBits(kingRing & pieceAttackTables[them][P]);  // enemy pawns (weight 0)
    int attackersWeight  = 0;
    int kingAttacksCount = 0;
    U64 occBoth = occupancies[both];
    const int kpTypes[4] = { N, B, R, Q };
    for (int t = 0; t < 4; t++) {
        int pieceType = kpTypes[t];
        U64 bb = bitboards[(us == white) ? (pieceType + 6) : pieceType];
        while (bb) {
            int sq = getLSFBIndex(bb);
            U64 a = (pieceType == N) ? knightAttacks[sq]
                  : (pieceType == B) ? getBishopAttacks(sq, occBoth)
                  : (pieceType == R) ? getRookAttacks(sq, occBoth)
                                     : getQueenAttacks(sq, occBoth);
            if (a & kingRing) {
                attackersCount++;
                attackersWeight += KingAttackWeights[pieceType];
                kingAttacksCount += countBits(a & kingAdj);
            }
            popBit(bb, sq);
        }
    }

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

    kingDanger += attackersCount * attackersWeight
                + kdWeakRing      * countBits(kingRing & weakSquares)
                + kdUnsafeCheck   * countBits(unsafeChecks)
                + kdBlocker       * blockers
                + kdKingAttacks   * kingAttacksCount
                + kdFlankAttack   * kingFlankAttack * kingFlankAttack / 8
                - kdNoQueen       * !hasEnemyQueen
                - kdKnightDefense * knightDefense
                - kdShelter       * shelterMg / 8
                - kdFlankDefense  * kingFlankDefense
                + kdInit;

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

// Threats by side `us` against the enemy, following Stockfish threats(). Uses the
// attack/weak/defended sets built in initAttacksTotal and the per-node mobility areas.
// Constants are old-Stockfish units. Returns a score from us's perspective.
static inline int evaluateThreats(int us, int stageScore) {
    int them = 1 - us;
    int mg = 0, eg = 0;

    U64 ourMinors = pieceAttackTables[us][N] | pieceAttackTables[us][B];
    U64 ourAll    = pieceAttackTables[us][allPieces];
    U64 theirAll  = pieceAttackTables[them][allPieces];

    U64 enemyP = bitboards[(us == white) ? p : P];
    U64 enemyN = bitboards[(us == white) ? n : N];
    U64 enemyB = bitboards[(us == white) ? b : B];
    U64 enemyR = bitboards[(us == white) ? r : R];
    U64 enemyQ = bitboards[(us == white) ? q : Q];

    // These BTC globals are indexed by the attacking side: weak[us]/nonPawnEnemies[us]
    // are the enemy pieces `us` threatens. Defended is recomputed because the defended[]
    // global wrongly uses the attacker's own protection.
    U64 stronglyProtectedThem = stronglyProtected[them];
    U64 Defended = nonPawnEnemies[us] & stronglyProtectedThem;
    U64 Weak = weak[us];

    if (Defended | Weak)
    {
        // Threats by minor pieces on defended or weak enemies, by attacked piece type
        U64 b = (Defended | Weak) & ourMinors;
        mg += ThreatByMinor[1][0] * countBits(b & enemyP) + ThreatByMinor[2][0] * countBits(b & enemyN)
            + ThreatByMinor[3][0] * countBits(b & enemyB) + ThreatByMinor[4][0] * countBits(b & enemyR)
            + ThreatByMinor[5][0] * countBits(b & enemyQ);
        eg += ThreatByMinor[1][1] * countBits(b & enemyP) + ThreatByMinor[2][1] * countBits(b & enemyN)
            + ThreatByMinor[3][1] * countBits(b & enemyB) + ThreatByMinor[4][1] * countBits(b & enemyR)
            + ThreatByMinor[5][1] * countBits(b & enemyQ);

        // Threats by rook on weak enemies, by attacked piece type
        b = Weak & pieceAttackTables[us][R];
        mg += ThreatByRook[1][0] * countBits(b & enemyP) + ThreatByRook[2][0] * countBits(b & enemyN)
            + ThreatByRook[3][0] * countBits(b & enemyB) + ThreatByRook[4][0] * countBits(b & enemyR)
            + ThreatByRook[5][0] * countBits(b & enemyQ);
        eg += ThreatByRook[1][1] * countBits(b & enemyP) + ThreatByRook[2][1] * countBits(b & enemyN)
            + ThreatByRook[3][1] * countBits(b & enemyB) + ThreatByRook[4][1] * countBits(b & enemyR)
            + ThreatByRook[5][1] * countBits(b & enemyQ);

        // Threat by king on weak enemies
        if (Weak & pieceAttackTables[us][K])
        {
            mg += ThreatByKing[0];
            eg += ThreatByKing[1];
        }

        // Hanging: weak pieces not defended, or non-pawns we attack twice
        U64 hanging = Weak & (~theirAll | (nonPawnEnemies[us] & attackedBy2[us]));
        mg += Hanging[0] * countBits(hanging);
        eg += Hanging[1] * countBits(hanging);

        // Weak pieces only protected by the enemy queen
        U64 wq = Weak & pieceAttackTables[them][Q];
        mg += WeakQueenProtection[0] * countBits(wq);
        eg += WeakQueenProtection[1] * countBits(wq);
    }

    // Restricting enemy piece moves (attacked, not strongly protected, and we attack the square)
    U64 restricted = theirAll & ~stronglyProtectedThem & ourAll;
    mg += RestrictedPiece[0] * countBits(restricted);
    eg += RestrictedPiece[1] * countBits(restricted);

    // Squares safe for our pawns to operate from
    U64 safe = ~theirAll | ourAll;
    U64 ourPawns = bitboards[(us == white) ? P : p];
    U64 nonPawnE = nonPawnEnemies[us];

    // Threat by our relatively safe pawns
    U64 pawnThreat = 0;
    U64 tmp = ourPawns & safe;
    while (tmp)
    {
        int sq = getLSFBIndex(tmp);
        pawnThreat |= pawnAttacks[us][sq];
        popBit(tmp, sq);
    }
    int safePawnHits = countBits(pawnThreat & nonPawnE);
    mg += ThreatBySafePawn[0] * safePawnHits;
    eg += ThreatBySafePawn[1] * safePawnHits;

    // Threat by a safe pawn push on the next move
    U64 empty = ~occupancies[both];
    U64 pushes;
    if (us == white)
    {
        pushes = (ourPawns >> 8) & empty;
        pushes |= ((pushes & rankMask[a3]) >> 8) & empty;
    }
    else
    {
        pushes = (ourPawns << 8) & empty;
        pushes |= ((pushes & rankMask[a6]) << 8) & empty;
    }
    pushes &= ~pieceAttackTables[them][P] & safe;

    U64 pushThreat = 0;
    tmp = pushes;
    while (tmp)
    {
        int sq = getLSFBIndex(tmp);
        pushThreat |= pawnAttacks[us][sq];
        popBit(tmp, sq);
    }
    int pushHits = countBits(pushThreat & nonPawnE);
    mg += ThreatByPawnPush[0] * pushHits;
    eg += ThreatByPawnPush[1] * pushHits;

    // Threats on the next move against a lone enemy queen. A square is a usable target if
    // it is in our mobility area, not on one of our pawns, and not strongly protected by
    // the enemy. KnightOnQueen counts safe squares from which a knight forks the queen;
    // SliderOnQueen counts safe, doubly-attacked squares from which a slider hits it.
    U64 enemyQueen = bitboards[(us == white) ? q : Q];
    if (countBits(enemyQueen) == 1)
    {
        int queenImbalance = (countBits(bitboards[Q] | bitboards[q]) == 1) ? 1 : 0;
        int qsq = getLSFBIndex(enemyQueen);
        U64 area = (us == white) ? mobilityAreaWhite : mobilityAreaBlack;
        U64 safeSq = area & ~ourPawns & ~stronglyProtectedThem;

        U64 kb = pieceAttackTables[us][N] & knightAttacks[qsq];
        int knightHits = countBits(kb & safeSq);
        mg += KnightOnQueen[0] * knightHits * (1 + queenImbalance);
        eg += KnightOnQueen[1] * knightHits * (1 + queenImbalance);

        U64 sb = (pieceAttackTables[us][B] & getBishopAttacks(qsq, occupancies[both]))
               | (pieceAttackTables[us][R] & getRookAttacks(qsq, occupancies[both]));
        int sliderHits = countBits(sb & safeSq & attackedBy2[us]);
        mg += SliderOnQueen[0] * sliderHits * (1 + queenImbalance);
        eg += SliderOnQueen[1] * sliderHits * (1 + queenImbalance);
    }

    return interpolate(mg, eg, stageScore);
}

// Minimum non-pawn material (both sides, mg units) for the space term to apply. Below
// this, too much has been traded for space to matter. Matches Stockfish SpaceThreshold.
static const int SpaceThreshold = 11551;

// Stockfish space(): counts safe squares on the four central files in our own half,
// weighted up by how many pieces are on the board and how blocked the pawn structure is.
// Pure middlegame term (eg component zero). stageScore is the non-pawn material total,
// which equals Stockfish's non_pawn_material() on this material scale.
static inline int evaluateSpace(int us, int stageScore) {
    if (stageScore < SpaceThreshold)
        return 0;

    int them = 1 - us;
    U64 ourPawns = bitboards[(us == white) ? P : p];
    U64 spaceMask = centerFiles & ((us == white) ? (rankMask[a2] | rankMask[a3] | rankMask[a4])
                                                 : (rankMask[a7] | rankMask[a6] | rankMask[a5]));

    // safe central squares: not on our pawns, not attacked by an enemy pawn
    U64 safeArea = spaceMask & ~ourPawns & ~pieceAttackTables[them][P];

    // squares up to three ranks behind a friendly pawn (count completely safe ones twice)
    U64 behind = ourPawns;
    if (us == white) { behind |= behind << 8; behind |= behind << 16; }
    else             { behind |= behind >> 8; behind |= behind >> 16; }

    int bonus = countBits(safeArea)
              + countBits(behind & safeArea & ~pieceAttackTables[them][allPieces]);

    // blocked pawns (both sides): a pawn whose front square holds an enemy pawn or is
    // covered by two enemy pawns
    int blockedCount = countBits((bitboards[P] >> 8) & (bitboards[p] | pawnDoubleTables[black]))
                     + countBits((bitboards[p] << 8) & (bitboards[P] | pawnDoubleTables[white]));

    int weight = countBits(occupancies[us]) - 3 + std::min(blockedCount, 9);
    return bonus * weight * weight / 16;
}

static inline int getPositionScore(int piece, int square, int stage, int stageScore) {
    if (stage == middlegame) {
        return interpolate(PieceTables[opening][piece][square],
                         PieceTables[endgame][piece][square],
                         stageScore);
    }
    return PieceTables[stage][piece][square];
}

// Chebyshev king distance to a square, capped at 5. (distance() elsewhere is Manhattan,
// so this is computed directly.)
static inline int kingProximity(int ksq, int sq) {
    int d = std::max(abs(getFile[ksq] - getFile[sq]), abs(getRank[ksq] - getRank[sq]));
    return std::min(d, 5);
}

// Passed-pawn bonus for a pawn of colour `us` on square s. Returns the bonus from us's
// perspective; pawns that are not passed return 0. Starts from a rank/file base, then for
// pawns past the 3rd rank adds king-proximity (endgame) and, if the pawn can advance, a
// bonus scaled by how safe and defended its path to promotion is.
static inline int evaluatePassedPawn(int us, int s, int stageScore) {
    int them = 1 - us;
    U64 span = (us == white) ? whitePassedMask[s] : blackPassedMask[s];

    // strict passed pawn: no enemy pawn anywhere in the 3-file forward span
    if (span & bitboards[(us == white) ? p : P])
        return 0;

    int r = relativeRank(us, getRank[s]);
    int mg = PassedRank[r][0];
    int eg = PassedRank[r][1];

    if (r > 2)
    {
        int up = (us == white) ? -8 : 8;
        int blockSq = s + up;
        int ksqUs   = getLSFBIndex(bitboards[(us == white) ? K : k]);
        int ksqThem = getLSFBIndex(bitboards[(us == white) ? k : K]);
        int w = 5 * r - 13;

        // king proximity to the stop square (endgame only)
        eg += (kingProximity(ksqThem, blockSq) * 19 / 4 - kingProximity(ksqUs, blockSq) * 2) * w;
        if (r != 6)
            eg -= kingProximity(ksqUs, blockSq + up) * w;

        // if the pawn is free to advance, reward a clear/defended path to promotion
        if (!getBit(occupancies[both], blockSq))
        {
            U64 squaresToQueen = (us == white) ? whiteOpposedMask[s] : blackOpposedMask[s];
            U64 unsafeSquares  = span;
            U64 behindFile     = (us == white) ? blackOpposedMask[s] : whiteOpposedMask[s];
            U64 rooksQueens    = bitboards[R] | bitboards[r] | bitboards[Q] | bitboards[q];
            U64 bb = behindFile & rooksQueens;

            // unless an enemy rook/queen sits behind the pawn, only enemy-controlled or
            // enemy-occupied squares on the span count as unsafe
            if (!(occupancies[them] & bb))
                unsafeSquares &= pieceAttackTables[them][allPieces] | occupancies[them];

            U64 blockBit = 1ULL << blockSq;
            int k = !unsafeSquares                               ? 36 :
                    !(unsafeSquares & ~pieceAttackTables[us][P]) ? 30 :
                    !(unsafeSquares & squaresToQueen)            ? 17 :
                    !(unsafeSquares & blockBit)                  ?  7 : 0;

            // bonus if our own rook/queen backs the pawn or we defend the stop square
            if ((occupancies[us] & bb) || (pieceAttackTables[us][allPieces] & blockBit))
                k += 5;

            mg += k * w;
            eg += k * w;
        }
    }

    int file = getFile[s];
    int edge = std::min(file, 7 - file);
    return interpolate(mg, eg, stageScore) - interpolate(PassedFile[0], PassedFile[1], stageScore) * edge;
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
    score += evaluatePassedPawn(white, square, stageScore);

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
    score -= evaluatePassedPawn(black, square, stageScore);

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
    int mobility = getMobility(N, square);
    if (stage == middlegame) {
        score += interpolate(mobilityBonus[N][mobility][opening], mobilityBonus[N][mobility][endgame], stageScore);
    } else {
        score += mobilityBonus[N][mobility][stage];
    }

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

    // KnightOnQueen handled centrally in evaluateThreats()

    // Threats handled centrally in evaluateThreats()

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
    int mobility = getMobility(n, square);
    if (stage == middlegame) {
        score -= interpolate(mobilityBonus[N][mobility][opening], mobilityBonus[N][mobility][endgame], stageScore);
    } else {
        score -= mobilityBonus[N][mobility][stage];
    }

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

    // KnightOnQueen handled centrally in evaluateThreats()

    // Threats handled centrally in evaluateThreats()

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
    int mobility = getMobility(B, square);
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

    // Threats handled centrally in evaluateThreats()

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
    int mobility = getMobility(b, square);
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

    // Threats handled centrally in evaluateThreats()

    return score;
}

// Second-degree polynomial material imbalance for one side and one phase (0 = mg, 1 = eg).
// pc[colour][type] holds piece counts; index 0 is a flag for holding two bishops, then
// 1=pawn..5=queen. Each piece type scores against our own other pieces (QuadraticOurs)
// and the enemy's pieces (QuadraticTheirs).
static inline int imbalanceSide(int us, const int pc[2][6], int phase) {
    int them = 1 - us;
    int bonus = 0;
    for (int pt1 = 0; pt1 <= 5; pt1++) {
        if (!pc[us][pt1])
            continue;
        int v = QuadraticOurs[pt1][pt1][phase] * pc[us][pt1];
        for (int pt2 = 0; pt2 < pt1; pt2++)
            v += QuadraticOurs[pt1][pt2][phase] * pc[us][pt2]
               + QuadraticTheirs[pt1][pt2][phase] * pc[them][pt2];
        bonus += pc[us][pt1] * v;
    }
    return bonus;
}

// Material imbalance from white's perspective.
static inline int evaluateImbalance(int stageScore) {
    int pc[2][6] = {
        { countBits(bitboards[B]) > 1, countBits(bitboards[P]), countBits(bitboards[N]),
          countBits(bitboards[B]),     countBits(bitboards[R]), countBits(bitboards[Q]) },
        { countBits(bitboards[b]) > 1, countBits(bitboards[p]), countBits(bitboards[n]),
          countBits(bitboards[b]),     countBits(bitboards[r]), countBits(bitboards[q]) }
    };
    int mg = (imbalanceSide(white, pc, 0) - imbalanceSide(black, pc, 0)) / 16;
    int eg = (imbalanceSide(white, pc, 1) - imbalanceSide(black, pc, 1)) / 16;
    return interpolate(mg, eg, stageScore);
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

    // Get mobility and attacks
    int mobility = getMobility(R, square);
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

    // Threats handled centrally in evaluateThreats()

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

    // Get mobility and attacks
    int mobility = getMobility(r, square);
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

    // Threats handled centrally in evaluateThreats()

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

    int queenMobility = getMobility(Q, square);
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
    int queenMobility = getMobility(q, square);
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

    // Add threat score (Stockfish-style threats, computed once per side)
    score += evaluateThreats(white, stageScore) - evaluateThreats(black, stageScore);

    // Space (middlegame-only; the mg total decays toward the endgame via interpolation)
    score += interpolate(evaluateSpace(white, stageScore) - evaluateSpace(black, stageScore), 0, stageScore);

    // material imbalance (piece-pair interactions)
    score += evaluateImbalance(stageScore);

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
