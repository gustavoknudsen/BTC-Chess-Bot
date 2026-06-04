/*
 * Texel tuning driver for BetterThanCris.
 *
 * Reads a labeled EPD set (FEN + game result in a c9 "1-0"/"0-1"/"1/2-1/2"
 * tag), fits the sigmoid scaling constant K, then runs coordinate-descent
 * (local search) on a registry of evaluation constants to minimise the mean
 * squared error between sigmoid(eval) and the game result.
 *
 * Positions are pre-parsed into board snapshots once at load time, so each
 * tuning pass restores the globals and re-runs initAttacksTotal + evaluate
 * without re-parsing FEN. Eval uses BTC's global board, so this is single
 * threaded.
 *
 * Build:  make tuner          (produces texel.exe)
 * Run:    ./texel.exe [epd-file] [maxPositions]
 *           epd-file      default quiet-labeled.v7.epd
 *           maxPositions  0 = all (default), else cap for fast iteration
 *
 * This file has its own main(), so it is linked without src/main.cpp.
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>

#include "../src/defs.h"
#include "../src/init.h"
#include "../src/position.h"
#include "../src/evaluation.h"

// One labeled position: just the raw board state plus the result (white POV).
struct Sample {
    U64 bb[12];
    int side;
    int castle;
    int enpassant;
    double result;   // 1.0 white win, 0.0 black win, 0.5 draw
};

static std::vector<Sample> samples;

// Restore the global board from a snapshot and rebuild the eval attack tables.
static inline void loadSample(const Sample &s) {
    memcpy(bitboards, s.bb, sizeof(bitboards));
    side = s.side;
    castle = s.castle;
    enpassant = s.enpassant;

    occupancies[white] = occupancies[black] = occupancies[both] = 0ULL;
    for (int pc = P; pc <= K; pc++) occupancies[white] |= bitboards[pc];
    for (int pc = p; pc <= k; pc++) occupancies[black] |= bitboards[pc];
    occupancies[both] = occupancies[white] | occupancies[black];

    initAttacksTotal();
    updateMobilityAreas();
}

// Evaluation from White's point of view.
static inline int evalWhite() {
    int e = evaluate();              // side-to-move relative
    return (side == white) ? e : -e;
}

// ---- parameter registry -------------------------------------------------

struct Param {
    std::string name;
    int *ptr;
    int *mirror;   // optional: kept at -value (used for material black entries)
};

static std::vector<Param> params;

static void addParam(const std::string &name, int *ptr, int *mirror = nullptr) {
    params.push_back({name, ptr, mirror});
}

// register a {mg, eg} pair stored as a 2-element array
static void addPair(const std::string &name, int *arr) {
    addParam(name + "_mg", &arr[0]);
    addParam(name + "_eg", &arr[1]);
}

static void setParam(int i, int v) {
    *params[i].ptr = v;
    if (params[i].mirror) *params[i].mirror = -v;
}

// Register the constants we tune. Pawn material is the anchor (left fixed) so the
// overall eval scale - and the positional constants calibrated against it - does
// not drift. The two big tables (mobility, PSTs) are deferred to a later round.
static void registerParams() {
    const char *pt[6]  = {"P", "N", "B", "R", "Q", "K"};
    const char *att[6] = {"x", "p", "n", "b", "r", "q"};  // attacked-type labels

    // --- material (pawn anchored; black entries mirrored to -value) ---
    for (int pc = N; pc <= Q; pc++) {
        addParam(std::string("mat_mg_") + pt[pc], &materialScore[opening][pc], &materialScore[opening][pc + 6]);
        addParam(std::string("mat_eg_") + pt[pc], &materialScore[endgame][pc], &materialScore[endgame][pc + 6]);
    }

    // --- king danger weights ---
    addParam("kdWeakRing", &kdWeakRing);
    addParam("kdUnsafeCheck", &kdUnsafeCheck);
    addParam("kdBlocker", &kdBlocker);
    addParam("kdKingAttacks", &kdKingAttacks);
    addParam("kdFlankAttack", &kdFlankAttack);
    addParam("kdNoQueen", &kdNoQueen);
    addParam("kdKnightDefense", &kdKnightDefense);
    addParam("kdShelter", &kdShelter);
    addParam("kdFlankDefense", &kdFlankDefense);
    addParam("kdInit", &kdInit);
    for (int pc = N; pc <= Q; pc++)
        addParam(std::string("kaw_") + pt[pc], &KingAttackWeights[pc]);
    for (int pc = N; pc <= Q; pc++) {
        addParam(std::string("safechk_") + pt[pc] + "_1", &SafeCheck[pc][0]);
        addParam(std::string("safechk_") + pt[pc] + "_2", &SafeCheck[pc][1]);
    }
    addPair("PawnlessFlank", PawnlessFlank);
    addPair("FlankAttacks", FlankAttacks);

    // --- threats ---
    for (int pc = 1; pc <= 5; pc++) addPair(std::string("ThreatByMinor_") + att[pc], ThreatByMinor[pc]);
    for (int pc = 1; pc <= 5; pc++) addPair(std::string("ThreatByRook_") + att[pc], ThreatByRook[pc]);
    addPair("ThreatByKing", ThreatByKing);
    addPair("Hanging", Hanging);
    addPair("WeakQueenProtection", WeakQueenProtection);
    addPair("RestrictedPiece", RestrictedPiece);
    addPair("ThreatBySafePawn", ThreatBySafePawn);
    addPair("ThreatByPawnPush", ThreatByPawnPush);

    // --- pawn structure ---
    addPair("doublePawn", doublePawnPenalty);
    addPair("isolatedPawn", isolatedPawnPenalty);
    addPair("backwardPawn", backwardPawnPenalty);
    addPair("weakUnopposed", weakUnopposed);
    for (int s = 0; s < 2; s++)
        for (int r = 1; r <= 6; r++)
            addParam("passedRank_" + std::to_string(s) + "_" + std::to_string(r), &passedPawnRankBonus[s][r]);
    for (int r = 1; r <= 6; r++)
        addParam("connected_" + std::to_string(r), &connectedPawnBonus[r]);

    // --- rook on file ---
    addPair("semiOpenFile", semiOpenFileScore);
    addPair("openFile", openFileScore);

    // --- piece bonuses ---
    addPair("bishopPair", BishopPairBonus);
    addPair("outpostKnight", OutpostBonusKnight);
    addPair("outpostBishop", OutpostBonusBishop);
    addPair("minorBehindPawn", MinorBehindPawn);

    // --- pawn shelter / storm ---
    for (int d = 0; d < 4; d++)
        for (int r = 0; r < 7; r++) {
            addParam("shelter_" + std::to_string(d) + "_" + std::to_string(r), &ShelterStrength[d][r]);
            addParam("storm_" + std::to_string(d) + "_" + std::to_string(r), &UnblockedStorm[d][r]);
        }

    addParam("tempo", &tempoBonus);
}

// ---- objective ----------------------------------------------------------

static void dumpConstants(FILE *o);  // defined below; used for per-phase checkpoints

static inline double sigmoid(double e, double K) {
    return 1.0 / (1.0 + pow(10.0, -K * e / 400.0));
}

static double meanSquaredError(double K) {
    double total = 0.0;
    size_t n = samples.size();
    for (size_t i = 0; i < n; i++) {
        loadSample(samples[i]);
        double e = (double) evalWhite();
        double diff = samples[i].result - sigmoid(e, K);
        total += diff * diff;
    }
    return total / (double) n;
}

// Find K that minimises MSE on the current parameters (coarse scan + refine).
static double fitK() {
    double bestK = 1.0, bestErr = 1e9;
    for (double K = 0.2; K <= 3.0; K += 0.1) {
        double err = meanSquaredError(K);
        if (err < bestErr) { bestErr = err; bestK = K; }
    }
    // refine around the best
    double lo = bestK - 0.1, hi = bestK + 0.1;
    for (int it = 0; it < 6; it++) {
        double m1 = lo + (hi - lo) / 3.0, m2 = hi - (hi - lo) / 3.0;
        if (meanSquaredError(m1) < meanSquaredError(m2)) hi = m2; else lo = m1;
    }
    return (lo + hi) / 2.0;
}

// ---- loading ------------------------------------------------------------

static double resultFromLine(const char *line) {
    if (strstr(line, "1/2-1/2")) return 0.5;
    if (strstr(line, "1-0"))     return 1.0;
    if (strstr(line, "0-1"))     return 0.0;
    return -1.0;
}

static void loadEPD(const char *path, long maxPositions) {
    FILE *f = fopen(path, "r");
    if (!f) { printf("ERROR: cannot open %s\n", path); exit(1); }

    char line[512];
    long count = 0;
    while (fgets(line, sizeof(line), f)) {
        double res = resultFromLine(line);
        if (res < 0.0) continue;

        parseFEN(line);   // ignores the trailing c9 tag harmlessly

        Sample s;
        memcpy(s.bb, bitboards, sizeof(s.bb));
        s.side = side;
        s.castle = castle;
        s.enpassant = enpassant;
        s.result = res;
        samples.push_back(s);

        if (++count % 200000 == 0) printf("  loaded %ld positions...\n", count);
        if (maxPositions > 0 && count >= maxPositions) break;
    }
    fclose(f);
    printf("loaded %zu positions from %s\n", samples.size(), path);
}

// ---- coordinate descent -------------------------------------------------

static void tune(double K) {
    double bestErr = meanSquaredError(K);
    printf("start MSE = %.8f (K=%.4f, %zu params)\n", bestErr, K, params.size());

    const double sweepEps = 1e-6;   // stop a step phase when a whole sweep barely helps
    for (int step = 16; step >= 1; step /= 2) {
        while (true) {
            double sweepStart = bestErr;
            for (size_t i = 0; i < params.size(); i++) {
                int best = *params[i].ptr;
                bool moved = false;

                // Try each direction; on an improving direction, keep stepping
                // until it stops helping (converges this param in one visit).
                for (int dir = 1; dir >= -1; dir -= 2) {
                    while (true) {
                        setParam(i, best + dir * step);
                        double e = meanSquaredError(K);
                        if (e < bestErr - 1e-9) {
                            bestErr = e;
                            best += dir * step;
                            moved = true;
                        } else break;
                    }
                    if (moved) break;   // found a good direction; don't try the other
                }
                setParam(i, best);      // restore to best value found
            }
            printf("step %2d : MSE = %.8f\n", step, bestErr);
            fflush(stdout);
            if (sweepStart - bestErr < sweepEps) break;  // diminishing returns; next step
        }
        // checkpoint after each step phase so the run can be stopped early
        FILE *cp = fopen("tuned_constants.txt", "w");
        if (cp) { dumpConstants(cp); fclose(cp); }
    }

    printf("\n=== tuned parameters ===\n");
    for (size_t i = 0; i < params.size(); i++)
        printf("%-22s = %d\n", params[i].name.c_str(), *params[i].ptr);
    printf("final MSE = %.8f\n", bestErr);
}

// Emit the tuned arrays in eval_constants.cpp form for paste-in application.
static void dumpConstants(FILE *o) {
    fprintf(o, "\n=== tuned eval_constants (paste-ready) ===\n");

    fprintf(o, "int materialScore[2][12] = {\n  {");
    for (int i = 0; i < 12; i++) fprintf(o, "%d, ", materialScore[opening][i]);
    fprintf(o, "},\n  {");
    for (int i = 0; i < 12; i++) fprintf(o, "%d, ", materialScore[endgame][i]);
    fprintf(o, "}\n};\n");

    fprintf(o, "int KingAttackWeights[6] = { ");
    for (int i = 0; i < 6; i++) fprintf(o, "%d, ", KingAttackWeights[i]);
    fprintf(o, "};\n");

    fprintf(o, "int SafeCheck[6][2] = { ");
    for (int i = 0; i < 6; i++) fprintf(o, "{%d,%d}, ", SafeCheck[i][0], SafeCheck[i][1]);
    fprintf(o, "};\n");

    fprintf(o, "kdWeakRing=%d kdUnsafeCheck=%d kdBlocker=%d kdKingAttacks=%d kdFlankAttack=%d "
               "kdNoQueen=%d kdKnightDefense=%d kdShelter=%d kdFlankDefense=%d kdInit=%d\n",
            kdWeakRing, kdUnsafeCheck, kdBlocker, kdKingAttacks, kdFlankAttack,
            kdNoQueen, kdKnightDefense, kdShelter, kdFlankDefense, kdInit);

    fprintf(o, "int ThreatByMinor[6][2] = { ");
    for (int i = 0; i < 6; i++) fprintf(o, "{%d,%d}, ", ThreatByMinor[i][0], ThreatByMinor[i][1]);
    fprintf(o, "};\n");
    fprintf(o, "int ThreatByRook[6][2] = { ");
    for (int i = 0; i < 6; i++) fprintf(o, "{%d,%d}, ", ThreatByRook[i][0], ThreatByRook[i][1]);
    fprintf(o, "};\n");

    fprintf(o, "PawnlessFlank{%d,%d} FlankAttacks{%d,%d} ThreatByKing{%d,%d} Hanging{%d,%d} "
               "WeakQueenProtection{%d,%d} RestrictedPiece{%d,%d} ThreatBySafePawn{%d,%d} ThreatByPawnPush{%d,%d}\n",
            PawnlessFlank[0], PawnlessFlank[1], FlankAttacks[0], FlankAttacks[1],
            ThreatByKing[0], ThreatByKing[1], Hanging[0], Hanging[1],
            WeakQueenProtection[0], WeakQueenProtection[1], RestrictedPiece[0], RestrictedPiece[1],
            ThreatBySafePawn[0], ThreatBySafePawn[1], ThreatByPawnPush[0], ThreatByPawnPush[1]);

    fprintf(o, "doublePawn{%d,%d} isolated{%d,%d} backward{%d,%d} weakUnopposed{%d,%d}\n",
            doublePawnPenalty[0], doublePawnPenalty[1], isolatedPawnPenalty[0], isolatedPawnPenalty[1],
            backwardPawnPenalty[0], backwardPawnPenalty[1], weakUnopposed[0], weakUnopposed[1]);

    fprintf(o, "semiOpenFile{%d,%d} openFile{%d,%d} bishopPair{%d,%d} outpostKnight{%d,%d} "
               "outpostBishop{%d,%d} minorBehindPawn{%d,%d}\n",
            semiOpenFileScore[0], semiOpenFileScore[1], openFileScore[0], openFileScore[1],
            BishopPairBonus[0], BishopPairBonus[1], OutpostBonusKnight[0], OutpostBonusKnight[1],
            OutpostBonusBishop[0], OutpostBonusBishop[1], MinorBehindPawn[0], MinorBehindPawn[1]);

    fprintf(o, "int passedPawnRankBonus[2][8] = {\n  {");
    for (int i = 0; i < 8; i++) fprintf(o, "%d, ", passedPawnRankBonus[0][i]);
    fprintf(o, "},\n  {");
    for (int i = 0; i < 8; i++) fprintf(o, "%d, ", passedPawnRankBonus[1][i]);
    fprintf(o, "}\n};\n");

    fprintf(o, "int connectedPawnBonus[8] = { ");
    for (int i = 0; i < 8; i++) fprintf(o, "%d, ", connectedPawnBonus[i]);
    fprintf(o, "};\n");

    fprintf(o, "int ShelterStrength[4][7] = {\n");
    for (int d = 0; d < 4; d++) {
        fprintf(o, "  {");
        for (int r = 0; r < 7; r++) fprintf(o, "%d, ", ShelterStrength[d][r]);
        fprintf(o, "},\n");
    }
    fprintf(o, "};\n");

    fprintf(o, "int UnblockedStorm[4][7] = {\n");
    for (int d = 0; d < 4; d++) {
        fprintf(o, "  {");
        for (int r = 0; r < 7; r++) fprintf(o, "%d, ", UnblockedStorm[d][r]);
        fprintf(o, "},\n");
    }
    fprintf(o, "};\n");

    fprintf(o, "tempoBonus = %d\n", tempoBonus);
}

int main(int argc, char **argv) {
    initAll();
    registerParams();

    const char *epd = (argc >= 2) ? argv[1] : "quiet-labeled.v7.epd";
    long maxPositions = (argc >= 3) ? atol(argv[2]) : 0;

    loadEPD(epd, maxPositions);
    if (samples.empty()) { printf("no positions loaded\n"); return 1; }

    double K = fitK();
    printf("fitted K = %.4f\n", K);

    tune(K);

    dumpConstants(stdout);
    FILE *tf = fopen("tuned_constants.txt", "w");
    if (tf) { dumpConstants(tf); fclose(tf); printf("\nwrote tuned_constants.txt\n"); }
    return 0;
}
