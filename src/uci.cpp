#include "uci.h"
#include "position.h"
#include "move.h"
#include "movegen.h"
#include "make_move.h"
#include "evaluation.h"
#include "search.h"
#include "tt.h"
#include "timeman.h"
#include "version.h"

// adjust for delay (like in lichess)
int moveOverhead = 50;

// parse user or GUI string input (like e4e5 or h7h8q for example)
int parseMove(const char* moveString)
{
    // create move list
    moves moveList[1];

    // generate moves
    generateMoves(moveList);

    // get source square (and convert to int the same way as before)
    int sourceSq = (moveString[0] - 'a') + (8 - (moveString[1] - '0')) * 8;

    // get target square
    int targetSq = (moveString[2] - 'a') + (8 - (moveString[3] - '0')) * 8;

    // loop over move list
    for (int moveCount = 0; moveCount < moveList->count; moveCount++)
    {
        // get current move
        int move = moveList->moves[moveCount];

        // check if source + target exist in move list
        if (sourceSq == getSource(move) && targetSq == getTarget(move))
        {
            // get promoted piece (if it exists)
            int promoted = getPromoted(move);

            // there is a promotion
            if (promoted)
            {
                // if promotes to queen
                if ((promoted == Q || promoted == q) && moveString[4] == 'q')
                {
                    return move;
                }
                // if promotes to rook
                else if ((promoted == R || promoted == r) && moveString[4] == 'r')
                {
                    return move;
                }
                // if promotes to knight
                else if ((promoted == N || promoted == n) && moveString[4] == 'n')
                {
                    return move;
                }
                // if promotes to bishop
                else if ((promoted == B || promoted == b) && moveString[4] == 'b')
                {
                    return move;
                }
                // in case illegal promotion (continue the loop)
                continue;
            }

            // if no promotion, return move
            return move;
        }
    }

    // move not in move list so illegal (or error with move gen)
    return 0;
}

// parse the UCI 'position' protocol
void parsePosition(const char* command)
{
    // shift pointer past 'position '
    command += 9;

    // current character that the pointer is at
    const char *currentChar = command;

    if (strncmp(command, "startpos", 8) == 0)
    {
        // if we find 'startpos', set up starting position
        parseFEN(start_position);
    }
    else // check if it is FEN protocol
    {
        // check for FEN
        currentChar = strstr(command, "fen");

        // if no FEN,
        if (currentChar == NULL)
        {
            parseFEN(start_position);
        }
        // found FEN
        else
        {
            // move pointer to fen string
            currentChar += 4;

            // set up board with fen
            parseFEN(currentChar);
        }
    }

    // get moves if they exist
    currentChar = strstr(command, "moves");

    // if moves found
    if (currentChar != NULL)
    {
        // shift pointer until move
        currentChar += 6;

        // loop all moves in this move string
        while(*currentChar)
        {
            // skip any leading whitespace (uci allows any whitespace between moves)
            while (*currentChar == ' ' || *currentChar == '\t')
            {
                currentChar++;
            }

            // stop if we ran off the end after skipping whitespace
            if (*currentChar == '\0')
            {
                break;
            }

            // parse the next move
            int move = parseMove(currentChar);

            // if no more moves
            if (move == 0)
            {
                break;
            }

            // increment repetition (we undo this if makeMove rejects the move)
            repetitionIndex++;

            // write hash into rep table
            repetitionTable[repetitionIndex] = hashKey;

            // make move on board, undo repetition bump if the move is illegal
            if (makeMove(move, allMoves) == 0)
            {
                repetitionIndex--;
                break;
            }

            // move pointer to the next move (until it reaches whitespace)
            while(*currentChar && *currentChar != ' ' && *currentChar != '\t')
            {
                currentChar++;
            }
        }
    }
    // update attacks
    initAttacksTotal();

    // printboard()
}

// reset time variables
void resetTimeControl()
{
    // reset timing
    quit = 0;
    movestogo = 30;
    movetime = -1;
    time = -1;
    inc = 0;
    starttime = 0;
    stoptime = 0;
    timeset = 0;
    stopped = 0;
}

int slowMover = 100;   // percentage (100 = normal speed)

// parse the UCI 'go' protocol
void parseGo(const char *command)
{
    // reset variables
    resetTimeControl();

    // create depth variable
    int depth = -1;

    // create current depth pointer
    const char *argument = NULL;

    // infinite search
    if ((argument = strstr(command,"infinite"))) {}

    // match UCI "binc" command
    if ((argument = strstr(command,"binc")) && side == black)
    {
        // parse black time increment
        inc = atoi(argument + 5);
    }

    // match UCI "winc" command
    if ((argument = strstr(command,"winc")) && side == white)
    {
        // parse white time increment
        inc = atoi(argument + 5);
    }

    // match UCI "wtime" command
    if ((argument = strstr(command,"wtime")) && side == white)
    {
        // parse white time limit
        time = atoi(argument + 6);
    }

    // match UCI "btime" command
    if ((argument = strstr(command,"btime")) && side == black)
    {
        // parse black time limit
        time = atoi(argument + 6);
    }

    // match UCI "movestogo" command
    if ((argument = strstr(command,"movestogo")))
    {
        // parse number of moves to go
        movestogo = atoi(argument + 10);
    }

    // match UCI "movetime" command
    if ((argument = strstr(command,"movetime")))
    {
        // parse amount of time allowed to spend to make a move
        movetime = atoi(argument + 9);
    }

    // for fixed depth search
    if (argument = strstr(command, "depth"))
    {
        // move pointer to depth then convert string to integer and then update depth variable
        depth = atoi(argument + 6);
    }

    // if there is no move time
    if(movetime != -1)
    {
        // set time equal to move time
        time = movetime;

        // set moves to go to 1
        movestogo = 1;
    }

    // start measuring time
    starttime = getTime();

    // get depth
    depth = depth;

    // if there is time control
    if(time != -1)
    {
        // flag of time control
        timeset = 1;

        // Maximum move horizon (similar to Stockfish)
        int mtg = movestogo ? std::min(movestogo, 50) : 50;

        // Calculate time left with overhead considered
        int timeLeft = time + inc * (mtg - 1) - moveOverhead * (2 + mtg);
        if (timeLeft < 0) timeLeft = 1;

        // Apply slow mover factor
        timeLeft = (timeLeft * slowMover) / 100;

        // Dynamic time scaling based on position and game phase.
        // optScale is a fraction of timeLeft we plan to spend on this move.
        double optScale;
        if (movestogo == 0) {
            // sudden death
            optScale = std::min(0.01 + std::sqrt(moveNumber + 3) * 0.004,
                               0.2 * time / (double)timeLeft);
        } else {
            // X moves in Y seconds
            optScale = std::min((0.9 + moveNumber / 120.0) / mtg,
                                0.9 * time / (double)timeLeft);
        }

        // soft target time and hard cap. soft is what we aim for. hard is
        // the wall we never cross. for short time controls the formula
        // above can over-commit, so we also enforce a sanity cap of
        // 1/20 of remaining time as the soft target floor.
        int optimalTime = (int)(optScale * timeLeft);
        int sanityCap = (time - moveOverhead) / 20;
        if (sanityCap > 0 && optimalTime > sanityCap) optimalTime = sanityCap;
        if (optimalTime < 1) optimalTime = 1;

        // hard cap: never spend more than 1/4 of remaining time on a single
        // move, and never more than 2.5x the optimum (since extending past
        // that is rarely worth it). this is the actual stoptime.
        int hardCap = std::min((int)((time - moveOverhead) / 4),
                               (int)(2.5 * optimalTime));
        if (hardCap < optimalTime) hardCap = optimalTime;

        // soft target for iterative deepening to consult between iterations
        softLimit = optimalTime;

        // stoptime is the HARD cap that communicate() compares against
        stoptime = starttime + hardCap;

        // emergency: at very low clock, just play instantly with a tiny
        // budget so we do not flag
        if (time < 1500) {
            int emergencyTime = time / 2 + inc - moveOverhead;
            if (emergencyTime < 10) emergencyTime = 10;
            stoptime = starttime + emergencyTime;
            softLimit = emergencyTime;
        }
    }

    // if depth is not available
    if(depth == -1)
    {
        // set depth to 64 plies (takes ages to complete...)
        depth = 64;
    }

        // print debug info
    //printf("time:%d start:%d stop:%d depth:%d timeset:%d\n",
    //time, starttime, stoptime, depth, timeset);

    // search position
    search(depth);
}

// UCI loop for communication
void uciLoop()
{
    // init max hash size and default mb
    int maxHash = 128;
    int mb = 64;

    // reset STDIN and STDOUT buffers
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);

    // define user or GUI input buffer (large number in case of large input)
    char input[2000];
    /*
    // print information
    printf("id name BetterThanCris 2.2\n");
    printf("id author Gustavo Knudsen\n");
    printf("uciok\n");
    fflush(stdout);
    */

    // loop
    while (1)
    {
        // reset input
        memset(input, 0, sizeof(input));

        // send output to gui
        fflush(stdout);

        // get input
        if (!fgets(input, 2000, stdin))
        {
            // continue the loop (returned NULL and no code should be executed)
            continue;
        }

        // trim newline character and null characters
        input[strcspn(input, "\n")] = 0;
        input[strcspn(input, "\x00")] = 0;

        // check 'isready' from uci
        if (strncmp(input, "isready", 7) == 0)
        {
            // answer with readyok to show that the engine is ready
            printf("readyok\n");
            fflush(stdout);
            continue;
        }

        // check position from uci
        else if (strncmp(input, "position", 8) == 0)
        {
            // parse position command
            parsePosition(input);
        }

        // check 'ucinewgame' command
        else if (strncmp(input, "ucinewgame", 10) == 0)
        {
            // clear tt and search heuristics on new game
            clearTT();
            clearSearchHeuristics();

            // call position with 'startpos'
            parsePosition("position startpos");
        }

        // check 'go' command
        else if (strncmp(input, "go", 2) == 0)
        {
            // parse go command
            parseGo(input);
        }

        // check UCI 'quit' command
        else if (strncmp(input, "quit", 4) == 0)
        {
            // exit (break)
            break;
        }

        // check uci command
        else if (strncmp(input, "uci", 3) == 0)
        {
            // print info (name, version, author all live in version.h)
            printf("id name %s %s\n", ENGINE_NAME, ENGINE_VERSION);
            printf("id author %s\n", ENGINE_AUTHOR);
            printf("uciok\n");
            printf("option name Move Overhead type spin default 10 min 0 max 5000\n");


            fflush(stdout);
        }

        // hash value option
        else if (!strncmp(input, "setoption name Hash value ", 26))
        {
            // init MB
            sscanf(input, "%*s %*s %*s %*s %d", &mb);

            // adjust mb if beyond bounds
            if (mb < 4) mb = 4;
            if (mb > maxHash) mb = maxHash;

            // set tt table size
            printf("Set hash table size to %dMB\n", mb);
            fflush(stdout);
            initTT(mb);
        }
        // move overhead option
        else if (!strncmp(input, "setoption name Move Overhead value ", 34))
        {
            // init move overhead
            sscanf(input, "%*s %*s %*s %*s %d", &moveOverhead);

            // adjust move overhead if beyond reasonable bounds
            if (moveOverhead < 0) moveOverhead = 0;
            if (moveOverhead > 1000) moveOverhead = 1000;

            // set move overhead
            printf("Set move overhead to %dms\n", moveOverhead);
            fflush(stdout);
        }


    }
}
