#include "match.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Tournament *tournament;
    int id;
    Engine engines[2];
    Thread thread;
} Worker;

static volatile int interrupted = 0;

void matchInterrupt(void)
{
    interrupted = 1;
}

// hands out the next opening pair, or -1 when the match is over
static int claimRound(Tournament *tournament)
{
    int round = -1;

    mutexLock(&tournament->lock);

    int limit = tournament->options->rounds;

    if (!tournament->stop && !tournament->fatal && !interrupted &&
        (limit <= 0 || tournament->nextRound < limit))
        round = tournament->nextRound++;

    mutexUnlock(&tournament->lock);
    return round;
}

static void printProgress(Tournament *tournament)
{
    Summary summary;
    summarise(&tournament->stats, &tournament->options->sprt, &summary);

    const Stats *stats = &tournament->stats;
    double elapsed = (timeNowMs() - tournament->startTime) / 1000.0;

    printf("%6lld pairs  %7lld games   +%lld =%lld -%lld  %5.1f%%   penta [%lld %lld %lld %lld %lld]   Elo %+7.1f +- %5.1f",
           summary.pairs, summary.games,
           stats->wins, stats->draws, stats->losses,
           summary.score * 100.0,
           stats->pentanomial[0], stats->pentanomial[1], stats->pentanomial[2],
           stats->pentanomial[3], stats->pentanomial[4],
           summary.elo, summary.ci95);

    if (tournament->options->sprt.enabled)
        printf("   LLR %6.2f [%.2f, %.2f]",
               summary.llr,
               tournament->options->sprt.lowerBound,
               tournament->options->sprt.upperBound);

    if (elapsed > 0.0)
        printf("   %.1f g/s", (double)summary.games / elapsed);

    printf("\n");
    fflush(stdout);
}

static void submitPair(Tournament *tournament, int gameOne, int gameTwo)
{
    mutexLock(&tournament->lock);

    statsAddPair(&tournament->stats, gameOne, gameTwo);

    if (tournament->options->sprt.enabled)
    {
        int verdict = sprtDecide(&tournament->stats, &tournament->options->sprt, NULL);

        if (verdict != SPRT_CONTINUE)
        {
            tournament->verdict = verdict;
            tournament->stop = 1;
        }
    }

    printProgress(tournament);

    mutexUnlock(&tournament->lock);
}

static void countTermination(Tournament *tournament, const GameRecord *record)
{
    mutexLock(&tournament->lock);

    if (record->termination == TERM_ENGINE_FAILURE) tournament->crashes++;
    if (record->termination == TERM_TIME_LOSS)      tournament->timeLosses++;
    if (record->termination == TERM_ILLEGAL_MOVE)   tournament->illegalMoves++;
    if (record->termination == TERM_ADJUDICATION)   tournament->adjudications++;

    mutexUnlock(&tournament->lock);
}

static int restartEngine(Worker *worker, int index)
{
    Tournament *tournament = worker->tournament;

    engineQuit(&worker->engines[index]);

    if (!engineStart(&worker->engines[index], &tournament->options->engines[index]))
    {
        logEvent(tournament->logger, "worker %d: cannot restart %s (%s)",
                 worker->id, tournament->options->engines[index].name,
                 worker->engines[index].lastError);
        return 0;
    }

    return 1;
}

static void abortPair(Tournament *tournament, int fatal)
{
    mutexLock(&tournament->lock);
    tournament->abortedPairs++;
    if (fatal)
        tournament->fatal = 1;
    mutexUnlock(&tournament->lock);
}

static void workerMain(void *arg)
{
    Worker *worker = (Worker *)arg;
    Tournament *tournament = worker->tournament;
    const Options *options = tournament->options;

    for (int i = 0; i < 2; i++)
    {
        if (!engineStart(&worker->engines[i], &options->engines[i]))
        {
            fprintf(stderr, "worker %d: %s\n", worker->id, worker->engines[i].lastError);
            logEvent(tournament->logger, "worker %d: %s", worker->id, worker->engines[i].lastError);
            abortPair(tournament, 1);
            return;
        }
    }

    while (1)
    {
        int round = claimRound(tournament);
        if (round < 0)
            break;

        Opening opening;
        bookOpening(tournament->book, options->bookStart + round, &opening);

        int results[2] = { 0, 0 };
        int completed = 1;

        for (int game = 0; game < 2 && completed; game++)
        {
            // the first game gives white to engine one, the second reverses it
            int whiteIndex = game == 0 ? 0 : 1;
            Engine *white = &worker->engines[whiteIndex];
            Engine *black = &worker->engines[whiteIndex ^ 1];

            // engines that ask for it get a new process for every game
            for (int i = 0; i < 2; i++)
            {
                if (!options->engines[i].restartEveryGame)
                    continue;

                if (!restartEngine(worker, i))
                {
                    abortPair(tournament, 1);
                    return;
                }
            }

            char label[64];
            snprintf(label, sizeof(label), "round %d game %d", round + 1, game + 1);

            GameRecord record;
            int played = playGame(white, black, &opening, &options->rules,
                                  &record, tournament->logger, label);

            if (!played)
            {
                completed = 0;

                if (!restartEngine(worker, 0) || !restartEngine(worker, 1))
                {
                    abortPair(tournament, 1);
                    return;
                }

                abortPair(tournament, 0);
                break;
            }

            char tcText[64];
            timeControlText(&options->engines[whiteIndex].tc, tcText, sizeof(tcText));

            pgnWriteGame(tournament->pgn, &record,
                         options->engines[whiteIndex].name,
                         options->engines[whiteIndex ^ 1].name,
                         round + 1, options->event, tcText);

            countTermination(tournament, &record);

            if (record.result == RES_DRAW)
                results[game] = 0;
            else
                results[game] = ((record.result == RES_WHITE_WINS) == (whiteIndex == 0)) ? 1 : -1;

            if (record.restartWhite && !restartEngine(worker, whiteIndex))
            {
                abortPair(tournament, 1);
                return;
            }

            if (record.restartBlack && !restartEngine(worker, whiteIndex ^ 1))
            {
                abortPair(tournament, 1);
                return;
            }
        }

        if (completed)
            submitPair(tournament, results[0], results[1]);
    }

    for (int i = 0; i < 2; i++)
        engineQuit(&worker->engines[i]);
}

static void printHeader(const Tournament *tournament)
{
    const Options *options = tournament->options;

    char tcOne[64], tcTwo[64];
    timeControlText(&options->engines[0].tc, tcOne, sizeof(tcOne));
    timeControlText(&options->engines[1].tc, tcTwo, sizeof(tcTwo));

    printf("engine 1: %s   [%s]   %s\n", options->engines[0].name,
           options->engines[0].command, tcOne);
    printf("engine 2: %s   [%s]   %s\n", options->engines[1].name,
           options->engines[1].command, tcTwo);

    if (options->bookPath[0])
        printf("openings: %s   %d loaded   %s   starting at %d\n",
               options->bookPath, tournament->book->count,
               options->bookRandom ? "shuffled" : "in order", options->bookStart);
    else
        printf("openings: none, every game starts from the initial position\n");

    printf("rounds:   %s   concurrency %d\n",
           options->rounds > 0 ? "limited" : "until the test decides",
           options->concurrency);

    if (options->sprt.enabled)
        printf("sprt:     H0 elo %.1f, H1 elo %.1f, alpha %.3f, beta %.3f, bounds [%.2f, %.2f]\n",
               options->sprt.elo0, options->sprt.elo1,
               options->sprt.alpha, options->sprt.beta,
               options->sprt.lowerBound, options->sprt.upperBound);
    else
        printf("sprt:     disabled, reporting Elo only\n");

    printf("\n");
    fflush(stdout);
}

static void printReport(const Tournament *tournament)
{
    const Options *options = tournament->options;
    Summary summary;
    summarise(&tournament->stats, &options->sprt, &summary);

    const Stats *stats = &tournament->stats;
    double elapsed = (timeNowMs() - tournament->startTime) / 1000.0;

    printf("\n");
    printf("=================================================================\n");
    printf("%s vs %s\n", options->engines[0].name, options->engines[1].name);
    printf("games      %lld in %lld pairs, %.0f seconds, %.1f games per second\n",
           summary.games, summary.pairs, elapsed,
           elapsed > 0.0 ? (double)summary.games / elapsed : 0.0);
    printf("result     +%lld =%lld -%lld   score %.2f%%   draws %.1f%%\n",
           stats->wins, stats->draws, stats->losses,
           summary.score * 100.0, summary.drawRatio * 100.0);
    printf("pentanomial [LL %lld, LD %lld, DD+WL %lld, WD %lld, WW %lld]\n",
           stats->pentanomial[0], stats->pentanomial[1], stats->pentanomial[2],
           stats->pentanomial[3], stats->pentanomial[4]);
    printf("elo        %+.1f +- %.1f (95%%)   likelihood of superiority %.1f%%\n",
           summary.elo, summary.ci95, summary.los * 100.0);

    if (options->sprt.enabled)
    {
        printf("llr        %.2f   bounds [%.2f, %.2f]\n",
               summary.llr, options->sprt.lowerBound, options->sprt.upperBound);

        if (tournament->verdict == SPRT_H1_ACCEPTED)
            printf("verdict    H1 accepted: the change is worth at least %.1f Elo\n",
                   options->sprt.elo0);
        else if (tournament->verdict == SPRT_H0_ACCEPTED)
            printf("verdict    H0 accepted: the change is not worth %.1f Elo\n",
                   options->sprt.elo1);
        else if (interrupted)
            printf("verdict    interrupted before the test decided\n");
        else
            printf("verdict    inconclusive, the round limit was reached first\n");
    }

    if (tournament->crashes || tournament->timeLosses || tournament->illegalMoves ||
        tournament->abortedPairs)
    {
        printf("anomalies  %lld engine failures, %lld losses on time, %lld illegal moves, "
               "%lld discarded pairs\n",
               tournament->crashes, tournament->timeLosses,
               tournament->illegalMoves, tournament->abortedPairs);

        if (options->logPath[0])
            printf("           details in %s\n", options->logPath);
        else
            printf("           run with -log FILE to record the details\n");
    }

    if (tournament->adjudications)
        printf("adjudicated %lld games\n", tournament->adjudications);

    printf("=================================================================\n");
    fflush(stdout);
}

int matchRun(Tournament *tournament)
{
    const Options *options = tournament->options;

    mutexInit(&tournament->lock);
    statsInit(&tournament->stats);
    tournament->startTime = timeNowMs();

    printHeader(tournament);

    int workerCount = options->concurrency;

    Worker *workers = (Worker *)calloc((size_t)workerCount, sizeof(Worker));
    if (!workers)
    {
        fprintf(stderr, "out of memory allocating %d workers\n", workerCount);
        return 1;
    }

    for (int i = 0; i < workerCount; i++)
    {
        workers[i].tournament = tournament;
        workers[i].id = i + 1;

        if (!threadCreate(&workers[i].thread, workerMain, &workers[i]))
        {
            fprintf(stderr, "cannot start worker %d\n", i + 1);
            tournament->fatal = 1;
            workerCount = i;
            break;
        }
    }

    for (int i = 0; i < workerCount; i++)
        threadJoin(&workers[i].thread);

    printReport(tournament);

    free(workers);
    mutexDestroy(&tournament->lock);

    if (tournament->fatal)
        return 2;

    // an SPRT that accepted H0 is a successful run with a negative answer, so
    // it gets its own exit code for scripting
    if (options->sprt.enabled && tournament->verdict != SPRT_H1_ACCEPTED)
        return 1;

    return 0;
}
