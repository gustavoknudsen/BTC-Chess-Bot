#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "defs.h"
#include "bitboard.h"
#include "attacks.h"
#include "position.h"
#include "move.h"
#include "copy_make.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

                Move Generation

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// is square under attack function
static inline int isUnderAttack(int square, int attackingSide)
{
    // if attacked by white pawn
    if ((attackingSide == white) && (pawnAttacks[black][square] & bitboards[P])) return 1;
    // if attacked by black pawn
    if ((attackingSide == black) && (pawnAttacks[white][square] & bitboards[p])) return 1;

    // if attacked by knight
    if (knightAttacks[square] & ((attackingSide == white) ? bitboards[N] : bitboards[n])) return 1;

    // if attacked by king
    if (kingAttacks[square] & ((attackingSide == white) ? bitboards[K] : bitboards[k])) return 1;

    // if attacked by bishop
    if (getBishopAttacks(square, occupancies[both]) & ((attackingSide == white) ? bitboards[B] : bitboards[b])) return 1;

    // if attacked by rook
    if (getRookAttacks(square, occupancies[both]) & ((attackingSide == white) ? bitboards[R] : bitboards[r])) return 1;

    // if attacked by queen
    if (getQueenAttacks(square, occupancies[both]) & ((attackingSide == white) ? bitboards[Q] : bitboards[q])) return 1;

    // if hasn't returned yet, no attacks on square
    return 0;
}

// print attacked squares (for testing)
void printAttackedSquares(int sideAttacker);

// generate captures only (including capture-promotions and en passant).
// non-capture promotions and castling are intentionally skipped.
// used by quiescence search.
static inline void generateCaptures(moves *moveList)
{
    moveList->count = 0;

    int sourceSquare;
    int targetSquare;
    U64 bitboard;
    U64 attacks;

    for (int piece = P; piece <= k; piece++)
    {
        bitboard = bitboards[piece];

        if (side == white)
        {
            // white pawn captures (and capture-promotions)
            if (piece == P)
            {
                while (bitboard)
                {
                    sourceSquare = getLSFBIndex(bitboard);

                    attacks = pawnAttacks[side][sourceSquare] & occupancies[black];
                    while (attacks)
                    {
                        targetSquare = getLSFBIndex(attacks);

                        if (sourceSquare >= a7 && sourceSquare <= h7)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, Q, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, R, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, N, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, B, 1, 0, 0, 0));
                        }
                        else
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                        }

                        popBit(attacks, targetSquare);
                    }

                    if (enpassant != noSq)
                    {
                        U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);
                        if (enpassantAttacks)
                        {
                            int targetEnpassant = getLSFBIndex(enpassantAttacks);
                            addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    popBit(bitboard, sourceSquare);
                }
            }
        }
        else
        {
            // black pawn captures (and capture-promotions)
            if (piece == p)
            {
                while (bitboard)
                {
                    sourceSquare = getLSFBIndex(bitboard);

                    attacks = pawnAttacks[side][sourceSquare] & occupancies[white];
                    while (attacks)
                    {
                        targetSquare = getLSFBIndex(attacks);

                        if (sourceSquare >= a2 && sourceSquare <= h2)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, q, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, r, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, n, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, b, 1, 0, 0, 0));
                        }
                        else
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                        }

                        popBit(attacks, targetSquare);
                    }

                    if (enpassant != noSq)
                    {
                        U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);
                        if (enpassantAttacks)
                        {
                            int targetEnpassant = getLSFBIndex(enpassantAttacks);
                            addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    popBit(bitboard, sourceSquare);
                }
            }
        }

        // knight captures
        if ((side == white) ? piece == N : piece == n)
        {
            while (bitboard)
            {
                sourceSquare = getLSFBIndex(bitboard);
                attacks = knightAttacks[sourceSquare] & ((side == white) ? occupancies[black] : occupancies[white]);
                while (attacks)
                {
                    targetSquare = getLSFBIndex(attacks);
                    addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    popBit(attacks, targetSquare);
                }
                popBit(bitboard, sourceSquare);
            }
        }

        // bishop captures
        if ((side == white) ? piece == B : piece == b)
        {
            while (bitboard)
            {
                sourceSquare = getLSFBIndex(bitboard);
                attacks = getBishopAttacks(sourceSquare, occupancies[both]) &
                            ((side == white) ? occupancies[black] : occupancies[white]);
                while (attacks)
                {
                    targetSquare = getLSFBIndex(attacks);
                    addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    popBit(attacks, targetSquare);
                }
                popBit(bitboard, sourceSquare);
            }
        }

        // rook captures
        if ((side == white) ? piece == R : piece == r)
        {
            while (bitboard)
            {
                sourceSquare = getLSFBIndex(bitboard);
                attacks = getRookAttacks(sourceSquare, occupancies[both]) &
                            ((side == white) ? occupancies[black] : occupancies[white]);
                while (attacks)
                {
                    targetSquare = getLSFBIndex(attacks);
                    addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    popBit(attacks, targetSquare);
                }
                popBit(bitboard, sourceSquare);
            }
        }

        // queen captures
        if ((side == white) ? piece == Q : piece == q)
        {
            while (bitboard)
            {
                sourceSquare = getLSFBIndex(bitboard);
                attacks = getQueenAttacks(sourceSquare, occupancies[both]) &
                            ((side == white) ? occupancies[black] : occupancies[white]);
                while (attacks)
                {
                    targetSquare = getLSFBIndex(attacks);
                    addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    popBit(attacks, targetSquare);
                }
                popBit(bitboard, sourceSquare);
            }
        }

        // king captures
        if ((side == white) ? piece == K : piece == k)
        {
            while (bitboard)
            {
                sourceSquare = getLSFBIndex(bitboard);
                attacks = kingAttacks[sourceSquare] & ((side == white) ? occupancies[black] : occupancies[white]);
                while (attacks)
                {
                    targetSquare = getLSFBIndex(attacks);
                    addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    popBit(attacks, targetSquare);
                }
                popBit(bitboard, sourceSquare);
            }
        }
    }
}

// generate all moves (not all legal)
static inline void generateMoves(moves *moveList)
{
    // create move count
    moveList->count = 0;

    // creating move's source square and target square
    int sourceSquare; int targetSquare;

    // create a copy of piece's bitboard and attacks
    U64 bitboard; U64 attacks;

    // loop over all bitboards
    for (int piece = P; piece <= k; piece++)
    {
        // assign correct bitboard
        bitboard = bitboards[piece];

        // generate white pawns and white king castling
        if (side == white)
        {
            // if white pawn
            if (piece == P)
            {
                // loop over all white pawns
                while (bitboard) // loop until bitboard has no more bits (no more white pawns)
                {
                    // update source and target squares
                    sourceSquare = getLSFBIndex(bitboard);
                    targetSquare = sourceSquare - 8; // white pawn one square forward

                    // get all quiet pawn moves (no captures)
                    if (!(targetSquare < a8) && !getBit(occupancies[both], targetSquare)) // target square exists and no other piece in front
                    {
                        // pawn promotion
                        if (sourceSquare >= a7 && sourceSquare <= h7)
                        {
                            // add move into a move list
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, Q, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, R, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, N, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, B, 0, 0, 0, 0));
                        }
                        else
                        {
                            // add single pawn push to move list
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));

                            // add double pawn push to move list
                            if ((sourceSquare >= a2 && sourceSquare <= h2) && !getBit(occupancies[both], targetSquare - 8))
                            {
                                addMove(moveList, encodeMove(sourceSquare, targetSquare - 8, piece, 0, 0, 1, 0, 0));
                            }
                        }
                    }
                    // assign value to pawn attacks bitboard
                    attacks = pawnAttacks[side][sourceSquare] & occupancies[black]; // where a black piece is and pawn attacks
                    // pawn captures
                    while (attacks) // loop over all attack bits
                    {
                        // update target square
                        targetSquare = getLSFBIndex(attacks);

                        // if pawn promotion as well
                        if (sourceSquare >= a7 && sourceSquare <= h7)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, Q, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, R, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, N, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, B, 1, 0, 0, 0));
                        }
                        else // not a promotion
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                        }

                        // pop current pawn attack
                        popBit(attacks, targetSquare);
                    }

                    // get enpassant captures
                    if (enpassant != noSq)
                    {
                        // create bitboard with valid enpassant squares
                        U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);

                        // if enpassant is possible
                        if (enpassantAttacks)
                        {
                            // create enpassant target square
                            int targetEnpassant = getLSFBIndex(enpassantAttacks);
                            addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
                        }

                    }

                    // pop the current white pawn
                    popBit(bitboard, sourceSquare);
                }

            }
            // check castling moves
            if (piece == K)
            {
                // king side castling
                if (castle & wk)
                {
                    // empty squares between king and right rook
                    if (!getBit(occupancies[both], f1) && !getBit(occupancies[both], g1))
                    {
                        // make sure king and adjacent square is not under attack
                        if (!isUnderAttack(e1, black) && !isUnderAttack(f1, black))
                        {
                            addMove(moveList, encodeMove(e1, g1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                // queen side castling
                if (castle & wq)
                {
                    // empty squares between king and left rook
                    if (!getBit(occupancies[both], d1) && !getBit(occupancies[both], c1) && !getBit(occupancies[both], b1))
                    {
                        // make sure king and adjacent square is not under attack (and also b1)
                        if (!isUnderAttack(e1, black) && !isUnderAttack(d1, black))
                        {
                            addMove(moveList, encodeMove(e1, c1, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }
        // generate black pawns and black king castling
        else
        {
            // if black pawn
            if (piece == p)
            {
                // loop over all black pawns
                while (bitboard) // loop until bitboard has no more bits (no more black pawns)
                {
                    // update source and target squares
                    sourceSquare = getLSFBIndex(bitboard);
                    targetSquare = sourceSquare + 8; // black pawn one square forward

                    // get all quiet pawn moves (no captures)
                    if (!(targetSquare > h1) && !getBit(occupancies[both], targetSquare)) // target square exists and no other piece in front
                    {
                        // pawn promotion
                        if (sourceSquare >= a2 && sourceSquare <= h2)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, q, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, r, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, n, 0, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, b, 0, 0, 0, 0));
                        }
                        else
                        {
                            // add single pawn push to move list
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));

                            // add double pawn push to move list
                            if ((sourceSquare >= a7 && sourceSquare <= h7) && !getBit(occupancies[both], targetSquare + 8))
                            {
                                addMove(moveList, encodeMove(sourceSquare, targetSquare + 8, piece, 0, 0, 1, 0, 0));
                            }
                        }
                    }
                    // assign value to pawn attacks bitboard
                    attacks = pawnAttacks[side][sourceSquare] & occupancies[white]; // where a black piece is and pawn attacks
                    // pawn captures
                    while (attacks) // loop over all attack bits
                    {
                        // update target square
                        targetSquare = getLSFBIndex(attacks);

                        // if pawn promotion as well
                        if (sourceSquare >= a2 && sourceSquare <= h2)
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, q, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, r, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, n, 1, 0, 0, 0));
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, b, 1, 0, 0, 0));
                        }
                        else // not a promotion
                        {
                            addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                        }

                        // pop current pawn attack
                        popBit(attacks, targetSquare);
                    }

                    // get enpassant captures
                    if (enpassant != noSq)
                    {
                        // create bitboard with valid enpassant squares
                        U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);

                        // if enpassant is possible
                        if (enpassantAttacks)
                        {
                            // create enpassant target square
                            int targetEnpassant = getLSFBIndex(enpassantAttacks);
                            addMove(moveList, encodeMove(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop the current white pawn
                    popBit(bitboard, sourceSquare);
                }
            }
            // check castling moves
            if (piece == k)
            {
                // king side castling
                if (castle & bk)
                {
                    // empty squares between king and right rook
                    if (!getBit(occupancies[both], f8) && !getBit(occupancies[both], g8))
                    {
                        // make sure king and adjacent square is not under attack
                        if (!isUnderAttack(e8, white) && !isUnderAttack(f8, white))
                        {
                            addMove(moveList, encodeMove(e8, g8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
                // queen side castling
                if (castle & bq)
                {
                    // empty squares between king and left rook
                    if (!getBit(occupancies[both], d8) && !getBit(occupancies[both], c8) && !getBit(occupancies[both], b8))
                    {
                        // make sure king and adjacent square is not under attack (and also b1)
                        if (!isUnderAttack(e8, white) && !isUnderAttack(d8, white))
                        {
                            addMove(moveList, encodeMove(e8, c8, piece, 0, 0, 0, 0, 1));
                        }
                    }
                }
            }
        }
        // generate knight moves
        if ((side == white) ? piece == N : piece == n)
        {
            // loop over all knights on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = knightAttacks[sourceSquare] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate bishop moves
        if ((side == white) ? piece == B : piece == b)
        {
            // loop over all bishops on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = getBishopAttacks(sourceSquare, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate rook moves
        if ((side == white) ? piece == R : piece == r)
        {
            // loop over all rooks on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = getRookAttacks(sourceSquare, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate queen moves
        if ((side == white) ? piece == Q : piece == q)
        {
            // loop over all queens on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = getQueenAttacks(sourceSquare, occupancies[both]) & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }

        // generate king moves
        if ((side == white) ? piece == K : piece == k)
        {
            // loop over all queens on side
            while (bitboard)
            {
                // update source square
                sourceSquare = getLSFBIndex(bitboard);

                // get piece attacks only if it is NOT a same side piece
                attacks = kingAttacks[sourceSquare] & ((side == white) ? ~occupancies[white] : ~occupancies[black]);

                // loop over all attacks
                while (attacks)
                {
                    // get target square
                    targetSquare = getLSFBIndex(attacks);

                    // if non-capture
                    if (!getBit(((side == white) ? occupancies[black] : occupancies[white]), targetSquare))
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    // if capture move
                    else
                    {
                        addMove(moveList, encodeMove(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    }

                    // pop current move
                    popBit(attacks, targetSquare);
                }


                // pop current square
                popBit(bitboard, sourceSquare);
            }
        }
    }
}

#endif // MOVEGEN_H
