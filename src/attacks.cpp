#include "attacks.h"

// not A file global variable
extern const U64 notAFile = 18374403900871474942ULL;

// not H file constant
extern const U64 notHFile = 9187201950435737471ULL;

// not AB file constant
extern const U64 notABFile = 18229723555195321596ULL;

// not HG file constant
extern const U64 notGHFile = 4557430888798830399ULL;

// bishops occupancy bit count lookup table
extern const int bishopBits[64] =
{
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6,
};

// rooks occupancy bit count lookup table
extern const int rookBits[64] =
{
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12,
};

// rook magic numbers
U64 rookMagicNumbers[64] =
{
0x8a80104000800020ULL,
0x140002000100040ULL,
0x2801880a0017001ULL,
0x100081001000420ULL,
0x200020010080420ULL,
0x3001c0002010008ULL,
0x8480008002000100ULL,
0x2080088004402900ULL,
0x800098204000ULL,
0x2024401000200040ULL,
0x100802000801000ULL,
0x120800800801000ULL,
0x208808088000400ULL,
0x2802200800400ULL,
0x2200800100020080ULL,
0x801000060821100ULL,
0x80044006422000ULL,
0x100808020004000ULL,
0x12108a0010204200ULL,
0x140848010000802ULL,
0x481828014002800ULL,
0x8094004002004100ULL,
0x4010040010010802ULL,
0x20008806104ULL,
0x100400080208000ULL,
0x2040002120081000ULL,
0x21200680100081ULL,
0x20100080080080ULL,
0x2000a00200410ULL,
0x20080800400ULL,
0x80088400100102ULL,
0x80004600042881ULL,
0x4040008040800020ULL,
0x440003000200801ULL,
0x4200011004500ULL,
0x188020010100100ULL,
0x14800401802800ULL,
0x2080040080800200ULL,
0x124080204001001ULL,
0x200046502000484ULL,
0x480400080088020ULL,
0x1000422010034000ULL,
0x30200100110040ULL,
0x100021010009ULL,
0x2002080100110004ULL,
0x202008004008002ULL,
0x20020004010100ULL,
0x2048440040820001ULL,
0x101002200408200ULL,
0x40802000401080ULL,
0x4008142004410100ULL,
0x2060820c0120200ULL,
0x1001004080100ULL,
0x20c020080040080ULL,
0x2935610830022400ULL,
0x44440041009200ULL,
0x280001040802101ULL,
0x2100190040002085ULL,
0x80c0084100102001ULL,
0x4024081001000421ULL,
0x20030a0244872ULL,
0x12001008414402ULL,
0x2006104900a0804ULL,
0x1004081002402ULL
};

// bishop magic numbers
U64 bishopMagicNumbers[64] =
{
0x40040844404084ULL,
0x2004208a004208ULL,
0x10190041080202ULL,
0x108060845042010ULL,
0x581104180800210ULL,
0x2112080446200010ULL,
0x1080820820060210ULL,
0x3c0808410220200ULL,
0x4050404440404ULL,
0x21001420088ULL,
0x24d0080801082102ULL,
0x1020a0a020400ULL,
0x40308200402ULL,
0x4011002100800ULL,
0x401484104104005ULL,
0x801010402020200ULL,
0x400210c3880100ULL,
0x404022024108200ULL,
0x810018200204102ULL,
0x4002801a02003ULL,
0x85040820080400ULL,
0x810102c808880400ULL,
0xe900410884800ULL,
0x8002020480840102ULL,
0x220200865090201ULL,
0x2010100a02021202ULL,
0x152048408022401ULL,
0x20080002081110ULL,
0x4001001021004000ULL,
0x800040400a011002ULL,
0xe4004081011002ULL,
0x1c004001012080ULL,
0x8004200962a00220ULL,
0x8422100208500202ULL,
0x2000402200300c08ULL,
0x8646020080080080ULL,
0x80020a0200100808ULL,
0x2010004880111000ULL,
0x623000a080011400ULL,
0x42008c0340209202ULL,
0x209188240001000ULL,
0x400408a884001800ULL,
0x110400a6080400ULL,
0x1840060a44020800ULL,
0x90080104000041ULL,
0x201011000808101ULL,
0x1a2208080504f080ULL,
0x8012020600211212ULL,
0x500861011240000ULL,
0x180806108200800ULL,
0x4000020e01040044ULL,
0x300000261044000aULL,
0x802241102020002ULL,
0x20906061210001ULL,
0x5a84841004010310ULL,
0x4010801011c04ULL,
0xa010109502200ULL,
0x4a02012000ULL,
0x500201010098b028ULL,
0x8040002811040900ULL,
0x28000010020204ULL,
0x6000020202d0240ULL,
0x8918844842082200ULL,
0x4010011029020020ULL
};

// bishop attack masks array
U64 bishopMasks[64];

// rook attack masks array
U64 rookMasks[64];

// bishop attacks table ([square][occupancies/block]) 512 possible occupancies for bishops
U64 bishopAttacks[64][512];

// rook attacks table ([square][occupancies/block])
U64 rookAttacks[64][4096];

// create pawn table (two dimensional array of [side to move][square])
U64 pawnAttacks[2][64];

// create knight attack table (one dimensional array of [square])
U64 knightAttacks[64];

// create king attack table (one dimensional array of [square])
U64 kingAttacks[64];

// get pawn attacks (mask)
U64 maskPawnAttacks(int side, int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create the piece bitboard
    U64 pieceBitboard = 0ULL;

    // set the pieces on the board
    setBit(pieceBitboard, square);

    // for white
    if (side == 0)
    {
        // if pawn not in H file, then valid attack to the north east
        if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard >> 7;
        // if pawn not in A file, then valid attack to the north west
        if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard >> 9;
    }
    else // for black
    {
        // if pawn not in A file, then valid attack to the south east
        if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard << 7;
        // if pawn not in H file, then valid attack to the south west
        if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard << 9;
    }
    // return the attacks
    return attacksBitboard;
}

// get knight attacks (mask)
U64 maskKnightAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create the piece bitboard
    U64 pieceBitboard = 0ULL;

    // set the pieces on the board
    setBit(pieceBitboard, square);

    // if knight not on A file, North North West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= (pieceBitboard >> 17);
    // if knight not on A or B file, North West West is valid
    if (pieceBitboard & notABFile) attacksBitboard |= (pieceBitboard >> 10);
    // if knight not on H file, North North East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= (pieceBitboard >> 15);
    // if knight not on G or H file, North East East is valid
    if (pieceBitboard & notGHFile) attacksBitboard |= (pieceBitboard >> 6);

    // if knight not on A file, South South West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= (pieceBitboard << 15);
    // if knight not on A or B file, South West West is valid
    if (pieceBitboard & notABFile) attacksBitboard |= (pieceBitboard << 6);
    // if knight not on H file, South South East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= (pieceBitboard << 17);
    // if knight not on H file, South East East is valid
    if (pieceBitboard & notGHFile) attacksBitboard |= (pieceBitboard << 10);

    // return the attacks
    return attacksBitboard;
}

// get king attacks (mask)
U64 maskKingAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create the piece bitboard
    U64 pieceBitboard = 0ULL;

    // set the pieces on the board
    setBit(pieceBitboard, square);

    // if result of pieceBitboard >> 8 is non-zero, movement North is valid (check is acutally not needed)
    if (pieceBitboard >> 8) attacksBitboard |= (pieceBitboard >> 8);
    // if king not on H file, attack to the North East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard >> 7;
    // if king not on A file, attack to the North West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard >> 9;
    // if king not on A file, attack to the West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard >> 1;
    // if king not on H file, attack to the East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard << 1;
    // if king not on H file, attack to the South East is valid
    if (pieceBitboard & notHFile) attacksBitboard |= pieceBitboard << 9;
    // if king not on A file, attack to the South West is valid
    if (pieceBitboard & notAFile) attacksBitboard |= pieceBitboard << 7;
    // if result of pieceBitboard >> 8 is non-zero, movement South is valid (check is acutally not needed)
    if (pieceBitboard << 8) attacksBitboard |= (pieceBitboard << 8);

    // return the attacks
    return attacksBitboard;

}

// get bishop attacks (mask)
U64 maskBishopAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    /*
    mask relevant bishop occupancy bits (for magic bitboard)
    e.g. for e4:
     8    0  0  0  0  0  0  0  0
     7    0  0  0  0  0  0  1  0
     6    0  1  0  0  0  1  0  0
     5    0  0  1  0  1  0  0  0
     4    0  0  0  0  0  0  0  0
     3    0  0  1  0  1  0  0  0
     2    0  1  0  0  0  1  0  0
     1    0  0  0  0  0  0  0  0

          a  b  c  d  e  f  g  h
    */

    // mask South East rays (r+, f+)
    for (ranks = targetRank + 1, files = targetFile + 1; ranks <= 6 && files <= 6; ranks++, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }
    // mask North East rays (r-, f+)
    for (ranks = targetRank - 1, files = targetFile + 1; ranks >= 1 && files <= 6; ranks--, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }
    // mask South West rays (r+, f-)
    for (ranks = targetRank + 1, files = targetFile - 1; ranks <= 6 && files >=1; ranks++, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }
    // mask North West  rays (r-, f-)
    for (ranks = targetRank - 1, files = targetFile - 1; ranks >= 1 && files >=1; ranks--, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
    }

    // return the attacks
    return attacksBitboard;
}

// get rook attacks (mask)
U64 maskRookAttacks(int square)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    /*
    mask relevant rook occupancy bits (for magic bitboard)
    e.g. for e4:
     8    0  0  0  0  0  0  0  0
     7    0  0  0  0  1  0  0  0
     6    0  0  0  0  1  0  0  0
     5    0  0  0  0  1  0  0  0
     4    0  1  1  1  0  1  1  0
     3    0  0  0  0  1  0  0  0
     2    0  0  0  0  1  0  0  0
     1    0  0  0  0  0  0  0  0

          a  b  c  d  e  f  g  h
    */

    // mask South rays
    for (ranks = targetRank + 1; ranks <= 6; ranks++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
    }
    // mask North rays
    for (ranks = targetRank - 1; ranks >= 1; ranks--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
    }
    // mask West rays
    for (files = targetFile - 1; files >= 1; files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
    }
    // mask East rays
    for (files = targetFile + 1; files <= 6; files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
    }

    // return the attacks
    return attacksBitboard;
}

// get bishop attacks on the fly (in the case of a piece 'blocking')
U64 bishopAttacksOTF(int square, U64 block)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    // generate ALL bishop attacks
    // mask South East rays (r+, f+)
    for (ranks = targetRank + 1, files = targetFile + 1; ranks <= 7 && files <= 7; ranks++, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;
    }
    // mask North East rays (r-, f+)
    for (ranks = targetRank - 1, files = targetFile + 1; ranks >= 0 && files <= 7; ranks--, files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;
    }
    // mask South West rays (r+, f-)
    for (ranks = targetRank + 1, files = targetFile - 1; ranks <= 7 && files >= 0; ranks++, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;

    }
    // mask North West  rays (r-, f-)
    for (ranks = targetRank - 1, files = targetFile - 1; ranks >= 0 && files >= 0; ranks--, files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + files)) & block) break;

    }

    // return the attacks
    return attacksBitboard;
}

// get bishop attacks on the fly (in the case of a piece 'blocking')
U64 rookAttacksOTF(int square, U64 block)
{
    // create attack bitboard
    U64 attacksBitboard = 0ULL;

    // create ranks and files
    int ranks, files;

    // create target rank and files
    int targetRank = square / 8; // gets the rank of the given square
    int targetFile = square % 8; // gets the file of the given square

    // mask South rays
    for (ranks = targetRank + 1; ranks <= 7; ranks++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + targetFile)) & block) break;

    }
    // mask North rays
    for (ranks = targetRank - 1; ranks >= 0; ranks--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (ranks * 8 + targetFile));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (ranks * 8 + targetFile)) & block) break;
    }
    // mask West rays
    for (files = targetFile - 1; files >= 0; files--)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (targetRank * 8 + files)) & block) break;
    }
    // mask East rays
    for (files = targetFile + 1; files <= 7; files++)
    {
        // update attacksBitboard with a bit on the corresponding square
        attacksBitboard |= (1ULL << (targetRank * 8 + files));
        // break if attack square has a piece (from the block bitboard)
        if ((1ULL << (targetRank * 8 + files)) & block) break;
    }

    // return the attacks
    return attacksBitboard;
}

// initialise leapers attacks (pawns, knights, kings)
void initLeapersAttacks()
{
    // for every square get leapers attacks and store in array (or two-dimensional array)
    for (int square = 0; square < 64; square ++)
    {
        // initialise white pawn attacks
        pawnAttacks[white][square] = maskPawnAttacks(white, square);
        // initialise black pawn attacks
        pawnAttacks[black][square] = maskPawnAttacks(black, square);

        // initialise knight attacks
        knightAttacks[square] = maskKnightAttacks(square);

        // initialise king attacks
        kingAttacks[square] = maskKingAttacks(square);
    }
}

// function that sets occupancies
U64 setOccupancy(int index, int bitsInMask, U64 attackMask)
{
    // initialise occupancy map
    U64 occupancyMap = 0ULL;

    // loop over bits in attack mask
    for (int count = 0; count < bitsInMask; count++)
    {
        // get LSFB of mask and remove (pop) it
        int square = getLSFBIndex(attackMask);
        popBit(attackMask, square);

        // if the 'count'-th bit in 'index' is set to 1,
        // set the corresponding bit in occupancyMap at position square
        if (index & (1 << count))
        {
            occupancyMap |= (1ULL << square);
        }
    }

    // return map
    return occupancyMap;
}

// initialise slider piece attack tables (bishop flag)
void initSlidersAttacks(int bishop)
{
    // loop through squares
    for (int square = 0; square < 64; square++)
    {
        // get bishop and rook masks
        bishopMasks[square] = maskBishopAttacks(square);
        rookMasks[square] = maskRookAttacks(square);

        // get current mask (bishop or rook)
        U64 attackMask = bishop ? bishopMasks[square] : rookMasks[square];

        // initialise relevant bitcount for bishop or rook
        int bitCount = countBits(attackMask);

        // get occupancy index
        int occupancyI = (1 << bitCount);

        // loop over all occupancies indices
        for (int index = 0; index < occupancyI; index++)
        {
            // for bishops
            if (bishop)
            {
                // get occupancy
                U64 occupancy = setOccupancy(index, bitCount, attackMask);

                // get magic index
                int magicIndex = (occupancy * bishopMagicNumbers[square]) >> (64 - bishopBits[square]);

                // get bishop attacks
                bishopAttacks[square][magicIndex] = bishopAttacksOTF(square, occupancy);
            }
            // for rooks
            else
            {
                // get occupancy
                U64 occupancy = setOccupancy(index, bitCount, attackMask);

                // get magic index
                int magicIndex = (occupancy * rookMagicNumbers[square]) >> (64 - rookBits[square]);

                // get bishop attacks
                rookAttacks[square][magicIndex] = rookAttacksOTF(square, occupancy);
            }
        }
    }
}
