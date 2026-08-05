#include "board.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 10x12 mailbox, so off-board squares are detected without file wrapping.
// north is +10 and east is +1.
static int mailbox64[64];
static int mailbox120[120];

static const int knightOffsets[8] = { 21, 19, 12, 8, -8, -12, -19, -21 };
static const int kingOffsets[8]   = { 10, -10, 1, -1, 11, 9, -9, -11 };
static const int bishopOffsets[4] = { 11, 9, -9, -11 };
static const int rookOffsets[4]   = { 10, -10, 1, -1 };

static unsigned long long pieceKeys[PIECE_NB][64];
static unsigned long long castleKeys[16];
static unsigned long long epKeys[8];
static unsigned long long sideKey;

static const char pieceChars[PIECE_NB] = { '.', 'P', 'N', 'B', 'R', 'Q', 'K',
                                                'p', 'n', 'b', 'r', 'q', 'k' };

static const char *squareNames[64] = {
    "a1","b1","c1","d1","e1","f1","g1","h1",
    "a2","b2","c2","d2","e2","f2","g2","h2",
    "a3","b3","c3","d3","e3","f3","g3","h3",
    "a4","b4","c4","d4","e4","f4","g4","h4",
    "a5","b5","c5","d5","e5","f5","g5","h5",
    "a6","b6","c6","d6","e6","f6","g6","h6",
    "a7","b7","c7","d7","e7","f7","g7","h7",
    "a8","b8","c8","d8","e8","f8","g8","h8"
};

static unsigned long long rngState = 0x9E3779B97F4A7C15ULL;

static unsigned long long nextRandom(void)
{
    rngState ^= rngState << 13;
    rngState ^= rngState >> 7;
    rngState ^= rngState << 17;
    return rngState;
}

void boardInit(void)
{
    for (int i = 0; i < 120; i++)
        mailbox120[i] = -1;

    for (int rank = 0; rank < 8; rank++)
    {
        for (int file = 0; file < 8; file++)
        {
            int sq = rank * 8 + file;
            int m  = (rank + 2) * 10 + (file + 1);
            mailbox64[sq]  = m;
            mailbox120[m]  = sq;
        }
    }

    for (int p = 0; p < PIECE_NB; p++)
        for (int sq = 0; sq < 64; sq++)
            pieceKeys[p][sq] = nextRandom();

    for (int i = 0; i < 16; i++)
        castleKeys[i] = nextRandom();

    for (int i = 0; i < 8; i++)
        epKeys[i] = nextRandom();

    sideKey = nextRandom();
}

static inline int pieceColour(int piece)
{
    return piece >= BP ? BLACK : WHITE;
}

static inline int isWhitePiece(int piece)
{
    return piece >= WP && piece <= WK;
}

static inline int isBlackPiece(int piece)
{
    return piece >= BP;
}

static inline int sameSide(int piece, int side)
{
    if (piece == EMPTY) return 0;
    return pieceColour(piece) == side;
}

// squares from which a pawn of the given side attacks sq
static inline int pawnAttacksSquare(const Position *pos, int sq, int side)
{
    int m = mailbox64[sq];
    int a = side == WHITE ? m - 11 : m + 11;
    int b = side == WHITE ? m - 9  : m + 9;
    int want = side == WHITE ? WP : BP;

    if (mailbox120[a] >= 0 && pos->squares[mailbox120[a]] == want) return 1;
    if (mailbox120[b] >= 0 && pos->squares[mailbox120[b]] == want) return 1;
    return 0;
}

static int isAttacked(const Position *pos, int sq, int bySide)
{
    if (pawnAttacksSquare(pos, sq, bySide))
        return 1;

    int m = mailbox64[sq];

    int knight = bySide == WHITE ? WN : BN;
    for (int i = 0; i < 8; i++)
    {
        int t = mailbox120[m + knightOffsets[i]];
        if (t >= 0 && pos->squares[t] == knight)
            return 1;
    }

    int king = bySide == WHITE ? WK : BK;
    for (int i = 0; i < 8; i++)
    {
        int t = mailbox120[m + kingOffsets[i]];
        if (t >= 0 && pos->squares[t] == king)
            return 1;
    }

    int bishop = bySide == WHITE ? WB : BB;
    int rook   = bySide == WHITE ? WR : BR;
    int queen  = bySide == WHITE ? WQ : BQ;

    for (int i = 0; i < 4; i++)
    {
        int step = bishopOffsets[i];
        for (int cur = m + step; mailbox120[cur] >= 0; cur += step)
        {
            int piece = pos->squares[mailbox120[cur]];
            if (piece == EMPTY) continue;
            if (piece == bishop || piece == queen) return 1;
            break;
        }
    }

    for (int i = 0; i < 4; i++)
    {
        int step = rookOffsets[i];
        for (int cur = m + step; mailbox120[cur] >= 0; cur += step)
        {
            int piece = pos->squares[mailbox120[cur]];
            if (piece == EMPTY) continue;
            if (piece == rook || piece == queen) return 1;
            break;
        }
    }

    return 0;
}

static int findKing(const Position *pos, int side)
{
    int king = side == WHITE ? WK : BK;
    for (int sq = 0; sq < 64; sq++)
        if (pos->squares[sq] == king)
            return sq;
    return -1;
}

int boardInCheck(const Position *pos, int side)
{
    int king = findKing(pos, side);
    if (king < 0)
        return 0;
    return isAttacked(pos, king, side ^ 1);
}

// an en passant square only changes the position for repetition purposes when
// a capture onto it is actually available
static int epIsRelevant(const Position *pos)
{
    if (pos->ep < 0)
        return 0;

    int m = mailbox64[pos->ep];
    int attacker = pos->side == WHITE ? WP : BP;
    int a = pos->side == WHITE ? m - 11 : m + 11;
    int b = pos->side == WHITE ? m - 9  : m + 9;

    if (mailbox120[a] >= 0 && pos->squares[mailbox120[a]] == attacker) return 1;
    if (mailbox120[b] >= 0 && pos->squares[mailbox120[b]] == attacker) return 1;
    return 0;
}

// recomputed from scratch after every move: an arbiter has no reason to risk
// an incremental update bug to save a few hundred nanoseconds
static void computeKey(Position *pos)
{
    unsigned long long key = 0;

    for (int sq = 0; sq < 64; sq++)
        if (pos->squares[sq] != EMPTY)
            key ^= pieceKeys[pos->squares[sq]][sq];

    key ^= castleKeys[pos->castling & 15];

    if (epIsRelevant(pos))
        key ^= epKeys[pos->ep & 7];

    if (pos->side == BLACK)
        key ^= sideKey;

    pos->key = key;
}

int boardParseFen(Position *pos, const char *fen)
{
    memset(pos, 0, sizeof(Position));
    pos->ep = -1;

    const char *c = fen;
    while (*c == ' ') c++;

    int rank = 7;
    int file = 0;

    while (*c && *c != ' ')
    {
        char ch = *c++;

        if (ch == '/')
        {
            if (file != 8) return 0;
            rank--;
            file = 0;
            if (rank < 0) return 0;
            continue;
        }

        if (ch >= '1' && ch <= '8')
        {
            file += ch - '0';
            if (file > 8) return 0;
            continue;
        }

        int piece = EMPTY;
        for (int p = WP; p < PIECE_NB; p++)
            if (pieceChars[p] == ch)
                piece = p;

        if (piece == EMPTY || file > 7 || rank < 0)
            return 0;

        pos->squares[rank * 8 + file] = (unsigned char)piece;
        file++;
    }

    if (rank != 0 || file != 8)
        return 0;

    while (*c == ' ') c++;
    pos->side = (*c == 'b') ? BLACK : WHITE;
    while (*c && *c != ' ') c++;
    while (*c == ' ') c++;

    while (*c && *c != ' ')
    {
        if (*c == 'K') pos->castling |= CR_WK;
        if (*c == 'Q') pos->castling |= CR_WQ;
        if (*c == 'k') pos->castling |= CR_BK;
        if (*c == 'q') pos->castling |= CR_BQ;
        c++;
    }

    while (*c == ' ') c++;
    if (*c && *c != '-')
    {
        int f = c[0] - 'a';
        int r = c[1] - '1';
        if (f >= 0 && f < 8 && r >= 0 && r < 8)
            pos->ep = (signed char)(r * 8 + f);
        while (*c && *c != ' ') c++;
    }
    else if (*c == '-')
    {
        c++;
    }

    while (*c == ' ') c++;
    pos->halfmove = 0;
    if (*c >= '0' && *c <= '9')
    {
        pos->halfmove = atoi(c);
        while (*c && *c != ' ') c++;
    }

    while (*c == ' ') c++;
    pos->fullmove = 1;
    if (*c >= '0' && *c <= '9')
        pos->fullmove = atoi(c);
    if (pos->fullmove < 1)
        pos->fullmove = 1;

    computeKey(pos);
    return 1;
}

void boardWriteFen(const Position *pos, char *out, int outSize)
{
    char buf[128];
    int n = 0;

    for (int rank = 7; rank >= 0; rank--)
    {
        int empty = 0;
        for (int file = 0; file < 8; file++)
        {
            int piece = pos->squares[rank * 8 + file];
            if (piece == EMPTY)
            {
                empty++;
                continue;
            }
            if (empty)
            {
                buf[n++] = (char)('0' + empty);
                empty = 0;
            }
            buf[n++] = pieceChars[piece];
        }
        if (empty)
            buf[n++] = (char)('0' + empty);
        if (rank)
            buf[n++] = '/';
    }
    buf[n] = '\0';

    char rights[8];
    int r = 0;
    if (pos->castling & CR_WK) rights[r++] = 'K';
    if (pos->castling & CR_WQ) rights[r++] = 'Q';
    if (pos->castling & CR_BK) rights[r++] = 'k';
    if (pos->castling & CR_BQ) rights[r++] = 'q';
    if (!r) rights[r++] = '-';
    rights[r] = '\0';

    snprintf(out, outSize, "%s %c %s %s %d %d",
             buf,
             pos->side == WHITE ? 'w' : 'b',
             rights,
             pos->ep >= 0 ? squareNames[pos->ep] : "-",
             pos->halfmove,
             pos->fullmove);
}

static inline void addMove(Move *list, int *count, int from, int to, int promo, int flags)
{
    Move *m = &list[*count];
    m->from  = (unsigned char)from;
    m->to    = (unsigned char)to;
    m->promo = (unsigned char)promo;
    m->flags = (unsigned char)flags;
    (*count)++;
}

static void addPawnMove(Move *list, int *count, int from, int to, int flags, int side)
{
    int promoRank = side == WHITE ? 7 : 0;

    if (to / 8 == promoRank)
    {
        int base = side == WHITE ? WN : BN;
        // knight, bishop, rook, queen in the piece-code order N B R Q
        for (int p = 0; p < 4; p++)
            addMove(list, count, from, to, base + p, flags);
        return;
    }

    addMove(list, count, from, to, 0, flags);
}

static int generatePseudo(const Position *pos, Move *list)
{
    int count = 0;
    int side = pos->side;

    for (int from = 0; from < 64; from++)
    {
        int piece = pos->squares[from];
        if (piece == EMPTY || pieceColour(piece) != side)
            continue;

        int m = mailbox64[from];

        if (piece == WP || piece == BP)
        {
            int forward = side == WHITE ? 10 : -10;
            int startRank = side == WHITE ? 1 : 6;

            int one = mailbox120[m + forward];
            if (one >= 0 && pos->squares[one] == EMPTY)
            {
                addPawnMove(list, &count, from, one, 0, side);

                if (from / 8 == startRank)
                {
                    int two = mailbox120[m + 2 * forward];
                    if (two >= 0 && pos->squares[two] == EMPTY)
                        addMove(list, &count, from, two, 0, MF_DOUBLE);
                }
            }

            int caps[2] = { m + forward - 1, m + forward + 1 };
            for (int i = 0; i < 2; i++)
            {
                int to = mailbox120[caps[i]];
                if (to < 0)
                    continue;

                int target = pos->squares[to];
                if (target != EMPTY && pieceColour(target) != side)
                    addPawnMove(list, &count, from, to, MF_CAPTURE, side);
                else if (target == EMPTY && to == pos->ep)
                    addMove(list, &count, from, to, 0, MF_CAPTURE | MF_EP);
            }

            continue;
        }

        if (piece == WN || piece == BN || piece == WK || piece == BK)
        {
            const int *offsets = (piece == WN || piece == BN) ? knightOffsets : kingOffsets;

            for (int i = 0; i < 8; i++)
            {
                int to = mailbox120[m + offsets[i]];
                if (to < 0)
                    continue;

                int target = pos->squares[to];
                if (sameSide(target, side))
                    continue;

                addMove(list, &count, from, to, 0, target == EMPTY ? 0 : MF_CAPTURE);
            }

            continue;
        }

        const int *offsets;
        int directions;

        if (piece == WB || piece == BB)      { offsets = bishopOffsets; directions = 4; }
        else if (piece == WR || piece == BR) { offsets = rookOffsets;   directions = 4; }
        else                                 { offsets = kingOffsets;   directions = 8; }

        for (int i = 0; i < directions; i++)
        {
            int step = offsets[i];
            for (int cur = m + step; mailbox120[cur] >= 0; cur += step)
            {
                int to = mailbox120[cur];
                int target = pos->squares[to];

                if (sameSide(target, side))
                    break;

                addMove(list, &count, from, to, 0, target == EMPTY ? 0 : MF_CAPTURE);

                if (target != EMPTY)
                    break;
            }
        }
    }

    // castling. the king must not start in check, pass through an attacked
    // square, or land on one, and the squares between must be empty.
    int enemy = side ^ 1;

    if (side == WHITE)
    {
        if ((pos->castling & CR_WK) &&
            pos->squares[5] == EMPTY && pos->squares[6] == EMPTY &&
            pos->squares[4] == WK && pos->squares[7] == WR &&
            !isAttacked(pos, 4, enemy) && !isAttacked(pos, 5, enemy) && !isAttacked(pos, 6, enemy))
            addMove(list, &count, 4, 6, 0, MF_CASTLE);

        if ((pos->castling & CR_WQ) &&
            pos->squares[3] == EMPTY && pos->squares[2] == EMPTY && pos->squares[1] == EMPTY &&
            pos->squares[4] == WK && pos->squares[0] == WR &&
            !isAttacked(pos, 4, enemy) && !isAttacked(pos, 3, enemy) && !isAttacked(pos, 2, enemy))
            addMove(list, &count, 4, 2, 0, MF_CASTLE);
    }
    else
    {
        if ((pos->castling & CR_BK) &&
            pos->squares[61] == EMPTY && pos->squares[62] == EMPTY &&
            pos->squares[60] == BK && pos->squares[63] == BR &&
            !isAttacked(pos, 60, enemy) && !isAttacked(pos, 61, enemy) && !isAttacked(pos, 62, enemy))
            addMove(list, &count, 60, 62, 0, MF_CASTLE);

        if ((pos->castling & CR_BQ) &&
            pos->squares[59] == EMPTY && pos->squares[58] == EMPTY && pos->squares[57] == EMPTY &&
            pos->squares[60] == BK && pos->squares[56] == BR &&
            !isAttacked(pos, 60, enemy) && !isAttacked(pos, 59, enemy) && !isAttacked(pos, 58, enemy))
            addMove(list, &count, 60, 58, 0, MF_CASTLE);
    }

    return count;
}

void boardMakeMove(Position *pos, Move m)
{
    int piece = pos->squares[m.from];
    int side  = pos->side;
    int captured = pos->squares[m.to];

    pos->squares[m.to]   = (unsigned char)piece;
    pos->squares[m.from] = EMPTY;

    if (m.flags & MF_EP)
    {
        int victim = side == WHITE ? m.to - 8 : m.to + 8;
        pos->squares[victim] = EMPTY;
    }

    if (m.promo)
        pos->squares[m.to] = m.promo;

    if (m.flags & MF_CASTLE)
    {
        if (m.to == 6)       { pos->squares[5]  = WR; pos->squares[7]  = EMPTY; }
        else if (m.to == 2)  { pos->squares[3]  = WR; pos->squares[0]  = EMPTY; }
        else if (m.to == 62) { pos->squares[61] = BR; pos->squares[63] = EMPTY; }
        else if (m.to == 58) { pos->squares[59] = BR; pos->squares[56] = EMPTY; }
    }

    // castling rights: moving a king or rook, or capturing a rook on its
    // original square, removes the matching right
    if (piece == WK) pos->castling &= ~(CR_WK | CR_WQ);
    if (piece == BK) pos->castling &= ~(CR_BK | CR_BQ);

    if (m.from == 0 || m.to == 0)   pos->castling &= ~CR_WQ;
    if (m.from == 7 || m.to == 7)   pos->castling &= ~CR_WK;
    if (m.from == 56 || m.to == 56) pos->castling &= ~CR_BQ;
    if (m.from == 63 || m.to == 63) pos->castling &= ~CR_BK;

    pos->ep = -1;
    if (m.flags & MF_DOUBLE)
        pos->ep = (signed char)(side == WHITE ? m.from + 8 : m.from - 8);

    if (piece == WP || piece == BP || captured != EMPTY || (m.flags & MF_EP))
        pos->halfmove = 0;
    else
        pos->halfmove++;

    if (side == BLACK)
        pos->fullmove++;

    pos->side = (unsigned char)(side ^ 1);

    computeKey(pos);
}

int boardGenerateLegal(const Position *pos, Move *list)
{
    Move pseudo[MAX_MOVES];
    int pseudoCount = generatePseudo(pos, pseudo);
    int count = 0;

    for (int i = 0; i < pseudoCount; i++)
    {
        Position copy = *pos;
        boardMakeMove(&copy, pseudo[i]);

        if (!boardInCheck(&copy, pos->side))
            list[count++] = pseudo[i];
    }

    return count;
}

void boardMoveToUci(Move m, char *out)
{
    out[0] = (char)('a' + m.from % 8);
    out[1] = (char)('1' + m.from / 8);
    out[2] = (char)('a' + m.to % 8);
    out[3] = (char)('1' + m.to / 8);

    if (m.promo)
    {
        int type = m.promo > WK ? m.promo - BP : m.promo - WP;
        const char *promoChars = "pnbrqk";
        out[4] = promoChars[type];
        out[5] = '\0';
    }
    else
    {
        out[4] = '\0';
    }
}

int boardMoveFromUci(const Position *pos, const char *text, Move *out)
{
    if (!text || strlen(text) < 4)
        return 0;

    int from = (text[0] - 'a') + (text[1] - '1') * 8;
    int to   = (text[2] - 'a') + (text[3] - '1') * 8;

    if (from < 0 || from > 63 || to < 0 || to > 63)
        return 0;

    char promo = 0;
    if (text[4] && text[4] != ' ' && text[4] != '\n')
        promo = (char)tolower((unsigned char)text[4]);

    Move list[MAX_MOVES];
    int count = boardGenerateLegal(pos, list);

    for (int i = 0; i < count; i++)
    {
        if (list[i].from != from || list[i].to != to)
            continue;

        if (list[i].promo)
        {
            int type = list[i].promo > WK ? list[i].promo - BP : list[i].promo - WP;
            const char *promoChars = "pnbrqk";
            if (promo != promoChars[type])
                continue;
        }
        else if (promo)
        {
            continue;
        }

        *out = list[i];
        return 1;
    }

    return 0;
}

static int pieceTypeOf(int piece)
{
    return piece > WK ? piece - BP + 1 : piece;
}

void boardMoveToSan(const Position *pos, Move m, char *out, int outSize)
{
    char buf[16];
    int n = 0;

    int piece = pos->squares[m.from];
    int type  = pieceTypeOf(piece);

    if (m.flags & MF_CASTLE)
    {
        if (m.to == 6 || m.to == 62)
            n += snprintf(buf + n, sizeof(buf) - n, "O-O");
        else
            n += snprintf(buf + n, sizeof(buf) - n, "O-O-O");
    }
    else if (type == WP)
    {
        if (m.flags & MF_CAPTURE)
            n += snprintf(buf + n, sizeof(buf) - n, "%cx", 'a' + m.from % 8);

        n += snprintf(buf + n, sizeof(buf) - n, "%s", squareNames[m.to]);

        if (m.promo)
        {
            const char *names = " PNBRQK";
            n += snprintf(buf + n, sizeof(buf) - n, "=%c", names[pieceTypeOf(m.promo)]);
        }
    }
    else
    {
        const char *names = " PNBRQK";
        buf[n++] = names[type];

        // disambiguate against other legal moves of the same piece type that
        // land on the same square
        Move list[MAX_MOVES];
        int count = boardGenerateLegal(pos, list);

        int ambiguous = 0, sameFile = 0, sameRank = 0;

        for (int i = 0; i < count; i++)
        {
            if (list[i].to != m.to || list[i].from == m.from)
                continue;
            if (pieceTypeOf(pos->squares[list[i].from]) != type)
                continue;

            ambiguous = 1;
            if (list[i].from % 8 == m.from % 8) sameFile = 1;
            if (list[i].from / 8 == m.from / 8) sameRank = 1;
        }

        if (ambiguous)
        {
            if (!sameFile)
                buf[n++] = (char)('a' + m.from % 8);
            else if (!sameRank)
                buf[n++] = (char)('1' + m.from / 8);
            else
            {
                buf[n++] = (char)('a' + m.from % 8);
                buf[n++] = (char)('1' + m.from / 8);
            }
        }

        if (m.flags & MF_CAPTURE)
            buf[n++] = 'x';

        buf[n] = '\0';
        n += snprintf(buf + n, sizeof(buf) - n, "%s", squareNames[m.to]);
    }

    Position after = *pos;
    boardMakeMove(&after, m);

    if (boardInCheck(&after, after.side))
    {
        Move replies[MAX_MOVES];
        int replyCount = boardGenerateLegal(&after, replies);
        buf[n++] = replyCount ? '+' : '#';
    }

    buf[n] = '\0';
    snprintf(out, outSize, "%s", buf);
}

int boardMoveFromSan(const Position *pos, const char *text, Move *out)
{
    char san[32];
    int n = 0;

    // copy the token, dropping decorations the matcher does not need
    for (const char *c = text; *c && n < (int)sizeof(san) - 1; c++)
    {
        if (*c == '+' || *c == '#' || *c == '!' || *c == '?')
            continue;
        san[n++] = *c;
    }
    san[n] = '\0';

    if (!n)
        return 0;

    Move list[MAX_MOVES];
    int count = boardGenerateLegal(pos, list);

    if (!strcmp(san, "O-O") || !strcmp(san, "0-0"))
    {
        for (int i = 0; i < count; i++)
            if ((list[i].flags & MF_CASTLE) && (list[i].to == 6 || list[i].to == 62))
            {
                *out = list[i];
                return 1;
            }
        return 0;
    }

    if (!strcmp(san, "O-O-O") || !strcmp(san, "0-0-0"))
    {
        for (int i = 0; i < count; i++)
            if ((list[i].flags & MF_CASTLE) && (list[i].to == 2 || list[i].to == 58))
            {
                *out = list[i];
                return 1;
            }
        return 0;
    }

    int type = WP;
    int index = 0;

    if (strchr("NBRQK", san[0]))
    {
        const char *names = " PNBRQK";
        type = (int)(strchr(names, san[0]) - names);
        index = 1;
    }

    int promoType = 0;
    char *eq = strchr(san, '=');
    if (eq)
    {
        const char *names = " PNBRQK";
        const char *found = strchr(names, eq[1]);
        if (!found)
            return 0;
        promoType = (int)(found - names);
        *eq = '\0';
    }

    // the destination is the last two characters of what remains
    int len = (int)strlen(san);
    if (len < 2)
        return 0;

    int toFile = san[len - 2] - 'a';
    int toRank = san[len - 1] - '1';
    if (toFile < 0 || toFile > 7 || toRank < 0 || toRank > 7)
        return 0;
    int to = toRank * 8 + toFile;

    int fromFile = -1, fromRank = -1;
    for (int i = index; i < len - 2; i++)
    {
        if (san[i] >= 'a' && san[i] <= 'h') fromFile = san[i] - 'a';
        if (san[i] >= '1' && san[i] <= '8') fromRank = san[i] - '1';
    }

    for (int i = 0; i < count; i++)
    {
        Move m = list[i];

        if (m.to != to)
            continue;
        if (pieceTypeOf(pos->squares[m.from]) != type)
            continue;
        if (fromFile >= 0 && m.from % 8 != fromFile)
            continue;
        if (fromRank >= 0 && m.from / 8 != fromRank)
            continue;
        if (promoType && (!m.promo || pieceTypeOf(m.promo) != promoType))
            continue;
        if (!promoType && m.promo)
            continue;

        *out = m;
        return 1;
    }

    return 0;
}

int boardInsufficientMaterial(const Position *pos)
{
    int minors[2] = { 0, 0 };

    for (int sq = 0; sq < 64; sq++)
    {
        int piece = pos->squares[sq];
        if (piece == EMPTY)
            continue;

        int type = pieceTypeOf(piece);

        if (type == WP || type == WR || type == WQ)
            return 0;

        if (type == WN || type == WB)
            minors[pieceColour(piece)]++;
    }

    // king against king, and king plus a single minor on either side, cannot
    // be forced. anything richer is left to the players.
    return minors[WHITE] <= 1 && minors[BLACK] <= 1;
}

unsigned long long boardPerft(const Position *pos, int depth)
{
    if (depth == 0)
        return 1;

    Move list[MAX_MOVES];
    int count = boardGenerateLegal(pos, list);

    if (depth == 1)
        return (unsigned long long)count;

    unsigned long long total = 0;

    for (int i = 0; i < count; i++)
    {
        Position copy = *pos;
        boardMakeMove(&copy, list[i]);
        total += boardPerft(&copy, depth - 1);
    }

    return total;
}
