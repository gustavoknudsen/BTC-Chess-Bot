#include "sprt.h"

#include <math.h>
#include <string.h>

void statsInit(Stats *stats)
{
    memset(stats, 0, sizeof(Stats));
}

void statsAddPair(Stats *stats, int gameOne, int gameTwo)
{
    int results[2] = { gameOne, gameTwo };
    double points = 0.0;

    for (int i = 0; i < 2; i++)
    {
        if (results[i] > 0)      { stats->wins++;   points += 1.0; }
        else if (results[i] < 0) { stats->losses++; }
        else                     { stats->draws++;  points += 0.5; }
    }

    int index = (int)(points * 2.0 + 0.5);
    if (index < 0) index = 0;
    if (index > 4) index = 4;

    stats->pentanomial[index]++;
}

double eloToScore(double elo)
{
    return 1.0 / (1.0 + pow(10.0, -elo / 400.0));
}

double scoreToElo(double score)
{
    if (score <= 0.0)   return -800.0;
    if (score >= 1.0)   return  800.0;
    return -400.0 * log10(1.0 / score - 1.0);
}

void sprtConfigure(SprtConfig *config, double elo0, double elo1, double alpha, double beta)
{
    config->enabled = 1;
    config->elo0  = elo0;
    config->elo1  = elo1;
    config->alpha = alpha;
    config->beta  = beta;

    // Wald's boundaries for the log likelihood ratio
    config->lowerBound = log(beta / (1.0 - alpha));
    config->upperBound = log((1.0 - beta) / alpha);

    if (config->minPairs <= 0)
        config->minPairs = 16;
}

/*
    An outcome nobody has produced yet would give that bucket a probability of
    exactly zero, and a run where every pair scored identically would give a
    variance of exactly zero, which leaves the ratio undefined. Mixing a tiny
    prior into the empty buckets keeps the estimate defined without moving it:
    a thousandth of an observation cannot compete with real counts.
*/
#define BUCKET_PRIOR 1e-3

// mean score per game and the variance of that mean.
// pairs are used when available: the two games of a pair share an opening, so
// treating them as one observation removes the opening's contribution to the
// variance instead of counting it twice.
static int scoreMoments(const Stats *stats, double *mean, double *varianceOfMean)
{
    long long observedPairs = 0;
    for (int i = 0; i < 5; i++)
        observedPairs += stats->pentanomial[i];

    double values[5];
    double counts[5];
    int buckets;
    double total = 0.0;

    if (observedPairs >= 2)
    {
        buckets = 5;
        for (int i = 0; i < 5; i++)
        {
            values[i] = (double)i / 4.0;        // points per game in the pair
            counts[i] = (double)stats->pentanomial[i];
        }
    }
    else
    {
        long long games = stats->wins + stats->draws + stats->losses;
        if (games < 2)
            return 0;

        buckets = 3;
        values[0] = 0.0;   counts[0] = (double)stats->losses;
        values[1] = 0.5;   counts[1] = (double)stats->draws;
        values[2] = 1.0;   counts[2] = (double)stats->wins;
    }

    for (int i = 0; i < buckets; i++)
    {
        if (counts[i] == 0.0)
            counts[i] = BUCKET_PRIOR;
        total += counts[i];
    }

    double sum = 0.0, sumSquares = 0.0;

    for (int i = 0; i < buckets; i++)
    {
        sum        += counts[i] * values[i];
        sumSquares += counts[i] * values[i] * values[i];
    }

    double mu = sum / total;
    double variance = sumSquares / total - mu * mu;

    if (variance <= 0.0)
        return 0;

    *mean = mu;
    *varianceOfMean = variance / total;
    return 1;
}

/*
    Log likelihood ratio between the two hypotheses, in the normal
    approximation used by every practical engine testing tool.

    The observed mean score is normal around the true score with variance
    sigma squared. For a known variance the log likelihood ratio between
    "true score is s1" and "true score is s0" reduces to

        LLR = (s1 - s0) * (2 * mean - s0 - s1) / (2 * sigma squared)

    s0 and s1 are the scores that correspond to elo0 and elo1 under the
    logistic Elo model.
*/
double sprtLLR(const Stats *stats, const SprtConfig *config)
{
    double mean = 0.0, varianceOfMean = 0.0;

    if (!scoreMoments(stats, &mean, &varianceOfMean))
        return 0.0;

    double s0 = eloToScore(config->elo0);
    double s1 = eloToScore(config->elo1);

    return (s1 - s0) * (2.0 * mean - s0 - s1) / (2.0 * varianceOfMean);
}

int sprtVerdict(double llr, const SprtConfig *config)
{
    if (!config->enabled)
        return SPRT_CONTINUE;

    if (llr >= config->upperBound)
        return SPRT_H1_ACCEPTED;

    if (llr <= config->lowerBound)
        return SPRT_H0_ACCEPTED;

    return SPRT_CONTINUE;
}

/*
    The first few pairs of a match can easily land in a single bucket, and a
    sample with no spread makes the estimated variance meaningless: the ratio
    would run off to a decision on a handful of games. Nothing is decided
    before a minimum number of pairs has been seen.
*/
int sprtDecide(const Stats *stats, const SprtConfig *config, double *llrOut)
{
    double llr = sprtLLR(stats, config);

    if (llrOut)
        *llrOut = llr;

    long long pairs = 0;
    for (int i = 0; i < 5; i++)
        pairs += stats->pentanomial[i];

    if (pairs < config->minPairs)
        return SPRT_CONTINUE;

    return sprtVerdict(llr, config);
}

void summarise(const Stats *stats, const SprtConfig *config, Summary *out)
{
    memset(out, 0, sizeof(Summary));

    out->games = stats->wins + stats->draws + stats->losses;
    for (int i = 0; i < 5; i++)
        out->pairs += stats->pentanomial[i];

    if (!out->games)
        return;

    out->score = ((double)stats->wins + 0.5 * (double)stats->draws) / (double)out->games;
    out->drawRatio = (double)stats->draws / (double)out->games;
    out->elo = scoreToElo(out->score);

    double mean = 0.0, varianceOfMean = 0.0;
    if (scoreMoments(stats, &mean, &varianceOfMean))
    {
        double sigma = sqrt(varianceOfMean);

        // delta method: convert the confidence interval on the score into one
        // on the Elo difference through the derivative of the logistic map
        double derivative = (400.0 / log(10.0)) / (mean * (1.0 - mean));
        out->ci95 = 1.96 * sigma * derivative;

        out->los = 0.5 * (1.0 + erf((mean - 0.5) / (sigma * sqrt(2.0))));
    }
    else
    {
        out->los = 0.5;
    }

    if (config && config->enabled)
        out->llr = sprtLLR(stats, config);
}
