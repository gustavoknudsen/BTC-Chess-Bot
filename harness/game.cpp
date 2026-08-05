#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POSITION_COMMAND_MAX 12288
#define MATE_SCORE 100000

typedef struct {
    Position pos;

    unsigned long long keys[GAME_MOVES_MAX + BOOK_MOVES_MAX + 8];
    int keyCount;

    // "position fen <fen> moves ..." rebuilt as the game goes
    char command[POSITION_COMMAND_MAX];
    int  commandLength;
    int  commandBase;   // length before any moves were appended

    double clock[2];
    int    movesLeft[2];
} GameState;

static void stateInit(GameState *state, const char *fen)
{
    boardParseFen(&state->pos, fen);

    state->keyCount = 0;
    state->keys[state->keyCount++] = state->pos.key;

    state->commandLength = snprintf(state->command, sizeof(state->command),
                                    "position fen %s", fen);
    state->commandBase = state->commandLength;
}

static void stateAppendMove(GameState *state, const char *uci)
{
    if (state->commandLength == state->commandBase)
        state->commandLength += snprintf(state->command + state->commandLength,
                                         sizeof(state->command) - state->commandLength,
                                         " moves");

    state->commandLength += snprintf(state->command + state->commandLength,
                                     sizeof(state->command) - state->commandLength,
                                     " %s", uci);
}

static void statePlay(GameState *state, Move move, const char *uci)
{
    boardMakeMove(&state->pos, move);
    state->keys[state->keyCount++] = state->pos.key;
    stateAppendMove(state, uci);
}

// three occurrences of the same position, counting the current one. only
// positions since the last irreversible move can repeat, so the scan stops at
// the halfmove clock.
static int isThreefold(const GameState *state)
{
    int repeats = 1;
    int limit = state->keyCount - 1 - state->pos.halfmove;
    if (limit < 0)
        limit = 0;

    for (int i = state->keyCount - 2; i >= limit; i--)
        if (state->keys[i] == state->keys[state->keyCount - 1])
            repeats++;

    return repeats >= 3;
}

// a mate score is compared against the resign and draw thresholds as if it
// were a very large centipawn score
static int normalisedScore(const GameMove *move)
{
    if (!move->haveScore)
        return 0;

    if (move->scoreIsMate)
        return move->score >= 0 ? MATE_SCORE - move->score : -MATE_SCORE - move->score;

    return move->score;
}

static void buildGoCommand(const TimeControl *tc, const GameState *state,
                           char *out, int outSize, int *waitMs, int timeMarginMs)
{
    switch (tc->type)
    {
        case TC_MOVETIME:
            snprintf(out, outSize, "go movetime %d", (int)(tc->movetimeMs + 0.5));
            *waitMs = (int)tc->movetimeMs + 10000;
            break;

        case TC_DEPTH:
            snprintf(out, outSize, "go depth %d", tc->depth);
            *waitMs = 600000;
            break;

        case TC_NODES:
            snprintf(out, outSize, "go nodes %lld", tc->nodes);
            *waitMs = 600000;
            break;

        default:
        {
            int white = (int)(state->clock[WHITE] + 0.5);
            int black = (int)(state->clock[BLACK] + 0.5);
            if (white < 1) white = 1;
            if (black < 1) black = 1;

            int increment = (int)(tc->incMs + 0.5);
            int offset = snprintf(out, outSize, "go wtime %d btime %d winc %d binc %d",
                                  white, black, increment, increment);

            if (tc->movesToGo > 0)
                snprintf(out + offset, outSize - offset, " movestogo %d",
                         state->movesLeft[state->pos.side]);

            // the engine is given its whole clock plus the grace period before
            // the harness stops waiting and calls it a loss on time
            double own = state->clock[state->pos.side];
            *waitMs = (int)(own + timeMarginMs + 2000.0);
            break;
        }
    }
}

static void finish(GameRecord *record, int result, const char *reason, int termination)
{
    record->result = result;
    record->termination = termination;
    snprintf(record->reason, sizeof(record->reason), "%s", reason);
}

int playGame(Engine *white, Engine *black, const Opening *opening,
             const GameRules *rules, GameRecord *record, Logger *logger,
             const char *label)
{
    Engine *engines[2] = { white, black };

    memset(record, 0, sizeof(GameRecord));
    snprintf(record->startFen, sizeof(record->startFen), "%s", opening->fen);

    Position check;
    if (!boardParseFen(&check, opening->fen))
    {
        logEvent(logger, "%s: unusable opening fen '%s'", label, opening->fen);
        return 0;
    }

    GameState state;
    stateInit(&state, opening->fen);

    // replay the book moves. these are not attributed to either engine.
    for (int i = 0; i < opening->moveCount; i++)
    {
        Move move;
        if (!boardMoveFromUci(&state.pos, opening->moves[i], &move))
        {
            logEvent(logger, "%s: illegal book move %s in opening %s",
                     label, opening->moves[i], opening->fen);
            return 0;
        }

        GameMove *entry = &record->moves[record->moveCount];
        memset(entry, 0, sizeof(GameMove));
        snprintf(entry->uci, sizeof(entry->uci), "%s", opening->moves[i]);
        boardMoveToSan(&state.pos, move, entry->san, sizeof(entry->san));
        record->moveCount++;
        record->openingPlies++;

        statePlay(&state, move, opening->moves[i]);
    }

    for (int side = 0; side < 2; side++)
    {
        const TimeControl *tc = &engines[side]->config->tc;
        state.clock[side] = tc->timeMs;
        state.movesLeft[side] = tc->movesToGo > 0 ? tc->movesToGo : 0;
    }

    if (!engineNewGame(white) || !engineNewGame(black))
    {
        record->restartWhite = !engineAlive(white);
        record->restartBlack = !engineAlive(black);
        logEvent(logger, "%s: engine failed to accept ucinewgame (%s / %s)",
                 label, white->lastError, black->lastError);
        return 0;
    }

    int resignWhiteStreak = 0;
    int resignBlackStreak = 0;
    int drawStreak = 0;

    while (1)
    {
        int side = state.pos.side;
        Engine *engine = engines[side];

        // terminal positions are decided by the arbiter, never by an engine
        Move legal[MAX_MOVES];
        int legalCount = boardGenerateLegal(&state.pos, legal);

        if (!legalCount)
        {
            if (boardInCheck(&state.pos, side))
                finish(record, side == WHITE ? RES_BLACK_WINS : RES_WHITE_WINS, "checkmate", TERM_NORMAL);
            else
                finish(record, RES_DRAW, "stalemate", TERM_NORMAL);
            return 1;
        }

        if (state.pos.halfmove >= 100)
        {
            finish(record, RES_DRAW, "fifty move rule", TERM_NORMAL);
            return 1;
        }

        if (isThreefold(&state))
        {
            finish(record, RES_DRAW, "threefold repetition", TERM_NORMAL);
            return 1;
        }

        if (boardInsufficientMaterial(&state.pos))
        {
            finish(record, RES_DRAW, "insufficient material", TERM_NORMAL);
            return 1;
        }

        if (rules->maxMoves && state.pos.fullmove > rules->maxMoves)
        {
            finish(record, RES_DRAW, "maximum moves reached", TERM_ADJUDICATION);
            return 1;
        }

        if (record->moveCount >= GAME_MOVES_MAX)
        {
            finish(record, RES_DRAW, "move limit reached", TERM_ADJUDICATION);
            return 1;
        }

        char goCommand[128];
        int waitMs = 600000;
        buildGoCommand(&engine->config->tc, &state, goCommand, sizeof(goCommand),
                       &waitMs, rules->timeMarginMs);

        int ok = engineGo(engine, state.command, goCommand, waitMs);

        char fen[128];
        boardWriteFen(&state.pos, fen, sizeof(fen));

        if (!ok)
        {
            char recent[4096];
            engineRecentOutput(engine, recent, sizeof(recent));

            const char *reason = engineAlive(engine) ? "no answer from engine" : "engine crashed";

            logEvent(logger, "%s: %s (%s) for %s at move %d\n  fen: %s\n  last output:\n%s",
                     label, reason, engine->lastError, engine->config->name,
                     state.pos.fullmove, fen, recent);

            if (side == WHITE) record->restartWhite = 1;
            else               record->restartBlack = 1;

            char text[160];
            snprintf(text, sizeof(text), "%s (%s)", reason, engine->lastError);
            finish(record, side == WHITE ? RES_BLACK_WINS : RES_WHITE_WINS, text, TERM_ENGINE_FAILURE);
            return 1;
        }

        // clock accounting. depth and node limited games have no clock.
        if (engine->config->tc.type == TC_TIME)
        {
            state.clock[side] -= engine->elapsedMs;

            if (state.clock[side] < -(double)rules->timeMarginMs)
            {
                logEvent(logger, "%s: %s lost on time at move %d, %.0f ms over\n  fen: %s",
                         label, engine->config->name, state.pos.fullmove,
                         -state.clock[side], fen);

                finish(record, side == WHITE ? RES_BLACK_WINS : RES_WHITE_WINS, "loss on time", TERM_TIME_LOSS);
                return 1;
            }

            state.clock[side] += engine->config->tc.incMs;

            if (engine->config->tc.movesToGo > 0 && --state.movesLeft[side] == 0)
            {
                state.movesLeft[side] = engine->config->tc.movesToGo;
                state.clock[side] += engine->config->tc.timeMs;
            }
        }

        Move move;
        if (!boardMoveFromUci(&state.pos, engine->bestmove, &move))
        {
            char recent[4096];
            engineRecentOutput(engine, recent, sizeof(recent));

            logEvent(logger, "%s: illegal move '%s' from %s at move %d\n  fen: %s\n  last output:\n%s",
                     label, engine->bestmove, engine->config->name, state.pos.fullmove, fen, recent);

            char text[160];
            snprintf(text, sizeof(text), "illegal move %s", engine->bestmove);
            finish(record, side == WHITE ? RES_BLACK_WINS : RES_WHITE_WINS, text, TERM_ILLEGAL_MOVE);
            return 1;
        }

        GameMove *entry = &record->moves[record->moveCount];
        memset(entry, 0, sizeof(GameMove));
        snprintf(entry->uci, sizeof(entry->uci), "%s", engine->bestmove);
        boardMoveToSan(&state.pos, move, entry->san, sizeof(entry->san));
        entry->haveScore   = engine->haveScore;
        entry->scoreIsMate = engine->scoreIsMate;
        entry->score       = engine->score;
        entry->depth       = engine->depth;
        entry->timeMs      = engine->elapsedMs;
        record->moveCount++;

        statePlay(&state, move, engine->bestmove);

        // adjudication. scores are converted to white's point of view first,
        // and both engines have to agree before anything is adjudicated.
        int score = normalisedScore(entry);
        int whiteScore = side == WHITE ? score : -score;

        if (entry->haveScore)
        {
            if (rules->resignEnabled)
            {
                resignWhiteStreak = whiteScore >=  rules->resignScore ? resignWhiteStreak + 1 : 0;
                resignBlackStreak = whiteScore <= -rules->resignScore ? resignBlackStreak + 1 : 0;

                // one full move is two plies, so both engines are counted
                if (resignWhiteStreak >= rules->resignMoveCount * 2)
                {
                    finish(record, RES_WHITE_WINS, "adjudication: resign", TERM_ADJUDICATION);
                    return 1;
                }

                if (resignBlackStreak >= rules->resignMoveCount * 2)
                {
                    finish(record, RES_BLACK_WINS, "adjudication: resign", TERM_ADJUDICATION);
                    return 1;
                }
            }

            if (rules->drawEnabled && state.pos.fullmove >= rules->drawMoveNumber)
            {
                drawStreak = (whiteScore <= rules->drawScore && whiteScore >= -rules->drawScore)
                             ? drawStreak + 1 : 0;

                if (drawStreak >= rules->drawMoveCount * 2)
                {
                    finish(record, RES_DRAW, "adjudication: drawn score", TERM_ADJUDICATION);
                    return 1;
                }
            }
            else
            {
                drawStreak = 0;
            }
        }
        else
        {
            resignWhiteStreak = resignBlackStreak = drawStreak = 0;
        }
    }
}
