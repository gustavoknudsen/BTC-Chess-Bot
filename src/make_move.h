#ifndef MAKE_MOVE_H
#define MAKE_MOVE_H

#include "defs.h"
#include "bitboard.h"
#include "attacks.h"
#include "position.h"
#include "zobrist.h"
#include "move.h"
#include "copy_make.h"
#include "movegen.h"
#include "evaluation.h"

/***********************************************
<><><><><><><><><><><><><><><><><><><><><><><><>

               Make Move

<><><><><><><><><><><><><><><><><><><><><><><><>
***********************************************/

// make move function
static inline int makeMove(int move, int moveType)
{
    // non-captures
    if (moveType == allMoves)
    {
        // copy board
        copyBoard();

        // get move values
        int sourceSq = getSource(move);
        int targetSq = getTarget(move);
        int piece = getPiece(move);
        int promoted = getPromoted(move);
        int capture = getCapture(move);
        int doublePush = getDouble(move);
        int enpass = getEnpassant(move);
        int castling = getCastle(move);

        // move the piece
        popBit(bitboards[piece], sourceSq);
        setBit(bitboards[piece], targetSq);

        // hash piece (remove from source and update new square)
        hashKey ^= pieceKeys[piece][sourceSq]; // remove from source square
        hashKey ^= pieceKeys[piece][targetSq]; // add piece to new square

        // increment fifty move
        fifty++;

        // increment move number
        moveNumber++;

        // if pawn move or capture
        if ((piece == P || piece == p) || capture)
        {
            // reset fifty move
            fifty = 0;
        }

        // hash enpassant remove enpassant from hash
        if (enpassant != noSq)
        {
            hashKey ^= enpassantKeys[enpassant];
        }

        // update enpassant square after any move
        enpassant = noSq;

        // get capture
        if (capture)
        {
            // get range of piece bitboards depending on side
            int startPiece, endPiece;

            // if white to move, get indices for black pieces
            if (side == white)
            {
                startPiece = p;
                endPiece = k;
            }
            else // for black get white piece range
            {
                startPiece = P;
                endPiece = K;
            }

            // loop over enemy's bitboard and remove the bit on the target square if it exists
            for (int bitPiece = startPiece; bitPiece <= endPiece; bitPiece++)
            {
                if (getBit(bitboards[bitPiece], targetSq))
                {
                    // pop bit if it exists on target square
                    popBit(bitboards[bitPiece], targetSq);

                    // remove taken piece from hash
                    hashKey ^= pieceKeys[bitPiece][targetSq];

                    // break out of loop as piece has been found
                    break;
                }
            }

        }

        // if promotion
        if (promoted)
        {
            // white to move
            if (side == white)
            {
                // erase pawn from target square in bitboard
                popBit(bitboards[P], targetSq);

                // remove pawn from hash key
                hashKey ^= pieceKeys[P][targetSq];
            }
            // black
            else
            {
                // erase pawn from target square in bitboard
                popBit(bitboards[p], targetSq);

                // remove pawn from hash key
                hashKey ^= pieceKeys[p][targetSq];
            }

            // place the promoted piece on the board
            setBit(bitboards[promoted], targetSq);

            // add promoted piece to hash
            hashKey ^= pieceKeys[promoted][targetSq];
        }

        // enpassant
        if (enpass)
        {
            // erase the taken pawn + remove hash key
            if (side == white) {
                // erase pawn from bitboard
                popBit(bitboards[p], targetSq + 8);

                // remove pawn from hash key
                hashKey ^= pieceKeys[p][targetSq + 8];
            }
            else
            {
                // erase pawn from bitboard
                popBit(bitboards[P], targetSq - 8);

                // remove pawn from hash key
                hashKey ^= pieceKeys[P][targetSq - 8];
            }
        }

        // if double pawn push
        if (doublePush)
        {
            // white to move
            if (side == white)
            {
                // set enpassant square
                enpassant = targetSq + 8;

                // hash enpassant square
                hashKey ^= enpassantKeys[targetSq + 8];
            }
            // black to move
            else
            {
                // set enpassant square
                enpassant = targetSq - 8;

                // hash enpassant square
                hashKey ^= enpassantKeys[targetSq - 8];
            }
        }

        // if castle
        if (castling)
        {
            // move rook depending on colour and side
            switch (targetSq)
            {
                // white castles king
                case (g1):
                    // move rook on h1
                    popBit(bitboards[R], h1);
                    setBit(bitboards[R], f1);

                    // hash new rook position
                    hashKey ^= pieceKeys[R][h1];
                    hashKey ^= pieceKeys[R][f1];

                    break;
                // white castles queen
                case (c1):
                    popBit(bitboards[R], a1);
                    setBit(bitboards[R], d1);

                    // hash new rook position
                    hashKey ^= pieceKeys[R][a1];
                    hashKey ^= pieceKeys[R][d1];

                    break;

                // black castles king
                case (g8):
                    popBit(bitboards[r], h8);
                    setBit(bitboards[r], f8);

                    // hash new rook position
                    hashKey ^= pieceKeys[r][h8];
                    hashKey ^= pieceKeys[r][f8];

                    break;

                // black castles queen
                case (c8):
                    popBit(bitboards[r], a8);
                    setBit(bitboards[r], d8);

                    hashKey ^= pieceKeys[r][a8];
                    hashKey ^= pieceKeys[r][d8];

                    break;
            }
        }

        // hash castling (remove castling rights from hash key)
        hashKey ^= castleKeys[castle];

        // update castling rights
        castle &= castlingRights[sourceSq]; // in case king/rook was moved
        castle &= castlingRights[targetSq]; // in case rook was captured

        // hash castling back after update
        hashKey ^= castleKeys[castle];

        // update the occupancies
        memset(occupancies, 0ULL, 24); // resets the occupancies (all 0)

        // loop over white piece bitboards
        for (int bitPiece = P; bitPiece <= K; bitPiece++)
        {
            // update white occupancies
            occupancies[white] |= bitboards[bitPiece];
        }
        // loop over black piece bitboards
        for (int bitPiece = p; bitPiece <= k; bitPiece++)
        {
            // update black occupancies
            occupancies[black] |= bitboards[bitPiece];
        }
        // update combined occupancies
        occupancies[both] |= occupancies[white];
        occupancies[both] |= occupancies[black];

        // change the side after move is made
        side ^= 1;

        // hash side variable
        hashKey ^= sideKey;

        initAttacksTotal();
        updateMobilityAreas();

        positionCache.positionHash = ~0ULL;

        // ===== debug hash key incremental update ==== //
        /*
        U64 hashFromScratch = generateHashKey();

        // in case hash key does not match the incrementally updated one
        // interrupt exe
        if (hashKey != hashFromScratch)
        {
            printf("\nmake move\n");
            printf("move: "); printMove(move);
            printBoard();
            printf("hash key should be %llx\n", hashFromScratch);
            getchar();
        }
        */


        // make sure king has not been exposed to a check (not a valid move)
        if (isUnderAttack((side == white) ? getLSFBIndex(bitboards[k]) : getLSFBIndex(bitboards[K]), side))
        {
            // illegal move
            undoBoard();

            // return 0
            return 0;
        }
        else
        {
            // move is made
            return 1;
        }
    }


    // captures only
    else
    {
        // check if capture
        if (getCapture(move))
        {
            // make the move without the check
            return makeMove(move, allMoves);
        }
        else
        {
            // no move is made
            return 0;
        }

    }
    // so all paths have a return, even though none should get here
    //return makeMove(move, allMoves);
}

#endif // MAKE_MOVE_H
