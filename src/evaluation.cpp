#include "evaluation.h"

// pawn-structure cache (keyed by pawn configuration; see evaluatePawnStructureCached)
PawnHashEntry pawnHash[PAWN_HASH_SIZE];

// KP vs K win/draw table, in NORMALIZED coordinates (the strong side is white and its pawn is
// on files a-d). Generated once at startup by retrograde fixpoint. Result codes are bit flags
// so the classify step can OR successor results together.
#define KPK_INVALID 0
#define KPK_UNKNOWN 1
#define KPK_DRAW    2
#define KPK_WIN     4

static signed char kpkDB[2 * 64 * 64 * 64];

// index by [side to move (0 = strong/white)][strong pawn][strong king][weak king]
static inline int kpkIdx(int stm, int wk, int wp, int bk) {
    return ((stm * 64 + wp) * 64 + wk) * 64 + bk;
}

static inline int kpkCheby(int a, int b) {
    return std::max(abs(getFile[a] - getFile[b]), abs(getRank[a] - getRank[b]));
}

// Result for one position, read from the current table state. white = strong side to move.
static int kpkClassify(int stm, int wk, int wp, int bk) {
    int them = 1 - stm;
    int Good = (stm == white) ? KPK_WIN : KPK_DRAW;
    int Bad  = (stm == white) ? KPK_DRAW : KPK_WIN;
    int r = KPK_INVALID;

    if (stm == white)
    {
        U64 b = kingAttacks[wk];
        while (b) { int s = getLSFBIndex(b); popBit(b, s); r |= kpkDB[kpkIdx(them, s, wp, bk)]; }

        int pr = getRank[wp];                 // 1 = 2nd rank .. 6 = 7th rank
        if (pr < 6)                            // single push (rank-7 promotion is a terminal win)
            r |= kpkDB[kpkIdx(them, wk, wp - 8, bk)];
        if (pr == 1 && (wp - 8) != wk && (wp - 8) != bk)   // double push from the 2nd rank
            r |= kpkDB[kpkIdx(them, wk, wp - 16, bk)];
    }
    else
    {
        U64 b = kingAttacks[bk];
        while (b) { int s = getLSFBIndex(b); popBit(b, s); r |= kpkDB[kpkIdx(them, wk, wp, s)]; }
    }

    return (r & Good) ? Good : (r & KPK_UNKNOWN) ? KPK_UNKNOWN : Bad;
}

void initKPK() {
    // terminal / invalid classification
    for (int stm = 0; stm < 2; stm++)
        for (int wp = 0; wp < 64; wp++)
            for (int wk = 0; wk < 64; wk++)
                for (int bk = 0; bk < 64; bk++)
                {
                    int pr = getRank[wp];
                    signed char res;

                    if (wk == bk || wk == wp || bk == wp || kpkCheby(wk, bk) <= 1
                        || pr == 0 || pr == 7
                        || (stm == white && (pawnAttacks[white][wp] & (1ULL << bk))))
                        res = KPK_INVALID;
                    else if (stm == white && pr == 6 && (wp - 8) != wk
                             && (kpkCheby(bk, wp - 8) > 1 || kpkCheby(wk, wp - 8) == 1))
                        res = KPK_WIN;             // safe promotion
                    else if (stm == black)
                    {
                        U64 ka = kingAttacks[bk];
                        U64 noSafe = ka & ~(kingAttacks[wk] | pawnAttacks[white][wp]);
                        bool stalemate = (noSafe == 0);
                        bool capturePawn = (ka & (1ULL << wp)) && !(kingAttacks[wk] & (1ULL << wp));
                        res = (stalemate || capturePawn) ? KPK_DRAW : KPK_UNKNOWN;
                    }
                    else
                        res = KPK_UNKNOWN;

                    kpkDB[kpkIdx(stm, wk, wp, bk)] = res;
                }

    // iterate to a fixpoint
    bool changed = true;
    while (changed)
    {
        changed = false;
        for (int stm = 0; stm < 2; stm++)
            for (int wp = 0; wp < 64; wp++)
                for (int wk = 0; wk < 64; wk++)
                    for (int bk = 0; bk < 64; bk++)
                    {
                        int idx = kpkIdx(stm, wk, wp, bk);
                        if (kpkDB[idx] != KPK_UNKNOWN)
                            continue;
                        signed char r = (signed char)kpkClassify(stm, wk, wp, bk);
                        if (r != KPK_UNKNOWN) { kpkDB[idx] = r; changed = true; }
                    }
    }
}

// probe in normalized coords (strong = white, pawn on files a-d); stm 0 = strong to move
bool kpkProbe(int wk, int wp, int bk, int stm) {
    return kpkDB[kpkIdx(stm, wk, wp, bk)] == KPK_WIN;
}

// set file or rank mask
U64 setFileOrRankMask(int file, int rank)
{
    // get the file or rank mask
    U64 mask = 0ULL;

    // loop over all squares via ranks and files
    for (int rankNumber = 0; rankNumber < 8; rankNumber ++)
    {
        for (int fileNumber = 0; fileNumber < 8; fileNumber ++)
        {
            // get square
            int square = rankNumber * 8 + fileNumber;

            // if there is a file
            if (file != -1)
            {
                // if file is the inputed file
                if (fileNumber == file)
                {
                    // set the bit on mask
                    mask |= setBit(mask, square);
                }
            }
            // if there is a rank
            else if (rank != -1)
            {
                // if file is the inputed file
                if (rankNumber == rank)
                {
                    // set the bit on mask
                    mask |= setBit(mask, square);
                }
            }
        }
    }

    // return the mask
    return mask;
}

void initAdjacentFilesMasks() {
    for (int file = 0; file < 8; file++) {
        adjacentFilesMask[file] = 0ULL;

        // Add left file if not on a-file
        if (file > 0) {
            adjacentFilesMask[file] |= fileMask[file - 1];
        }

        // Add right file if not on h-file
        if (file < 7) {
            adjacentFilesMask[file] |= fileMask[file + 1];
        }
    }
}

// initialise evaluation masks
void initEvalMasks()
{
    // get all file + rank masks + isolated pawn masks + passed pawn masks

    // loop over all squares via ranks and files
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // create file mask for square
            fileMask[square] |= setFileOrRankMask(file, -1);

            // create rank mask for square
            rankMask[square] |= setFileOrRankMask(-1, rank);

            // create isolated pawn mask
            isolatedMask[square] |= setFileOrRankMask(file - 1, -1);
            isolatedMask[square] |= setFileOrRankMask(file + 1, -1);

            // support square

            // calculate support squares for white
            if (file > 0 && rank < 7) // avoid edge cases on the left and bottom edges of the board
            {
                // bottom left support square
                setBit(whiteSupportMask[square], square + 7);
            }
            if (file < 7 && rank < 7) // avoid edge cases on the right and bottom edges of the board
            {
                // bottom right support square
                setBit(whiteSupportMask[square], square + 9);
            }

            // calculate support squares for black
            if (file > 0 && rank > 0) // avoid edge cases on the left and bottom edges of the board
            {
                // top left support square
                setBit(blackSupportMask[square], square - 9);
            }
            if (file < 7 && rank > 0) // avoid edge cases on the right and bottom edges of the board
            {
                // bottom right support square
                setBit(blackSupportMask[square], square - 7);
            }

            // phalanx mask

            // calculate phalanx squares
            if (file > 0) // avoid edge cases on the left and bottom edges of the board
            {
                // left phalanx square
                setBit(phalanxMask[square], square - 1);
            }
            if (file < 7) // avoid edge cases on the right and bottom edges of the board
            {
                // right phalanx square
                setBit(phalanxMask[square], square + 1);
            }
        }
    }
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // white passed pawn masks
            whitePassedMask[square] |= setFileOrRankMask(file - 1, -1);
            whitePassedMask[square] |= setFileOrRankMask(file, -1);
            whitePassedMask[square] |= setFileOrRankMask(file + 1, -1);

            // white opposed pawn mask
            whiteOpposedMask[square] |= setFileOrRankMask(file, -1);

            // loop over wrong ranks
            for (int i = 0; i < (8 - rank); i++)
            {
                // reset wrong bits
                whitePassedMask[square] &= ~rankMask[(7 - i) * 8 + file];
                whiteOpposedMask[square] &= ~rankMask[(7 - i) * 8 + file];
            }

            // black passed pawn masks
            blackPassedMask[square] |= setFileOrRankMask(file - 1, -1);
            blackPassedMask[square] |= setFileOrRankMask(file, -1);
            blackPassedMask[square] |= setFileOrRankMask(file + 1, -1);

            // black opposed mask
            blackOpposedMask[square] |= setFileOrRankMask(file, -1);

            // loop over wrong ranks
            for (int i = 0; i < rank + 1; i++)
            {
                // reset wrong bits
                blackPassedMask[square] &= ~rankMask[i * 8 + file];
                blackOpposedMask[square] &= ~rankMask[i * 8 + file];
            }
        }
    }

    // set up king zone masks

    // loop through all squares on board
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // add squares king can reach
            whiteKingZoneMask[square] |= kingAttacks[square];
            blackKingZoneMask[square] |= kingAttacks[square];

            // add three squares in front

            // add left squares
            if (file > 0) // ensure it's not the first file
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 17));
                }


                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 15));
                }
            }
            else // king on first file (A)
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 14));
                }
                if (rank > 0)
                {
                    setBit(whiteKingZoneMask[square], (square - 6));
                }
                if (rank < 7)
                {
                    setBit(whiteKingZoneMask[square], (square + 10));
                }

                setBit(whiteKingZoneMask[square], (square + 2));



                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 18));
                }
                if (rank < 7)
                {
                    setBit(blackKingZoneMask[square], (square + 10));
                }
                if (rank > 0)
                {
                    setBit(blackKingZoneMask[square], (square - 6));
                }

                setBit(blackKingZoneMask[square], (square + 2));

            }


            // add right squares
            if (file < 7) // ensure it's not the first file
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 15));
                }


                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 17));
                }
            }
            else // king on last file (H)
            {
                // for white
                if (rank > 1)
                {
                    setBit(whiteKingZoneMask[square], (square - 18));
                }
                if (rank > 0)
                {
                    setBit(whiteKingZoneMask[square], (square - 10));
                }
                if (rank < 7)
                {
                    setBit(whiteKingZoneMask[square], (square + 6));
                }


                setBit(whiteKingZoneMask[square], (square - 2));


                // for black
                if (rank < 6)
                {
                    setBit(blackKingZoneMask[square], (square + 14));
                }
                if (rank < 7)
                {
                    setBit(blackKingZoneMask[square], (square + 6));
                }
                if (rank > 0)
                {
                    setBit(blackKingZoneMask[square], (square - 10));
                }


                setBit(blackKingZoneMask[square], (square - 2));

            }



            // add middle squares

            // for white
            if (rank > 1)
            {
                setBit(whiteKingZoneMask[square], (square - 16));
            }


            // for black
            if (rank < 6)
            {
                setBit(blackKingZoneMask[square], (square + 16));
            }

        }
    }
    // loop over all squares
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // for white
            if (rank > 0)
            {
                setBit(whiteBlockedMask[square], (square - 8));
            }

            // for black
            if (rank < 7)
            {
                setBit(blackBlockedMask[square], (square + 8));
            }
        }
    }
    // loop over all squares for pinned masks
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            for (int direction = 0; direction < 8; direction++)
            {
                // get square
                int square = rank * 8 + file;

                // make mask
                U64 mask = 0;

                // make bitboard with square
                U64 bitboard = 1ULL << square;

                // make shifted bitboard
                U64 shiftedBitboard = shift(bitboard, direction);

                while (shiftedBitboard)
                {
                    // update mask with shifted board
                    mask |= shiftedBitboard;
                    // get new shifted bitboard
                    shiftedBitboard = shift(shiftedBitboard, direction);
                }
                // init pinnedmask for current square and direction
                pinnedMasks[direction][square] = mask;
            }

        }
    }

    // get forward ranks bb
    // loop over all squares
    for (int rank = 0; rank < 8; rank ++)
    {
        for (int file = 0; file < 8; file ++)
        {
            // get square
            int square = rank * 8 + file;

            // for white
            for (int r = rank - 1; r >= 0; r--)
            {
                forwardRanksMasks[white][square] |= setFileOrRankMask(-1, r);
            }
            // for black
            for (int r = rank + 1; r <= 8; r++)
            {
                forwardRanksMasks[black][square] |= setFileOrRankMask(-1, r);
            }

        }
    }
    // create adjacent files masks
    initAdjacentFilesMasks();

    // now that fileMask and rankMask are filled, we can build the derived globals
    centerFiles = fileMask[c1] | fileMask[d1] | fileMask[e1] | fileMask[f1];
    outpostRanksWhite = rankMask[a4] | rankMask[a5] | rankMask[a6];
    outpostRanksBlack = rankMask[a5] | rankMask[a4] | rankMask[a3];

    // king-flank file groups
    U64 fA = fileMask[a1], fB = fileMask[b1], fC = fileMask[c1], fD = fileMask[d1];
    U64 fE = fileMask[e1], fF = fileMask[f1], fG = fileMask[g1], fH = fileMask[h1];
    U64 qSide = fA | fB | fC | fD, kSide = fE | fF | fG | fH, cFiles = fC | fD | fE | fF;
    kingFlankMask[0] = qSide ^ fD;   // a-file king: files a-c
    kingFlankMask[1] = qSide;
    kingFlankMask[2] = qSide;
    kingFlankMask[3] = cFiles;
    kingFlankMask[4] = cFiles;
    kingFlankMask[5] = kSide;
    kingFlankMask[6] = kSide;
    kingFlankMask[7] = kSide ^ fE;   // h-file king: files f-h

    // per-side camp: own half plus the middle rank, excluding the enemy's back three ranks
    campMask[white] = 0ULL;
    campMask[black] = 0ULL;
    for (int sq = 0; sq < 64; sq++)
    {
        int r = getRank[sq];        // 0 = rank 1, 7 = rank 8
        if (r <= 4) campMask[white] |= (1ULL << sq);   // ranks 1-5
        if (r >= 3) campMask[black] |= (1ULL << sq);   // ranks 4-8
    }

    // precompute strictly-between masks for fast blocker/pin detection. Built from magic
    // slider attacks (orientation agnostic): the squares both endpoints can see when each
    // is blocked by the other are exactly the squares strictly between them.
    for (int s1 = 0; s1 < 64; s1++)
    {
        for (int s2 = 0; s2 < 64; s2++)
        {
            U64 b1 = 1ULL << s1, b2 = 1ULL << s2;
            if (getBishopAttacks(s1, 0ULL) & b2)
            {
                betweenMask[s1][s2] = getBishopAttacks(s1, b2) & getBishopAttacks(s2, b1);
                lineBB[s1][s2] = (getBishopAttacks(s1, 0ULL) & getBishopAttacks(s2, 0ULL)) | b1 | b2;
            }
            else if (getRookAttacks(s1, 0ULL) & b2)
            {
                betweenMask[s1][s2] = getRookAttacks(s1, b2) & getRookAttacks(s2, b1);
                lineBB[s1][s2] = (getRookAttacks(s1, 0ULL) & getRookAttacks(s2, 0ULL)) | b1 | b2;
            }
            else
            {
                betweenMask[s1][s2] = 0ULL;
                lineBB[s1][s2] = 0ULL;
            }
        }
    }
}

// scale attacks on king based off of attack value and number of attackers
int scaleAttacks(AttackInfo info) {
  int result = 0;
  int numberAttackers = info.numberAttackers;
  int attackValue = info.valueAttacks;

  switch (numberAttackers) {
  case 0:
    result = attackValue * attackWeight[0] / 100;
    break;
  case 1:
    result = attackValue * attackWeight[1] / 100;
    break;
  case 2:
    result = attackValue * attackWeight[2] / 100;
    break;
  case 3:
    result = attackValue * attackWeight[3] / 100;
    break;
  case 4:
    result = attackValue * attackWeight[4] / 100;
    break;
  default:
    result = attackValue * 2;
  }

  return -result;
}

// get number of attackers and attack value on king zone for side (disregarding their own pieces except for their pawns)
AttackInfo getAttackInfo(int kingSide)
{
    AttackInfo info;
    info.numberAttackers = 0;
    info.valueAttacks = 0;
    info.numberAttacks = 0;

    // for white
    if (!kingSide)
    {
        // get white king square
        int kingSquare = getLSFBIndex(bitboards[K]);

        // check knight attacks

        // get black knights bitboard
        U64 bitboard = bitboards[n];
        while (bitboard)
        {
            // get square of current knight
            int attackerSquare = getLSFBIndex(bitboard);

            U64 overlap = knightAttacks[attackerSquare] & whiteKingZoneMask[kingSquare] & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;
            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[N];
                info.numberAttackers++;
            }
            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check bishop attacks
        // get black bishops bitboard
        bitboard = bitboards[b];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getBishopAttacks(attackerSquare, (occupancies[white] | bitboards[p]));
            U64 overlap = (attacks & whiteKingZoneMask[kingSquare]) & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[B];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check rook attacks
        // get black rook bitboard
        bitboard = bitboards[r];

        while (bitboard)
        {
            // get square of current rook
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getRookAttacks(attackerSquare, (occupancies[white] | bitboards[p]));
            U64 overlap = (attacks & whiteKingZoneMask[kingSquare]) & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[R];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check queen attacks
        // get black queen bitboard
        bitboard = bitboards[q];

        while (bitboard)
        {
            // get square of current queen
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getQueenAttacks(attackerSquare, (occupancies[white] | bitboards[p]));
            U64 overlap = (attacks & whiteKingZoneMask[kingSquare]) & ~occupancies[black];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[Q];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }
    }

    // for black
    else
    {
        // get white king square
        int kingSquare = getLSFBIndex(bitboards[k]);

        // check knight attacks

        // get white knights bitboard
        U64 bitboard = bitboards[N];
        while (bitboard)
        {
            // get square of current knight
            int attackerSquare = getLSFBIndex(bitboard);

            U64 overlap = knightAttacks[attackerSquare] & blackKingZoneMask[kingSquare] & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[N];
                info.numberAttackers++;
            }
            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check bishop attacks
        // get white bishops bitboard
        bitboard = bitboards[B];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getBishopAttacks(attackerSquare, (occupancies[black] | bitboards[P]));
            U64 overlap = (attacks & blackKingZoneMask[kingSquare]) & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[B];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check rook attacks
        // get white rook bitboard
        bitboard = bitboards[R];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getRookAttacks(attackerSquare, (occupancies[black] | bitboards[P]));
            U64 overlap = (attacks & blackKingZoneMask[kingSquare]) & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[R];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

        // check queen attacks
        // get white queen bitboard
        bitboard = bitboards[Q];

        while (bitboard)
        {
            // get square of current bishop
            int attackerSquare = getLSFBIndex(bitboard);

            U64 attacks = getQueenAttacks(attackerSquare, (occupancies[black] | bitboards[P]));
            U64 overlap = (attacks & blackKingZoneMask[kingSquare]) & ~occupancies[white];
            int count = countBits(overlap);
            info.numberAttacks += count;

            if (overlap)
            {
                info.valueAttacks += KingAttackWeights[Q];
                info.numberAttackers++;
            }

            // pop bit
            popBit(bitboard, attackerSquare);
        }

    }

    return info;
}

// function that evaluates the pawn penalty [file][white or black] (does both pawn storms and pawn shield)
int KingPawnPenalty(int file, int side)
{
    int penalty = 0;

    // white pawn
    if (side == white)
    {
        /* pawn shield */
        // white pawn hasnt moved
        if (getBit(bitboards[P], 6 * 8 + file));
        // pawn moved once
        else if (getBit(bitboards[P], 5 * 8 + file))
        {
            penalty -= 10;
        }
        // pawn moved more than once
        else if (fileMask[file] & bitboards[P])
        {
            penalty -= 20;
        }
        // no pawn on file
        else
        {
            penalty -= 25;
        }

        /* pawn storm + open file towards king */
        // if no enemy pawn -> open file
        if (!(fileMask[file] & bitboards[p]))
        {
            penalty -= 15;
        }
        // enemy pawn on 3rd rank
        else if (getBit(bitboards[p], 5 * 8 + file))
        {
            penalty -= 10;
        }
        // enemy pawn on 4th rank
        else if (getBit(bitboards[p], 4 * 8 + file))
        {
            penalty -= 5;
        }
    }
    // black pawn
    else
    {
        /* pawn shield */
        // black pawn hasnt moved
        if (getBit(bitboards[p], 1 * 8 + file));
        // pawn moved once
        else if (getBit(bitboards[p], 2 * 8 + file))
        {
            penalty -= 10;
        }
        // pawn moved more than once
        else if (fileMask[file] & bitboards[p])
        {
            penalty -= 20;
        }
        // no pawn on file
        else
        {
            penalty -= 25;
        }

        /* pawn storm + open file towards king */
        // if no enemy pawn -> open file
        if (!(fileMask[file] & bitboards[P]))
        {
            penalty -= 15;
        }
        // enemy (white) pawn on 3rd rank from black's perspective
        else if (getBit(bitboards[P], 2 * 8 + file))
        {
            penalty -= 10;
        }
        // enemy (white) pawn on 4th rank from black's perspective
        else if (getBit(bitboards[P], 3 * 8 + file))
        {
            penalty -= 5;
        }
    }
    return penalty;
}

// get relative rank for side
int relativeRank(int side, int rank)
{
    if (side == white)
    {
        // relative rank for white is the same
        return rank;
    }
    else
    {
        // inverse rank for black
        return 7 - rank;
    }
}

// Helper function to get piece type at a square
int getPieceType(int square, int side) {
    for (int piece = (side == white ? P : p); piece <= (side == white ? K : k); piece++) {
        if (getBit(bitboards[piece], square))
            return (piece % 6); // Convert to 0=pawn, 1=knight, 2=bishop, 3=rook, 4=queen, 5=king
    }
    return -1; // Empty square
}

// test king eval
void testKingEval()
{
    // king penalty
    // print attack info
    AttackInfo infoWhite = getAttackInfo(white);
    AttackInfo infoBlack = getAttackInfo(black);

    printf("white attack info: \nnumberAttackers: %d\nvalueAttackers:%d\nnumberAttacks:%d\n", infoWhite.numberAttackers, infoWhite.valueAttacks, infoWhite.numberAttacks);
    printf("black attack info: \nnumberAttackers: %d\nvalueAttackers:%d\nnumberAttacks:%d\n", infoBlack.numberAttackers, infoBlack.valueAttacks, infoBlack.numberAttacks);

    // print king penalty stuff
    int kingPenaltyWhite = 0;
    int kingPenaltyBlack = 0;

}
