/*
 * One-off generator: converts old-Stockfish PSQT (psqt.cpp Bonus/PBonus) and material
 * (types.h PieceValue) into BetterThanCris array form.
 *
 * SF orientation: square A1=0, rank index 0 = RANK_1, Bonus files A-D explicit and
 * mirrored for E-H. BTC orientation: square a8=0 (rank 8 top), PieceTables[stage][piece]
 * [square] with stage 0=mg,1=eg and piece 0..5 = P,N,B,R,Q,K, white POV.
 *
 * Build/run:  g++ -O2 tuner/genpsqt.cpp -o genpsqt && ./genpsqt
 */
#include <cstdio>

// SF Bonus, positional only (material added separately). Order here: N,B,R,Q,K.
// [piece][rank 0..7 = RANK_1..RANK_8][file 0..3 = A..D]{mg,eg}
static const int Bonus[5][8][4][2] = {
  { // Knight
   {{-175,-96},{-92,-65},{-74,-49},{-73,-21}},
   {{-77,-67},{-41,-54},{-27,-18},{-15,8}},
   {{-61,-40},{-17,-27},{6,-8},{12,29}},
   {{-35,-35},{8,-2},{40,13},{49,28}},
   {{-34,-45},{13,-16},{44,9},{51,39}},
   {{-9,-51},{22,-44},{58,-16},{53,17}},
   {{-67,-69},{-27,-50},{4,-51},{37,12}},
   {{-201,-100},{-83,-88},{-56,-56},{-26,-17}}
  },
  { // Bishop
   {{-37,-40},{-4,-21},{-6,-26},{-16,-8}},
   {{-11,-26},{6,-9},{13,-12},{3,1}},
   {{-5,-11},{15,-1},{-4,-1},{12,7}},
   {{-4,-14},{8,-4},{18,0},{27,12}},
   {{-8,-12},{20,-1},{15,-10},{22,11}},
   {{-11,-21},{4,4},{1,3},{8,4}},
   {{-12,-22},{-10,-14},{4,-1},{0,1}},
   {{-34,-32},{1,-29},{-10,-26},{-16,-17}}
  },
  { // Rook
   {{-31,-9},{-20,-13},{-14,-10},{-5,-9}},
   {{-21,-12},{-13,-9},{-8,-1},{6,-2}},
   {{-25,6},{-11,-8},{-1,-2},{3,-6}},
   {{-13,-6},{-5,1},{-4,-9},{-6,7}},
   {{-27,-5},{-15,8},{-4,7},{3,-6}},
   {{-22,6},{-2,1},{6,-7},{12,10}},
   {{-2,4},{12,5},{16,20},{18,-5}},
   {{-17,18},{-19,0},{-1,19},{9,13}}
  },
  { // Queen
   {{3,-69},{-5,-57},{-5,-47},{4,-26}},
   {{-3,-54},{5,-31},{8,-22},{12,-4}},
   {{-3,-39},{6,-18},{13,-9},{7,3}},
   {{4,-23},{5,-3},{9,13},{8,24}},
   {{0,-29},{14,-6},{12,9},{5,21}},
   {{-4,-38},{10,-18},{6,-11},{8,1}},
   {{-5,-50},{6,-27},{10,-24},{8,-8}},
   {{-2,-74},{-2,-52},{1,-43},{-2,-34}}
  },
  { // King
   {{271,1},{327,45},{271,85},{198,76}},
   {{278,53},{303,100},{234,133},{179,135}},
   {{195,88},{258,130},{169,169},{120,175}},
   {{164,103},{190,156},{138,172},{98,172}},
   {{154,96},{179,166},{105,199},{70,199}},
   {{123,92},{145,172},{81,184},{31,191}},
   {{88,47},{120,121},{65,116},{33,131}},
   {{59,11},{89,59},{45,73},{-1,78}}
  }
};

// SF PBonus (pawns, asymmetric, full 8 files, no mirror). [rank 0..7][file 0..7]{mg,eg}.
// Rank 0 (RANK_1) and rank 7 (RANK_8) are zero (pawns never sit there).
static const int PBonus[8][8][2] = {
  {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}},
  {{2,-8},{4,-6},{11,9},{18,5},{16,16},{21,6},{9,-6},{-3,-18}},
  {{-9,-9},{-15,-7},{11,-10},{15,5},{31,2},{23,3},{6,-8},{-20,-5}},
  {{-3,7},{-20,1},{8,-8},{19,-2},{39,-14},{17,-13},{2,-11},{-5,-6}},
  {{11,12},{-4,6},{-11,2},{2,-6},{11,-5},{0,-4},{-12,14},{5,9}},
  {{3,27},{-11,18},{-6,19},{22,29},{-8,30},{-5,9},{-14,8},{-11,14}},
  {{-7,-1},{6,-14},{-2,13},{-11,22},{4,24},{-14,17},{10,7},{-9,7}},
  {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}}
};

// SF material (PieceValue), P,N,B,R,Q,K. King given BTC's 10000 sentinel.
static const int matMg[6] = {126, 781, 825, 1276, 2538, 10000};
static const int matEg[6] = {208, 854, 915, 1380, 2682, 10000};

static int PT[2][6][64];  // [stage][piece][square] in BTC orientation

int main() {
    // pawns (BTC piece 0)
    for (int R = 0; R < 8; R++)
        for (int F = 0; F < 8; F++) {
            int sq = (7 - R) * 8 + F;
            PT[0][0][sq] = PBonus[R][F][0];
            PT[1][0][sq] = PBonus[R][F][1];
        }
    // knight,bishop,rook,queen,king (BTC pieces 1..5), mirrored files A-D -> E-H
    for (int bi = 0; bi < 5; bi++)
        for (int R = 0; R < 8; R++)
            for (int F = 0; F < 4; F++) {
                int mg = Bonus[bi][R][F][0], eg = Bonus[bi][R][F][1];
                int sqL = (7 - R) * 8 + F;
                int sqR = (7 - R) * 8 + (7 - F);
                PT[0][bi + 1][sqL] = mg; PT[0][bi + 1][sqR] = mg;
                PT[1][bi + 1][sqL] = eg; PT[1][bi + 1][sqR] = eg;
            }

    // emit materialScore
    printf("int materialScore[2][12] = {\n{\n");
    for (int i = 0; i < 6; i++) printf("    %d,\n", matMg[i]);
    for (int i = 0; i < 6; i++) printf("    %d,\n", -matMg[i]);
    printf("},\n{\n");
    for (int i = 0; i < 6; i++) printf("    %d,\n", matEg[i]);
    for (int i = 0; i < 6; i++) printf("    %d,\n", -matEg[i]);
    printf("}\n};\n\n");

    // emit PieceTables
    const char *pn[6] = {"Pawn", "Knight", "Bishop", "Rook", "Queen", "King"};
    const char *sn[2] = {"Mid", "End"};
    printf("extern const int PieceTables[2][6][64] = {\n");
    for (int s = 0; s < 2; s++) {
        printf("    {\n");
        for (int p = 0; p < 6; p++) {
            printf("        { // %s%sTable\n        ", sn[s], pn[p]);
            for (int sq = 0; sq < 64; sq++) {
                printf("%5d,", PT[s][p][sq]);
                if (sq % 8 == 7) printf("\n        ");
            }
            printf("},\n");
        }
        printf("    },\n");
    }
    printf("};\n");
    return 0;
}
