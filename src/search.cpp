#include "search.h"
#include "bitboard.h"
#include "position.h"
#include "move.h"
#include "movegen.h"
#include "make_move.h"
#include "copy_make.h"
#include "evaluation.h"
#include "tt.h"
#include "timeman.h"

// table for mvvlva
/*

    (Victims) Pawn  Knight  Bishop  Rook  Queen  King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/

// Most Valuable Victim - Least Valuable Attacker [attacker][victim]
int mvvlva[12][12] = {
 	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

	105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
	104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
	103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
	102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
	101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
	100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600
};

// 2 killer moves [id][ply]
int killerMoves[2][maxPly];

// history moves
int historyMoves[12][64];

/*
      ================================
            Triangular PV table
      --------------------------------
        PV line: e2e4 e7e5 g1f3 b8c6
      ================================

           0    1    2    3    4    5

      0    m1   m2   m3   m4   m5   m6

      1    0    m2   m3   m4   m5   m6

      2    0    0    m3   m4   m5   m6

      3    0    0    0    m4   m5   m6

      4    0    0    0    0    m5   m6

      5    0    0    0    0    0    m6
*/

// PV length [ply]
int PVLength[maxPly];

// PV table [ply][ply]
int PVTable[maxPly][maxPly];

// follow PV and score PV (if follow = 1, follow pv, 0 we don't)
int followPV, scorePV;

// enable pv move scoring
static inline void enablePVScoring(moves *moveList)
{
    // disable pv follow
    followPV = 0;

    // loop over moves
    for (int count = 0; count < moveList->count; count++)
    {
        // make sure its a pv move
        if (PVTable[0][ply] == moveList->moves[count])
        {
            // enable move scoring
            scorePV = 1;

            // enable following (since if we find this pv move, we continue in pv)
            followPV = 1;
        }
    }
}

/*  =======================
         Move ordering
    =======================

    1. PV move
    2. Captures in MVV/LVA
    3. 1st killer move
    4. 2nd killer move
    5. History moves
    6. Unsorted moves
*/

// score move function (for move ordering)
static inline int scoreMove(int move)
{
    // if pv move scoring is enabled
    if (scorePV)
    {
        // check if we are in pv
        if (PVTable[0][ply] == move)
        {
            // disable score flag
            scorePV = 0;

            // return high for pv move so it is searched first
            return 20000;
        }
    }

    if (getCapture(move))
    {
        // create target piece variable
        int targetPiece = P;

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
            if (getBit(bitboards[bitPiece], getTarget(move)))
            {
                targetPiece = bitPiece;
                // break out of loop as piece has been found
                break;
            }
        }

        // score move using mvvlva[source][target]
        return mvvlva[getPiece(move)][targetPiece] + 10000;
    }

    //score quiet move
    else
    {
        // score 1st killer move
        if (killerMoves[0][ply] == move)
        {
            return 9000;
        }

        // score 2nd killer move
        else if (killerMoves[1][ply] == move)
        {
            return 8000;
        }

        // score history move
        else
        {
            return historyMoves[getPiece(move)][getTarget(move)];
        }
    }

    return 0;
}

// sort moves (for better pruning)
static inline void sortMoves(moves *moveList, int best)
{
    // create move scores
    int moveScores[moveList->count];

    // score all moves
    for (int count = 0; count < moveList->count; count++)
    {
        // if hash has best move
        if (best == moveList->moves[count])
        {
            // score move (for priority)
            moveScores[count] = 30000;
        }
        else
        {
            // score move normally
            moveScores[count] = scoreMove(moveList->moves[count]);

        }
    }

    // loop over move in list (specialMovesFound depending on if there was a PV move)
    for (int current = 0; current < moveList->count; current++)
    {
        // loop over next move
        for (int next = current + 1; next < moveList->count; next++)
        {
            // compare current and next move
            if (moveScores[current] < moveScores[next])
            {
                // swap scores
                int tempScore = moveScores[current];
                moveScores[current] = moveScores[next];
                moveScores[next] = tempScore;

                // swap moves
                int tempMove = moveList->moves[current];
                moveList->moves[current] = moveList->moves[next];
                moveList->moves[next] = tempMove;
            }
        }
    }
}

// print move scores
void printMoveScores(moves *moveList)
{
    for (int count = 0; count < moveList->count; count++)
    {
        printMove(moveList->moves[count]);
        printf(" score: %d\n", scoreMove(moveList->moves[count]));
    }
}

// repetition detection
static inline int isRepetition()
{
    // loop over repetition index range
    for (int index = 0; index < repetitionIndex; index++)
    {
        if (repetitionTable[index] == hashKey)
        {
            return 1;
        }
    }

    // if no repetition
    return 0;
}

// quiescence search (to stop the horizon effect)
static inline int quiescence(int alpha, int beta)
{
    // every 2047 nodes check for user input
    if ((nodes & 2047) == 0)
    {
        // listen to GUI or user input
        communicate();
    }

    // add nodes
    nodes++;

    // drop search if max ply
    if (ply > maxPly - 1)
    {
        // just evaluate position
        return evaluate();
    }

    // evaluate position
    int eval = evaluate();

    // fail-hard beta cutoff (score can't go outside of alpha beta bounds)
    if (eval >= beta)
    {
        // node fails high
        return beta;
    }

    // if found a better move
    if (eval > alpha)
    {
        // PV (principal variation) node
        alpha = eval;
    }

    // create moves list
    moves moveList[1];

    // gen moves
    generateMoves(moveList);

    // sort moves mvvlva
    sortMoves(moveList, 0);

    // loop over moves
    for (int count = 0; count < moveList->count; count++)
    {
        // copy the board
        copyBoard();

        // increment ply
        ply++;

        // increment repetition & store hash
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        // only make legal moves
        if (makeMove(moveList->moves[count], capturesOnly) == 0)
        {
            // decrease ply
            ply--;

            // decrease repetition & store hash
            repetitionIndex--;

            // skip
            continue;
        }

        // score the move
        int score = -quiescence(-beta, -alpha);

        // decrease ply
        ply--;

        // decrease repetition & store hash
        repetitionIndex--;

        // undo move
        undoBoard();

        // return 0 if time is up
        if (stopped == 1)
        {
            return 0;
        }

        // if found a better move
        if (score > alpha)
        {
            // PV (principal variation) node
            alpha = score;

            // fail-hard beta cutoff (score can't go outside of alpha beta bounds)
            if (score >= beta)
            {
                // node fails high
                return beta;
            }
        }
    }
    // node fails low
    return alpha;
}

extern const int fullDepthMoves = 5;
extern const int reductionLimit = 2;

// negamax alpha beta search
static inline int negamax(int alpha, int beta, int depth)
{
    // create PV length
    PVLength[ply] = ply;

    // create move variable
    // (the inner-loop redeclares "move" with the same name, so this outer
    //  one must be initialised for the final recordHash call below)
    int move = 0, score = 0;

    // best move
    int bestMove = 0;

    // get hash flag (initially as alpha flag)
    int hashFlag = hashFlagAlpha;

    // if position repetition occurs
    // (both checks are gated on ply, otherwise the root can return draw
    //  without producing a bestmove)
    if (ply && (isRepetition() || fifty >= 100))
        // return draw score
        return 0;

    // mate distance pruning
    // if we already found a mate, don't search for a slower one
    // alpha bound: at this ply, the worst we can return is being mated now
    // beta bound: at this ply, the best we can return is mating in 1 ply
    if (ply)
    {
        if (alpha < -mateValue + ply) alpha = -mateValue + ply;
        if (beta > mateValue - ply - 1) beta = mateValue - ply - 1;
        if (alpha >= beta) return alpha;
    }

    // check if pv node
    int pvNode = beta - alpha > 1;

    // always probe the tt so bestMove is populated for move ordering
    // (the previous form short-circuited on ply == 0 and skipped the probe at root)
    // only return the stored score for non-root, non-pv nodes
    score = probeHash(alpha, beta, depth, &bestMove);
    if (ply && pvNode == 0 && score != noHashEntry)
    {
        // return score of the move without search
        return score;
    }

    // check for input from gui or user every 2047 nodes
    if ((nodes & 2047) == 0)
    {
        // listen to inputs
        communicate();
    }

    // recursion escape
    if (depth == 0)
    {
        // run quiescence search
        return quiescence(alpha, beta);
    }

    // drop search if max ply
    if (ply > maxPly - 1)
    {
        // just evaluate position
        return evaluate();
    }

    // increment nodes
    nodes++;

    // incheck?
    int inCheck = isUnderAttack((side == white) ? getLSFBIndex(bitboards[K]) :
                                                        getLSFBIndex(bitboards[k]),
                                                        side ^ 1);



    // if in check, increase depth
    if (inCheck)
    {
        depth++;
    }

    // legal moves counter
    int legalMoves = 0;

    // get static eval
    int eval = evaluate();

    /*
        skips moves if material balance plus gain of the move and safety margin
        does not improve alpha
    */
    // reverse futility pruning (RFP or static null move pruning)
    // do not prune in mate territory, otherwise we can prune away a mate
    // (the previous guard "abs(beta - 1) > -infinity + 100" was always true
    //  since abs is non-negative and -infinity + 100 is negative)
	if (depth < 3 && !pvNode && !inCheck && abs(beta) < mateScore)
	{
        // define evaluation margin
		int evalMargin = 120 * depth;

		// evaluation margin substracted from static evaluation score fails high
		if (eval - evalMargin >= beta)
		    // evaluation margin substracted from static evaluation score
			return eval - evalMargin;
	}

    // null move pruning
    if (depth >= 3 && inCheck == 0 && ply)
    {
        // copy board
        copyBoard();

        // increment ply to sync
        ply++;

        // increment repetition & store hash
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        // update hash key
        if (enpassant != noSq)
        {
            hashKey ^= enpassantKeys[enpassant];
        }

        // reset enpassant square
        enpassant = noSq;

        // switch the side to give opponent a second move
        side ^= 1;

        // hash the side
        hashKey ^= sideKey;

        // search with less depth (depth - 1 - R, R=2)
        score = -negamax(-beta, -beta + 1, depth - 1 - 2);

        // bring back sync
        ply--;

        // decrease repetition & store hash
        repetitionIndex--;

        // restore board and variables
        undoBoard();

        // return 0 if time is up
        if (stopped == 1)
        {
            return 0;
        }

        // check fail hard beta cutoff
        if (score >= beta)
        {
            // node fails high
            return beta;
        }
    }

    /*
        If the static evaluation indicates a fail-low node, but q-search fails high,
        the score of the reduced fail-high search is returned, since there was obviously
        a winning capture raising the score
        from: https://www.chessprogramming.org/Razoring
    */
    // razoring (same idea as implemented in Strelka)
    if (!pvNode && !inCheck && depth <= 3)
    {
        // get static eval and add bonus
        score = eval + 125;

        // get new score
        int newScore;

        // static evaluation indicates a fail-low node
        if (score < beta)
        {
            // on depth 1
            if (depth == 1)
            {
                // get quiscence score
                newScore = quiescence(alpha, beta);

                // return quiescence score if it's greater then static evaluation score
                return (newScore > score) ? newScore : score;
            }

            // add second bonus to static evaluation
            score += 175;

            // static evaluation indicates a fail-low node
            if (score < beta && depth <= 2)
            {
                // get quiscence score
                newScore = quiescence(alpha, beta);

                // quiescence score indicates fail-low node
                if (newScore < beta)
                    // return quiescence score if it's greater then static evaluation score
                    return (newScore > score) ? newScore : score;
            }
        }
	}

    // create moves list
    moves moveList[1];

    // gen moves
    generateMoves(moveList);

    // if we are following pv line
    if (followPV)
    {
        // enable pv score
        enablePVScoring(moveList);
    }

    // sort moves mvvlva
    sortMoves(moveList, bestMove);

    // number of moves searched
    int movesSearched = 0;

    // loop over moves
    for (int count = 0; count < moveList->count; count++)
    {
        int move = moveList->moves[count];

        // copy the board
        copyBoard();

        // increment ply
        ply++;

        // increment repetition & store hash
        repetitionIndex++;
        repetitionTable[repetitionIndex] = hashKey;

        // only make legal moves
        if (makeMove(moveList->moves[count], allMoves) == 0)
        {
            // decrease ply
            ply--;

            // decrease repetition & store hash
            repetitionIndex--;

            // skip
            continue;
        }

        // add legal move
        legalMoves++;

        // opponent in check flag (for lmr)
        int opponentInCheck = isUnderAttack((side == white) ? getLSFBIndex(bitboards[K]) :
                                                        getLSFBIndex(bitboards[k]),
                                                        side ^ 1);


        // if no moves searched
        if (movesSearched == 0)
        {
            // do a normal search
            score = -negamax(-beta, -alpha, depth -1);
        }
        else
        {
            // condition to initiate late move reductions
            if (movesSearched >= fullDepthMoves &&
                        depth >= reductionLimit &&
                        inCheck == 0 && pvNode == 0)
            {
                // if move is promotion or capture,
                if (getPromoted(move) || getCapture(move))
                {
                    // if moves give check, search w reduced depth of 2
                    if (opponentInCheck)
                    {
                        score = -negamax(-alpha - 1, -alpha, depth - 1 - 2);
                    }
                    else // if move does not give check, reduce depth by 3
                    {
                        score = -negamax(-alpha - 1, -alpha, depth - 1 - 3);
                    }
                }
                else // quiet move
                {

                    // search this move with reduced depth
                    // movesSearched is the count of legal moves already searched at this node,
                    // which is what the Ethereal formula expects, not the game's halfmove counter
                    double depthAdjustment = 0.7844 + std::log(depth) * std::log(movesSearched) / 2.4696;
                    // Ensure the resulting depth is an integer and not less than 1 (values taken from Ethereal)
                    int adjustedDepth = static_cast<int>(std::max(1.0, depth - 1 - depthAdjustment));
                    score = -negamax(-alpha - 1, -alpha, adjustedDepth);

                }
            }
            else // ensure full-depth search
            {
                score = alpha + 1;
            }
            // PVS (principal variation search)
            if (score > alpha)
            {
                // once you find a move with a score between alpha and beta
                // search for the rest of the moves trying to prove they are bad
                // this is faster than searching thinking the other moves are good
                score = -negamax(-alpha - 1, - alpha, depth - 1);

                // if it finds out it is wrong (there is a better move)
                // you have to search with normal alpha beta again
                // this is a waste of time but happens not that often so overall it's worth it
                if ((score > alpha) && (score < beta))
                {
                    // search normally again if move happened to be better than pv
                    score = -negamax(-beta, -alpha, depth-1);
                }
            }
        }

        // decrease ply
        ply--;

        // decrease repetition & store hash
        repetitionIndex--;

        // undo move
        undoBoard();

        // return 0 if time is up
        if (stopped == 1)
        {
            return 0;
        }

        // add move searched
        movesSearched++;

        // if found a better move
        if (score > alpha)
        {
            // switch hash flag to pv node
            hashFlag = hashFlagExact;

            // store best move
            bestMove = moveList->moves[count];

            // only store history moves on quiet moves
            if (getCapture(moveList->moves[count]) == 0)
            {
                // store history moves
                historyMoves[getPiece(moveList->moves[count])][getTarget(moveList->moves[count])] += depth;
            }
            // PV (principal variation) node
            alpha = score;

            // write PV move
            PVTable[ply][ply] = moveList->moves[count];

            // loop over next ply
            for (int nextPly = ply + 1; nextPly < PVLength[ply + 1]; nextPly++)
            {
                // copy move from deeper ply and add to current ply's line
                PVTable[ply][nextPly] = PVTable[ply + 1][nextPly];
            }

            // update PV length
            PVLength[ply] = PVLength[ply + 1];

            // fail-hard beta cutoff (score can't go outside of alpha beta bounds)
            if (score >= beta)
            {
                // store hash with beta flag and beta value
                recordHash(beta, depth, hashFlagBeta, moveList->moves[count], bestMove);

                // on quiet moves
                if (getCapture(moveList->moves[count]) == 0)
                {
                    // store killer moves
                    killerMoves[1][ply] = killerMoves[0][ply];
                    killerMoves[0][ply] = moveList->moves[count];
                }

                // node fails high
                return beta;
            }
        }
    }

    // no legal moves currently
    if (legalMoves == 0)
    {
        // king in check (chekmate)
        if (inCheck)
        {
            // return mating score (+ ply to find checkmates when using larger depths)
            return -mateValue + ply;
        }
        // king not in check (stalemate)
        else
        {
            // return stalemate score
            return 0;
        }
    }

    // store hash with alpha flag and alpha scoreb
    recordHash(alpha, depth, hashFlag, move, bestMove);

    // node fails low
    return alpha;
}

// search position
void search(int depth)
{
    // create score
    int score = 0;

    // reset node counter
    nodes = 0;

    // reset no more time flag
    stopped = 0;

    // reset pv follow and pv scores
    followPV = 0;
    scorePV = 0;

    // clear helper data structures for search
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(historyMoves, 0, sizeof(historyMoves));
    memset(PVTable, 0, sizeof(PVTable));
    memset(PVLength, 0, sizeof(PVLength));

    // give initial alpha and beta values
    int alpha = -infinity;
    int beta = infinity;

    // iterative deepening
    for (int currentDepth = 1; currentDepth <= depth; currentDepth++)
    {
        // no more time
        if (stopped == 1)
        {
            // stop calculating
            break;
        }

        // activate follow pv flag
        followPV = 1;

        // find best move
        score = negamax(alpha, beta, currentDepth);

        // we are outside of the window so we try again with a full window
        if ((score <= alpha) || (score >= beta))
        {
            alpha = -infinity;
            beta = infinity;
            continue;
        }

        // decrease window for next node
        alpha = score - 50;
        beta = score + 50;

        // print score/mate info
        // if pv length is not 0

        if (PVLength[0])
        {
            if (score > mateScore || score < -mateScore)
            {
                printf("info score mate %d ", (score > 0) ? ((mateValue-score)/2+1) : (-mateValue+(mateValue-score)/2));
            }
            else
            {
                printf("info score cp %d ", score);
            }

            // print depth, nodes, pv info
            printf("depth %d nodes %lld pv ", currentDepth, nodes);

            // loop over moves in pv line
            for (int count = 0; count < PVLength[0]; count++)
            {
                // print move (PVTable[0] has the principle variation)
                printMove(PVTable[0][count]);
                printf(" ");
            }
            printf("\n");
        }

    }


    // best move placeholder
    printf("bestmove ");
    printMove(PVTable[0][0]);
    printf("\n");
}
