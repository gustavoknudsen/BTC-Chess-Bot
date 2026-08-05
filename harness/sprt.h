#ifndef HARNESS_SPRT_H
#define HARNESS_SPRT_H

/*
    Sequential probability ratio test over game pairs.

    Results are collected pentanomially: every opening is played twice with
    the colours reversed, and the pair result (0, 0.5, 1, 1.5 or 2 points for
    the first engine) is the unit of observation. Pairing removes most of the
    variance that colour and opening choice contribute, so a pentanomial test
    reaches a decision in noticeably fewer games than counting single games.
*/

typedef struct {
    int enabled;
    double elo0;
    double elo1;
    double alpha;
    double beta;
    double lowerBound;   // log(beta / (1 - alpha)), accept H0 at or below
    double upperBound;   // log((1 - beta) / alpha), accept H1 at or above
    long long minPairs;  // no verdict before this many pairs, see sprtDecide
} SprtConfig;

typedef struct {
    long long wins;
    long long draws;
    long long losses;
    long long pentanomial[5];   // LL, LD, DD and WL, WD, WW
} Stats;

typedef struct {
    long long games;
    long long pairs;
    double score;        // points per game for the first engine
    double elo;
    double ci95;
    double los;          // probability the first engine is stronger
    double drawRatio;
    double llr;
} Summary;

enum {
    SPRT_CONTINUE = 0,
    SPRT_H1_ACCEPTED,
    SPRT_H0_ACCEPTED
};

void statsInit(Stats *stats);

// both game results are from the first engine's point of view:
// +1 win, 0 draw, -1 loss
void statsAddPair(Stats *stats, int gameOne, int gameTwo);

void sprtConfigure(SprtConfig *config, double elo0, double elo1, double alpha, double beta);

double sprtLLR(const Stats *stats, const SprtConfig *config);

// pure boundary test on a ratio
int    sprtVerdict(double llr, const SprtConfig *config);

// the test as the match should apply it: the ratio, plus the refusal to decide
// on a sample too small for the variance estimate to mean anything
int    sprtDecide(const Stats *stats, const SprtConfig *config, double *llrOut);

void   summarise(const Stats *stats, const SprtConfig *config, Summary *out);

double eloToScore(double elo);
double scoreToElo(double score);

#endif // HARNESS_SPRT_H
