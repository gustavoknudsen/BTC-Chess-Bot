#ifndef SEE_H
#define SEE_H

#include "defs.h"
#include "bitboard.h"
#include "attacks.h"
#include "position.h"
#include "move.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

         Static Exchange Evaluation
         (Stockfish-style null-window swap)

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// piece values used by SEE only. midgame approximations.
// indexed by piece (P N B R Q K p n b r q k = 0..11). king is huge so the
// swap loop never breaks on king material, the king branch handles legality.
extern const int seePieceValue[12];

// bitboard of all pieces of either colour that attack the given square,
// using occ for slider blockers
static inline U64 attackersTo(int square, U64 occ)
{
    return  (pawnAttacks[black][square] & bitboards[P])
          | (pawnAttacks[white][square] & bitboards[p])
          | (knightAttacks[square] & (bitboards[N] | bitboards[n]))
          | (kingAttacks[square]   & (bitboards[K] | bitboards[k]))
          | (getBishopAttacks(square, occ) &
                (bitboards[B] | bitboards[b] | bitboards[Q] | bitboards[q]))
          | (getRookAttacks(square, occ) &
                (bitboards[R] | bitboards[r] | bitboards[Q] | bitboards[q]));
}

// Static Exchange Evaluation Greater-or-Equal.
// returns 1 if the swap-off value at the move's target square is >= threshold,
// 0 otherwise. uses a null-window swap algorithm (mirrors Stockfish's see_ge).
//
// non-normal moves (en passant, castling, promotions) are approximated as
// SEE = 0. that is conservative enough for skip-bad-captures filtering.
static inline int seeGe(int move, int threshold)
{
    if (getEnpassant(move) || getCastle(move) || getPromoted(move))
        return 0 >= threshold;

    int from     = getSource(move);
    int to       = getTarget(move);
    int attacker = getPiece(move);

    // find the captured piece (if any) on the to-square
    int captured = -1;
    int startEnemy = (side == white) ? p : P;
    int endEnemy   = (side == white) ? k : K;
    for (int bp = startEnemy; bp <= endEnemy; bp++)
    {
        if (getBit(bitboards[bp], to))
        {
            captured = bp;
            break;
        }
    }

    // initial swap-off value relative to the side that just moved.
    // if even winning the captured piece outright cannot meet the threshold,
    // SEE certainly cannot, so fail.
    int gain = (captured >= 0) ? seePieceValue[captured] : 0;
    int swap = gain - threshold;
    if (swap < 0) return 0;

    // if after losing the moving piece on the recapture we still beat the
    // threshold (swap <= 0), we succeed regardless of any further exchange.
    swap = seePieceValue[attacker] - swap;
    if (swap <= 0) return 1;

    U64 occ = occupancies[both] ^ (1ULL << from) ^ (1ULL << to);
    U64 attackers = attackersTo(to, occ);
    int stm = side;
    int res = 1;

    while (1)
    {
        stm ^= 1;
        attackers &= occ;

        U64 myPieces = (stm == white) ? occupancies[white] : occupancies[black];
        U64 sideAttackers = attackers & myPieces;
        if (!sideAttackers)
            break;

        // tentatively flip the result; if the recapture is too lossy we abort
        res ^= 1;

        U64 bb;
        int pieceP = (stm == white) ? P : p;
        int pieceN = (stm == white) ? N : n;
        int pieceB = (stm == white) ? B : b;
        int pieceR = (stm == white) ? R : r;
        int pieceQ = (stm == white) ? Q : q;

        if ((bb = sideAttackers & bitboards[pieceP]))
        {
            if ((swap = seePieceValue[P] - swap) < res) break;
            occ ^= bb & (0ULL - bb);
            attackers |= getBishopAttacks(to, occ) &
                            (bitboards[B] | bitboards[b] | bitboards[Q] | bitboards[q]);
        }
        else if ((bb = sideAttackers & bitboards[pieceN]))
        {
            if ((swap = seePieceValue[N] - swap) < res) break;
            occ ^= bb & (0ULL - bb);
        }
        else if ((bb = sideAttackers & bitboards[pieceB]))
        {
            if ((swap = seePieceValue[B] - swap) < res) break;
            occ ^= bb & (0ULL - bb);
            attackers |= getBishopAttacks(to, occ) &
                            (bitboards[B] | bitboards[b] | bitboards[Q] | bitboards[q]);
        }
        else if ((bb = sideAttackers & bitboards[pieceR]))
        {
            if ((swap = seePieceValue[R] - swap) < res) break;
            occ ^= bb & (0ULL - bb);
            attackers |= getRookAttacks(to, occ) &
                            (bitboards[R] | bitboards[r] | bitboards[Q] | bitboards[q]);
        }
        else if ((bb = sideAttackers & bitboards[pieceQ]))
        {
            if ((swap = seePieceValue[Q] - swap) < res) break;
            occ ^= bb & (0ULL - bb);
            attackers |= (getBishopAttacks(to, occ) &
                            (bitboards[B] | bitboards[b] | bitboards[Q] | bitboards[q]))
                       | (getRookAttacks(to, occ) &
                            (bitboards[R] | bitboards[r] | bitboards[Q] | bitboards[q]));
        }
        else
        {
            // king capture: if the opponent still has a piece attacking us,
            // capturing with the king is illegal so flip the result back
            return (attackers & ~myPieces) ? (res ^ 1) : res;
        }
    }

    return res;
}

#endif // SEE_H
