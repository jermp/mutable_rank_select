#ifndef RANK_HPP
#define RANK_HPP

#include <iostream>

using namespace std;

namespace shared {

enum RankBasicType { RANK_BASIC_STANDARD = 1, RANK_BASIC_COMPRESSED_HEADERS = 2 };

template <RankBasicType T>
class RankBasic32 {
  private:
    void freeMemory() {
        if (this->ranks != NULL) delete[] this->ranks;
        if (this->bits != NULL) delete[] this->bits;
    }

    void initialize() {
        this->ranks = NULL;
        this->alignedRanks = NULL;
        this->ranksLen = 0;
        this->bits = NULL;
        this->alignedBits = NULL;
        this->bitsLen = 0;
        this->textLen = 0;
        this->pointer = NULL;
        this->pointer2 = NULL;
    }

    void build_std(unsigned char* text, unsigned int extendedTextLen, bool* emptyBlock, unsigned int notEmptyBlocks) {
        this->bitsLen = notEmptyBlocks * (BLOCK_LEN / sizeof(unsigned long long));
        this->bits = new unsigned long long[this->bitsLen + 16];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        this->ranksLen = 2 * (extendedTextLen / BLOCK_LEN) + 1;
        this->ranks = new unsigned int[this->ranksLen + 32];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned int rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            this->alignedRanks[rankCounter++] = rank;
            this->alignedRanks[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (j % sizeof(unsigned long long) == 0) this->alignedBits[bitsCounter] = 0;
                if (i + j < this->textLen)
                    this->alignedBits[bitsCounter] +=
                        (((unsigned long long)text[i + j])
                         << (8 * (sizeof(unsigned long long) - j % sizeof(unsigned long long) - 1)));
                if (j % sizeof(unsigned long long) == 7) rank += __builtin_popcountll(this->alignedBits[bitsCounter++]);
            }
        }
        this->alignedRanks[rankCounter++] = rank;
    }

    void build_bch(unsigned char* text, unsigned int extendedTextLen, bool* emptyBlock, unsigned int notEmptyBlocks) {
        this->bitsLen = notEmptyBlocks * sizeof(unsigned long long);
        this->bits = new unsigned long long[this->bitsLen + 16];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned int ranksTempLen = 2 * (extendedTextLen / BLOCK_LEN) + 2;
        if (ranksTempLen % (2 * SUPERBLOCKLEN) > 0)
            ranksTempLen += ((2 * SUPERBLOCKLEN) - ranksTempLen % (2 * SUPERBLOCKLEN));
        unsigned int* ranksTemp = new unsigned int[ranksTempLen];

        unsigned int rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter++] = rank;
            ranksTemp[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (j % sizeof(unsigned long long) == 0) this->alignedBits[bitsCounter] = 0;
                if (i + j < this->textLen)
                    this->alignedBits[bitsCounter] +=
                        (((unsigned long long)text[i + j])
                         << (sizeof(unsigned long long) *
                             (sizeof(unsigned long long) - j % sizeof(unsigned long long) - 1)));
                if (j % sizeof(unsigned long long) == 7) rank += __builtin_popcountll(this->alignedBits[bitsCounter++]);
            }
        }
        ranksTemp[rankCounter++] = rank;
        ranksTemp[rankCounter++] = 0;

        for (unsigned int i = rankCounter; i < ranksTempLen; ++i) {
            ranksTemp[i] = 0;
        }

        this->ranksLen = ranksTempLen / 2;
        this->ranksLen = (this->ranksLen / SUPERBLOCKLEN) * INTSINSUPERBLOCK;
        ranksTempLen /= 2;

        this->ranks = new unsigned int[this->ranksLen + 32];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned int bucket[INTSINSUPERBLOCK];
        rankCounter = 0;
        for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) bucket[j] = 0;

        for (unsigned int i = 0; i < ranksTempLen; i += SUPERBLOCKLEN) {
            bucket[0] = ranksTemp[2 * i];
            for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                bucket[j / 2 + 1] += ((ranksTemp[2 * (i + j + 1)] - bucket[0]) << (16 * (j % 2)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK] = ranksTemp[2 * i + 1];
            for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                // AddressSanitizer: heap-buffer-overflow?
                if (!emptyBlock[i + j]) bucket[OFFSETSTARTINSUPERBLOCK + 1] += (1U << (31 - j));
            }
            for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) {
                this->alignedRanks[rankCounter++] = bucket[j];
                bucket[j] = 0;
            }
        }
        delete[] ranksTemp;
    }

    unsigned int getRank_std(unsigned int i) {
        unsigned int start = i / BLOCK_IN_BITS;
        unsigned int rank = this->alignedRanks[2 * start];
        i %= BLOCK_IN_BITS;
        unsigned int rankDiff = this->alignedRanks[2 * start + 2] - rank;
        if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
        this->pointer = this->alignedBits + this->alignedRanks[2 * start + 1];
        switch (i / 64) {
            case 7:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                rank += __builtin_popcountll(this->pointer[6]);
                return rank + __builtin_popcountll(this->pointer[7] & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                return rank + __builtin_popcountll(this->pointer[6] & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                return rank + __builtin_popcountll(this->pointer[5] & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                return rank + __builtin_popcountll(this->pointer[4] & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                return rank + __builtin_popcountll(this->pointer[3] & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                return rank + __builtin_popcountll(this->pointer[2] & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(this->pointer[0]);
                return rank + __builtin_popcountll(this->pointer[1] & masks[i - 64]);
            default:
                return rank + __builtin_popcountll(this->pointer[0] & masks[i]);
        }
    }

    unsigned int getRank_bch(unsigned int i) {
        unsigned int start = i / BLOCK_IN_BITS;
        this->pointer2 = this->alignedRanks + INTSINSUPERBLOCK * (start / SUPERBLOCKLEN);
        unsigned int rank = *(this->pointer2);
        unsigned int rankNumInCL = (start % SUPERBLOCKLEN);
        if (rankNumInCL > 0) {
            if (rankNumInCL % 2 == 1)
                rank += (this->pointer2[(rankNumInCL + 1) / 2] & 0xFFFF);
            else
                rank += (this->pointer2[(rankNumInCL + 1) / 2] >> 16);
        }
        i %= BLOCK_IN_BITS;
        unsigned int rank2;
        if (rankNumInCL == (SUPERBLOCKLEN - 1))
            rank2 = this->pointer2[INTSINSUPERBLOCK];
        else {
            rank2 = *(this->pointer2);
            if (rankNumInCL % 2 == 0)
                rank2 += (this->pointer2[(rankNumInCL + 2) / 2] & 0xFFFF);
            else
                rank2 += (this->pointer2[(rankNumInCL + 2) / 2] >> 16);
        }
        unsigned int rankDiff = rank2 - rank;
        if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
        if (rankNumInCL > 0)
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] +
                            8 * __builtin_popcount(this->pointer2[OFFSETSTARTINSUPERBLOCK + 1] & masks2[rankNumInCL]);
        else
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK];
        switch (i / 64) {
            case 7:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                rank += __builtin_popcountll(this->pointer[6]);
                return rank + __builtin_popcountll(this->pointer[7] & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                return rank + __builtin_popcountll(this->pointer[6] & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                return rank + __builtin_popcountll(this->pointer[5] & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                return rank + __builtin_popcountll(this->pointer[4] & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                return rank + __builtin_popcountll(this->pointer[3] & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                return rank + __builtin_popcountll(this->pointer[2] & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(this->pointer[0]);
                return rank + __builtin_popcountll(this->pointer[1] & masks[i - 64]);
            default:
                return rank + __builtin_popcountll(this->pointer[0] & masks[i]);
        }
    }

  public:
    unsigned int* ranks;
    unsigned int* alignedRanks;
    unsigned int ranksLen;
    unsigned long long* bits;
    unsigned long long* alignedBits;
    unsigned int bitsLen;
    unsigned int textLen;
    unsigned long long* pointer;
    unsigned int* pointer2;

    const static unsigned int BLOCK_LEN = sizeof(unsigned long long) * 8;
    const static unsigned int BLOCK_IN_BITS = 8 * BLOCK_LEN;

    const static unsigned int BLOCK_MASK = (1 << 9) - 1;

    const static unsigned int SUPERBLOCKLEN = 32;
    const static unsigned int OFFSETSTARTINSUPERBLOCK = 17;
    const static unsigned int INTSINSUPERBLOCK = 19;
    /*
    INTSINSUPERBLOCK = 2 + (SUPERBLOCKLEN - 1) / 2 + (SUPERBLOCKLEN - 1) / 32;
    if ((SUPERBLOCKLEN - 1) % 2 == 1) ++INTSINSUPERBLOCK;
    if ((SUPERBLOCKLEN - 1) % 32 > 0) ++INTSINSUPERBLOCK;
    *
    OFFSETSTARTINSUPERBLOCK = 1 + (SUPERBLOCKLEN - 1) / 2;
    if ((SUPERBLOCKLEN - 1) % 2 == 1) ++OFFSETSTARTINSUPERBLOCK;
    */

    const unsigned long long masks[64] = {
        0x0000000000000000, 0x8000000000000000, 0xC000000000000000, 0xE000000000000000, 0xF000000000000000,
        0xF800000000000000, 0xFC00000000000000, 0xFE00000000000000, 0xFF00000000000000, 0xFF80000000000000,
        0xFFC0000000000000, 0xFFE0000000000000, 0xFFF0000000000000, 0xFFF8000000000000, 0xFFFC000000000000,
        0xFFFE000000000000, 0xFFFF000000000000, 0xFFFF800000000000, 0xFFFFC00000000000, 0xFFFFE00000000000,
        0xFFFFF00000000000, 0xFFFFF80000000000, 0xFFFFFC0000000000, 0xFFFFFE0000000000, 0xFFFFFF0000000000,
        0xFFFFFF8000000000, 0xFFFFFFC000000000, 0xFFFFFFE000000000, 0xFFFFFFF000000000, 0xFFFFFFF800000000,
        0xFFFFFFFC00000000, 0xFFFFFFFE00000000, 0xFFFFFFFF00000000, 0xFFFFFFFF80000000, 0xFFFFFFFFC0000000,
        0xFFFFFFFFE0000000, 0xFFFFFFFFF0000000, 0xFFFFFFFFF8000000, 0xFFFFFFFFFC000000, 0xFFFFFFFFFE000000,
        0xFFFFFFFFFF000000, 0xFFFFFFFFFF800000, 0xFFFFFFFFFFC00000, 0xFFFFFFFFFFE00000, 0xFFFFFFFFFFF00000,
        0xFFFFFFFFFFF80000, 0xFFFFFFFFFFFC0000, 0xFFFFFFFFFFFE0000, 0xFFFFFFFFFFFF0000, 0xFFFFFFFFFFFF8000,
        0xFFFFFFFFFFFFC000, 0xFFFFFFFFFFFFE000, 0xFFFFFFFFFFFFF000, 0xFFFFFFFFFFFFF800, 0xFFFFFFFFFFFFFC00,
        0xFFFFFFFFFFFFFE00, 0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFFFF80, 0xFFFFFFFFFFFFFFC0, 0xFFFFFFFFFFFFFFE0,
        0xFFFFFFFFFFFFFFF0, 0xFFFFFFFFFFFFFFF8, 0xFFFFFFFFFFFFFFFC, 0xFFFFFFFFFFFFFFFE};

    const unsigned int masks2[32] = {0x00000000, 0x80000000, 0xC0000000, 0xE0000000, 0xF0000000, 0xF8000000, 0xFC000000,
                                     0xFE000000, 0xFF000000, 0xFF800000, 0xFFC00000, 0xFFE00000, 0xFFF00000, 0xFFF80000,
                                     0xFFFC0000, 0xFFFE0000, 0xFFFF0000, 0xFFFF8000, 0xFFFFC000, 0xFFFFE000, 0xFFFFF000,
                                     0xFFFFF800, 0xFFFFFC00, 0xFFFFFE00, 0xFFFFFF00, 0xFFFFFF80, 0xFFFFFFC0, 0xFFFFFFE0,
                                     0xFFFFFFF0, 0xFFFFFFF8, 0xFFFFFFFC, 0xFFFFFFFE};

    RankBasic32() {
        this->initialize();
    }

    ~RankBasic32() {
        this->freeMemory();
    }

    void build(unsigned char* text, unsigned int textLen) {
        this->free();
        this->textLen = textLen;
        unsigned int extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned int notEmptyBlocks = 0;
        for (unsigned int i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        switch (T) {
            case RankBasicType::RANK_BASIC_COMPRESSED_HEADERS:
                this->build_bch(text, extendedTextLen, emptyBlock, notEmptyBlocks);
                break;
            default:
                this->build_std(text, extendedTextLen, emptyBlock, notEmptyBlocks);
                break;
        }

        delete[] emptyBlock;
    }

    unsigned int rank(unsigned int i) {
        switch (T) {
            case RankBasicType::RANK_BASIC_COMPRESSED_HEADERS:
                return this->getRank_bch(i);
                break;
            default:
                return this->getRank_std(i);
                break;
        }
    }

    void save(FILE* outFile) {
        fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        fwrite(&this->ranksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        if (this->ranksLen > 0)
            fwrite(this->alignedRanks, (size_t)sizeof(unsigned int), (size_t)this->ranksLen, outFile);
        fwrite(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        if (this->bitsLen > 0)
            fwrite(this->alignedBits, (size_t)sizeof(unsigned long long), (size_t)this->bitsLen, outFile);
    }

    void load(FILE* inFile) {
        this->free();
        size_t result;
        result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->ranksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->ranksLen > 0) {
            this->ranks = new unsigned int[this->ranksLen + 32];
            this->alignedRanks = this->ranks;
            while ((unsigned long long)(this->alignedRanks) % 128) ++(this->alignedRanks);
            result = fread(this->alignedRanks, (size_t)sizeof(unsigned int), (size_t)this->ranksLen, inFile);
            if (result != this->ranksLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
        result = fread(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->bitsLen > 0) {
            this->bits = new unsigned long long[this->bitsLen + 16];
            this->alignedBits = this->bits;
            while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
            result = fread(this->alignedBits, (size_t)sizeof(unsigned long long), (size_t)this->bitsLen, inFile);
            if (result != this->bitsLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
    }

    void free() {
        this->freeMemory();
        this->initialize();
    }

    unsigned long long getSize() {
        unsigned long long size =
            sizeof(this->ranksLen) + sizeof(this->bitsLen) + sizeof(this->pointer) + sizeof(this->pointer2);
        if (this->ranksLen > 0) size += (this->ranksLen + 32) * sizeof(unsigned int);
        if (this->bitsLen > 0) size += (this->bitsLen + 16) * sizeof(unsigned long long);
        return size;
    }

    unsigned int getTextSize() {
        return this->textLen * sizeof(unsigned char);
    }

    void testRank(unsigned char* text, unsigned int textLen) {
        unsigned int rankTest = 0;

        for (unsigned int i = 0; i < textLen; ++i) {
            unsigned int bits = (unsigned int)text[i];
            for (unsigned int j = 0; j < 8; ++j) {
                if (((bits >> (7 - j)) & 1) == 1) ++rankTest;
                if (rankTest != this->rank(8 * i + j + 1)) {
                    cout << (8 * i + j + 1) << " " << rankTest << " " << this->rank(8 * i + j + 1);
                    cin.ignore();
                }
            }
        }
    }
};

template <RankBasicType T>
class RankBasic64 {
  private:
    void freeMemory() {
        if (this->ranks != NULL) delete[] this->ranks;
        if (this->bits != NULL) delete[] this->bits;
    }

    void initialize() {
        this->ranks = NULL;
        this->alignedRanks = NULL;
        this->ranksLen = 0;
        this->bits = NULL;
        this->alignedBits = NULL;
        this->bitsLen = 0;
        this->textLen = 0;
        this->pointer = NULL;
        this->pointer2 = NULL;
    }

    void build_std(unsigned char* text, unsigned long long extendedTextLen, bool* emptyBlock,
                   unsigned long long notEmptyBlocks) {
        this->bitsLen = notEmptyBlocks * (BLOCK_LEN / sizeof(unsigned long long));
        this->bits = new unsigned long long[this->bitsLen + 16];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        this->ranksLen = 2 * (extendedTextLen / BLOCK_LEN) + 1;
        this->ranks = new unsigned long long[this->ranksLen + 16];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned long long rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            this->alignedRanks[rankCounter++] = rank;
            this->alignedRanks[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (j % sizeof(unsigned long long) == 0) this->alignedBits[bitsCounter] = 0;
                if (i + j < this->textLen)
                    this->alignedBits[bitsCounter] +=
                        (((unsigned long long)text[i + j])
                         << (8 * (sizeof(unsigned long long) - j % sizeof(unsigned long long) - 1)));
                if (j % sizeof(unsigned long long) == 7) rank += __builtin_popcountll(this->alignedBits[bitsCounter++]);
            }
        }
        this->alignedRanks[rankCounter++] = rank;
    }

    void build_bch(unsigned char* text, unsigned long long extendedTextLen, bool* emptyBlock,
                   unsigned long long notEmptyBlocks) {
        this->bitsLen = notEmptyBlocks * sizeof(unsigned long long);
        this->bits = new unsigned long long[this->bitsLen + 16];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned long long ranksTempLen = 2 * (extendedTextLen / BLOCK_LEN) + 2;
        if (ranksTempLen % (2 * SUPERBLOCKLEN) > 0)
            ranksTempLen += ((2 * SUPERBLOCKLEN) - ranksTempLen % (2 * SUPERBLOCKLEN));
        unsigned long long* ranksTemp = new unsigned long long[ranksTempLen];

        unsigned long long rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter++] = rank;
            ranksTemp[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (j % sizeof(unsigned long long) == 0) this->alignedBits[bitsCounter] = 0;
                if (i + j < this->textLen)
                    this->alignedBits[bitsCounter] +=
                        (((unsigned long long)text[i + j])
                         << (sizeof(unsigned long long) *
                             (sizeof(unsigned long long) - j % sizeof(unsigned long long) - 1)));
                if (j % sizeof(unsigned long long) == 7) rank += __builtin_popcountll(this->alignedBits[bitsCounter++]);
            }
        }
        ranksTemp[rankCounter++] = rank;
        ranksTemp[rankCounter++] = 0;

        for (unsigned long long i = rankCounter; i < ranksTempLen; ++i) {
            ranksTemp[i] = 0;
        }

        this->ranksLen = ranksTempLen / 2;
        this->ranksLen = (this->ranksLen / SUPERBLOCKLEN) * LONGSINSUPERBLOCK;
        ranksTempLen /= 2;

        this->ranks = new unsigned long long[this->ranksLen + 16];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned long long bucket[LONGSINSUPERBLOCK];
        rankCounter = 0;
        for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) bucket[j] = 0;

        for (unsigned long long i = 0; i < ranksTempLen; i += SUPERBLOCKLEN) {
            bucket[0] = ranksTemp[2 * i];
            for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                bucket[j / 4 + 1] += ((ranksTemp[2 * (i + j + 1)] - bucket[0]) << (16 * (j % 4)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK] = ranksTemp[2 * i + 1];
            for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                if (!emptyBlock[i + j]) bucket[OFFSETSTARTINSUPERBLOCK + 1] += (1ULL << (63 - j));
            }
            for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) {
                this->alignedRanks[rankCounter++] = bucket[j];
                bucket[j] = 0;
            }
        }
        delete[] ranksTemp;
    }

    unsigned long long getRank_std(unsigned long long i) {
        unsigned long long start = i / BLOCK_IN_BITS;
        unsigned long long rank = this->alignedRanks[2 * start];
        i %= BLOCK_IN_BITS;
        unsigned long long rankDiff = this->alignedRanks[2 * start + 2] - rank;
        if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
        this->pointer = this->alignedBits + this->alignedRanks[2 * start + 1];
        switch (i / 64) {
            case 7:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                rank += __builtin_popcountll(this->pointer[6]);
                return rank + __builtin_popcountll(this->pointer[7] & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                return rank + __builtin_popcountll(this->pointer[6] & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                return rank + __builtin_popcountll(this->pointer[5] & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                return rank + __builtin_popcountll(this->pointer[4] & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                return rank + __builtin_popcountll(this->pointer[3] & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                return rank + __builtin_popcountll(this->pointer[2] & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(this->pointer[0]);
                return rank + __builtin_popcountll(this->pointer[1] & masks[i - 64]);
            default:
                return rank + __builtin_popcountll(this->pointer[0] & masks[i]);
        }
    }

    unsigned long long getRank_bch(unsigned long long i) {
        unsigned long long start = i / BLOCK_IN_BITS;
        this->pointer2 = this->alignedRanks + LONGSINSUPERBLOCK * (start / SUPERBLOCKLEN);
        unsigned long long rank = *(this->pointer2);
        unsigned long long rankNumInCL = (start % SUPERBLOCKLEN);
        if (rankNumInCL > 0) {
            switch (rankNumInCL % 4) {
                case 3:
                    rank += ((this->pointer2[(rankNumInCL + 3) / 4] >> 32) & 0xFFFF);
                    break;
                case 2:
                    rank += ((this->pointer2[(rankNumInCL + 3) / 4] >> 16) & 0xFFFF);
                    break;
                case 1:
                    rank += (this->pointer2[(rankNumInCL + 3) / 4] & 0xFFFF);
                    break;
                default:
                    rank += (this->pointer2[(rankNumInCL + 3) / 4] >> 48);
                    break;
            }
        }
        i %= BLOCK_IN_BITS;
        unsigned long long rank2;
        if (rankNumInCL == (SUPERBLOCKLEN - 1))
            rank2 = this->pointer2[LONGSINSUPERBLOCK];
        else {
            rank2 = *(this->pointer2);
            switch (rankNumInCL % 4) {
                case 3:
                    rank2 += (this->pointer2[(rankNumInCL + 3) / 4] >> 48);
                    break;
                case 2:
                    rank2 += ((this->pointer2[(rankNumInCL + 3) / 4] >> 32) & 0xFFFF);
                    break;
                case 1:
                    rank2 += ((this->pointer2[(rankNumInCL + 3) / 4] >> 16) & 0xFFFF);
                    break;
                default:
                    rank2 += (this->pointer2[(rankNumInCL + 4) / 4] & 0xFFFF);
                    break;
            }
        }
        unsigned long long rankDiff = rank2 - rank;
        if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
        if (rankNumInCL > 0)
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] +
                            8 * __builtin_popcountll(this->pointer2[OFFSETSTARTINSUPERBLOCK + 1] & masks[rankNumInCL]);
        else
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK];
        switch (i / 64) {
            case 7:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                rank += __builtin_popcountll(this->pointer[6]);
                return rank + __builtin_popcountll(this->pointer[7] & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                return rank + __builtin_popcountll(this->pointer[6] & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                return rank + __builtin_popcountll(this->pointer[5] & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                return rank + __builtin_popcountll(this->pointer[4] & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                return rank + __builtin_popcountll(this->pointer[3] & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                return rank + __builtin_popcountll(this->pointer[2] & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(this->pointer[0]);
                return rank + __builtin_popcountll(this->pointer[1] & masks[i - 64]);
            default:
                return rank + __builtin_popcountll(this->pointer[0] & masks[i]);
        }
    }

  public:
    unsigned long long* ranks;
    unsigned long long* alignedRanks;
    unsigned long long ranksLen;
    unsigned long long* bits;
    unsigned long long* alignedBits;
    unsigned long long bitsLen;
    unsigned long long textLen;
    unsigned long long* pointer;
    unsigned long long* pointer2;

    const static unsigned long long BLOCK_LEN = sizeof(unsigned long long) * 8;
    const static unsigned long long BLOCK_IN_BITS = 8 * BLOCK_LEN;

    const static unsigned long long BLOCK_MASK = (1 << 9) - 1;

    const static unsigned long long SUPERBLOCKLEN = 64;
    const static unsigned long long OFFSETSTARTINSUPERBLOCK = 17;
    const static unsigned long long LONGSINSUPERBLOCK = 19;
    /*
    LONGSINSUPERBLOCK = 2 + (SUPERBLOCKLEN - 1) / 4 + (SUPERBLOCKLEN - 1) / 64;
    if ((SUPERBLOCKLEN - 1) % 4 > 0) ++LONGSINSUPERBLOCK;
    if ((SUPERBLOCKLEN - 1) % 64 > 0) ++LONGSINSUPERBLOCK;
    *
    OFFSETSTARTINSUPERBLOCK = 1 + (SUPERBLOCKLEN - 1) / 4;
    if ((SUPERBLOCKLEN - 1) % 4 > 0) ++OFFSETSTARTINSUPERBLOCK;
    */

    const unsigned long long masks[64] = {
        0x0000000000000000, 0x8000000000000000, 0xC000000000000000, 0xE000000000000000, 0xF000000000000000,
        0xF800000000000000, 0xFC00000000000000, 0xFE00000000000000, 0xFF00000000000000, 0xFF80000000000000,
        0xFFC0000000000000, 0xFFE0000000000000, 0xFFF0000000000000, 0xFFF8000000000000, 0xFFFC000000000000,
        0xFFFE000000000000, 0xFFFF000000000000, 0xFFFF800000000000, 0xFFFFC00000000000, 0xFFFFE00000000000,
        0xFFFFF00000000000, 0xFFFFF80000000000, 0xFFFFFC0000000000, 0xFFFFFE0000000000, 0xFFFFFF0000000000,
        0xFFFFFF8000000000, 0xFFFFFFC000000000, 0xFFFFFFE000000000, 0xFFFFFFF000000000, 0xFFFFFFF800000000,
        0xFFFFFFFC00000000, 0xFFFFFFFE00000000, 0xFFFFFFFF00000000, 0xFFFFFFFF80000000, 0xFFFFFFFFC0000000,
        0xFFFFFFFFE0000000, 0xFFFFFFFFF0000000, 0xFFFFFFFFF8000000, 0xFFFFFFFFFC000000, 0xFFFFFFFFFE000000,
        0xFFFFFFFFFF000000, 0xFFFFFFFFFF800000, 0xFFFFFFFFFFC00000, 0xFFFFFFFFFFE00000, 0xFFFFFFFFFFF00000,
        0xFFFFFFFFFFF80000, 0xFFFFFFFFFFFC0000, 0xFFFFFFFFFFFE0000, 0xFFFFFFFFFFFF0000, 0xFFFFFFFFFFFF8000,
        0xFFFFFFFFFFFFC000, 0xFFFFFFFFFFFFE000, 0xFFFFFFFFFFFFF000, 0xFFFFFFFFFFFFF800, 0xFFFFFFFFFFFFFC00,
        0xFFFFFFFFFFFFFE00, 0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFFFF80, 0xFFFFFFFFFFFFFFC0, 0xFFFFFFFFFFFFFFE0,
        0xFFFFFFFFFFFFFFF0, 0xFFFFFFFFFFFFFFF8, 0xFFFFFFFFFFFFFFFC, 0xFFFFFFFFFFFFFFFE};

    RankBasic64() {
        this->initialize();
    }

    ~RankBasic64() {
        this->freeMemory();
    }

    void build(unsigned char* text, unsigned long long textLen) {
        this->free();
        this->textLen = textLen;
        unsigned long long extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned long long notEmptyBlocks = 0;
        for (unsigned long long i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        switch (T) {
            case RankBasicType::RANK_BASIC_COMPRESSED_HEADERS:
                this->build_bch(text, extendedTextLen, emptyBlock, notEmptyBlocks);
                break;
            default:
                this->build_std(text, extendedTextLen, emptyBlock, notEmptyBlocks);
                break;
        }

        delete[] emptyBlock;
    }

    unsigned long long rank(unsigned long long i) {
        switch (T) {
            case RankBasicType::RANK_BASIC_COMPRESSED_HEADERS:
                return this->getRank_bch(i);
                break;
            default:
                return this->getRank_std(i);
                break;
        }
    }

    void save(FILE* outFile) {
        fwrite(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        fwrite(&this->ranksLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        if (this->ranksLen > 0)
            fwrite(this->alignedRanks, (size_t)sizeof(unsigned long long), (size_t)this->ranksLen, outFile);
        fwrite(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        if (this->bitsLen > 0)
            fwrite(this->alignedBits, (size_t)sizeof(unsigned long long), (size_t)this->bitsLen, outFile);
    }

    void load(FILE* inFile) {
        this->free();
        size_t result;
        result = fread(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->ranksLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->ranksLen > 0) {
            this->ranks = new unsigned long long[this->ranksLen + 16];
            this->alignedRanks = this->ranks;
            while ((unsigned long long)(this->alignedRanks) % 128) ++(this->alignedRanks);
            result = fread(this->alignedRanks, (size_t)sizeof(unsigned long long), (size_t)this->ranksLen, inFile);
            if (result != this->ranksLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
        result = fread(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->bitsLen > 0) {
            this->bits = new unsigned long long[this->bitsLen + 16];
            this->alignedBits = this->bits;
            while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
            result = fread(this->alignedBits, (size_t)sizeof(unsigned long long), (size_t)this->bitsLen, inFile);
            if (result != this->bitsLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
    }

    void free() {
        this->freeMemory();
        this->initialize();
    }

    unsigned long long getSize() {
        unsigned long long size =
            sizeof(this->ranksLen) + sizeof(this->bitsLen) + sizeof(this->pointer) + sizeof(this->pointer2);
        if (this->ranksLen > 0) size += (this->ranksLen + 16) * sizeof(unsigned long long);
        if (this->bitsLen > 0) size += (this->bitsLen + 16) * sizeof(unsigned long long);
        return size;
    }

    unsigned long long getTextSize() {
        return this->textLen * sizeof(unsigned char);
    }

    void testRank(unsigned char* text, unsigned long long textLen) {
        unsigned long long rankTest = 0;

        for (unsigned long long i = 0; i < textLen; ++i) {
            unsigned int bits = (unsigned int)text[i];
            for (unsigned int j = 0; j < 8; ++j) {
                if (((bits >> (7 - j)) & 1) == 1) ++rankTest;
                if (rankTest != this->rank(8 * i + j + 1)) {
                    cout << (8 * i + j + 1) << " " << rankTest << " " << this->rank(8 * i + j + 1);
                    cin.ignore();
                }
            }
        }
    }
};

class RankCF32 {
  private:
    void freeMemory() {
        if (this->bits != NULL) delete[] this->bits;
    }

    void initialize() {
        this->bits = NULL;
        this->alignedBits = NULL;
        this->bitsLen = 0;
        this->textLen = 0;
        this->border = 0;
        this->blockBorder = 0;
        this->pointer = NULL;
    }

  public:
    unsigned int* bits;
    unsigned int* alignedBits;
    unsigned int bitsLen;
    unsigned int textLen;
    unsigned int border;
    unsigned int blockBorder;
    unsigned int* pointer;

    const static unsigned int INT_BLOCK_LEN = 16;
    const static unsigned int INT_BLOCK_LEN_WITH_RANK = INT_BLOCK_LEN + 1;
    const static unsigned int BLOCK_LEN = sizeof(unsigned int) * INT_BLOCK_LEN;
    const static unsigned int BLOCK_IN_BITS = 8 * BLOCK_LEN;

    const static unsigned int BLOCK_MASK = (1 << 9) - 1;

    unsigned long long masks[64] = {
        0x0000000000000000ULL, 0x0000000080000000ULL, 0x00000000C0000000ULL, 0x00000000E0000000ULL,
        0x00000000F0000000ULL, 0x00000000F8000000ULL, 0x00000000FC000000ULL, 0x00000000FE000000ULL,
        0x00000000FF000000ULL, 0x00000000FF800000ULL, 0x00000000FFC00000ULL, 0x00000000FFE00000ULL,
        0x00000000FFF00000ULL, 0x00000000FFF80000ULL, 0x00000000FFFC0000ULL, 0x00000000FFFE0000ULL,
        0x00000000FFFF0000ULL, 0x00000000FFFF8000ULL, 0x00000000FFFFC000ULL, 0x00000000FFFFE000ULL,
        0x00000000FFFFF000ULL, 0x00000000FFFFF800ULL, 0x00000000FFFFFC00ULL, 0x00000000FFFFFE00ULL,
        0x00000000FFFFFF00ULL, 0x00000000FFFFFF80ULL, 0x00000000FFFFFFC0ULL, 0x00000000FFFFFFE0ULL,
        0x00000000FFFFFFF0ULL, 0x00000000FFFFFFF8ULL, 0x00000000FFFFFFFCULL, 0x00000000FFFFFFFEULL,
        0x00000000FFFFFFFFULL, 0x80000000FFFFFFFFULL, 0xC0000000FFFFFFFFULL, 0xE0000000FFFFFFFFULL,
        0xF0000000FFFFFFFFULL, 0xF8000000FFFFFFFFULL, 0xFC000000FFFFFFFFULL, 0xFE000000FFFFFFFFULL,
        0xFF000000FFFFFFFFULL, 0xFF800000FFFFFFFFULL, 0xFFC00000FFFFFFFFULL, 0xFFE00000FFFFFFFFULL,
        0xFFF00000FFFFFFFFULL, 0xFFF80000FFFFFFFFULL, 0xFFFC0000FFFFFFFFULL, 0xFFFE0000FFFFFFFFULL,
        0xFFFF0000FFFFFFFFULL, 0xFFFF8000FFFFFFFFULL, 0xFFFFC000FFFFFFFFULL, 0xFFFFE000FFFFFFFFULL,
        0xFFFFF000FFFFFFFFULL, 0xFFFFF800FFFFFFFFULL, 0xFFFFFC00FFFFFFFFULL, 0xFFFFFE00FFFFFFFFULL,
        0xFFFFFF00FFFFFFFFULL, 0xFFFFFF80FFFFFFFFULL, 0xFFFFFFC0FFFFFFFFULL, 0xFFFFFFE0FFFFFFFFULL,
        0xFFFFFFF0FFFFFFFFULL, 0xFFFFFFF8FFFFFFFFULL, 0xFFFFFFFCFFFFFFFFULL, 0xFFFFFFFEFFFFFFFFULL};

    RankCF32() {
        this->initialize();
    }

    ~RankCF32() {
        this->freeMemory();
    }

    void build(unsigned char* text, unsigned int textLen) {
        this->free();
        this->textLen = textLen;
        unsigned int extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned int notEmptyBlocks = 0;
        for (unsigned int i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        unsigned int ranksLen = 2 * (extendedTextLen / BLOCK_LEN) + 1;
        unsigned int* ranks = new unsigned int[ranksLen];

        unsigned int bitsTempLen = notEmptyBlocks * INT_BLOCK_LEN;
        unsigned int* bitsTemp = new unsigned int[bitsTempLen];

        unsigned int rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranks[rankCounter++] = rank;
            ranks[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (j % sizeof(unsigned int) == 0) bitsTemp[bitsCounter] = 0;
                if (i + j < this->textLen)
                    bitsTemp[bitsCounter] +=
                        (((unsigned int)text[i + j]) << (8 * (sizeof(unsigned int) - j % sizeof(unsigned int) - 1)));
                if (j % sizeof(unsigned int) == 3) rank += __builtin_popcount(bitsTemp[bitsCounter++]);
            }
        }
        ranks[rankCounter++] = rank;

        unsigned int curEmptyBlocksL = 0;
        unsigned int curNotEmptyBlocksR = notEmptyBlocks;

        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            if (emptyBlock[i / BLOCK_LEN])
                ++curEmptyBlocksL;
            else
                --curNotEmptyBlocksR;
            if (curEmptyBlocksL >= curNotEmptyBlocksR) {
                this->border = i / BLOCK_LEN + 1;
                this->blockBorder = this->border * INT_BLOCK_LEN_WITH_RANK;
                break;
            }
        }

        this->bitsLen =
            (this->border - 1) * INT_BLOCK_LEN_WITH_RANK + (extendedTextLen / BLOCK_LEN - (this->border - 1)) * 2 + 16;
        this->bits = new unsigned int[this->bitsLen + 32];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned int LCounter = 0;
        unsigned int RCounter = this->border;
        rankCounter = 0;
        unsigned int offsetCounter = 2 * this->border + 1;

        for (unsigned int i = 0; i < (this->border * BLOCK_LEN); i += BLOCK_LEN) {
            this->alignedBits[LCounter++] = ranks[rankCounter];
            if (emptyBlock[i / BLOCK_LEN]) {
                while (true) {
                    if (!emptyBlock[RCounter]) break;
                    ++RCounter;
                    offsetCounter += 2;
                }
                unsigned int offset = ranks[offsetCounter];
                ranks[offsetCounter] = LCounter;
                for (unsigned int j = 0; j < INT_BLOCK_LEN; ++j) {
                    this->alignedBits[LCounter++] = bitsTemp[offset + j];
                }
                ++RCounter;
                offsetCounter += 2;
            } else {
                for (unsigned int j = 0; j < INT_BLOCK_LEN; ++j) {
                    this->alignedBits[LCounter++] = bitsTemp[ranks[rankCounter + 1] + j];
                }
            }
            rankCounter += 2;
        }
        for (unsigned int i = (2 * this->border); i < ranksLen; ++i) this->alignedBits[LCounter++] = ranks[i];

        delete[] ranks;
        delete[] bitsTemp;
        delete[] emptyBlock;
    }

    unsigned int rank(unsigned int i) {
        unsigned int start = i / BLOCK_IN_BITS;
        unsigned int rank;
        if (start < this->border) {
            unsigned int LStart = start * INT_BLOCK_LEN + start;
            this->pointer = this->alignedBits + LStart;
            rank = *(this->pointer);
            i %= BLOCK_IN_BITS;
            unsigned int rankDiff = *(this->pointer + INT_BLOCK_LEN_WITH_RANK) - rank;
            if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
            ++this->pointer;
        } else {
            unsigned int RStart = this->blockBorder + 2 * (start - this->border);
            rank = this->alignedBits[RStart];
            i %= BLOCK_IN_BITS;
            unsigned int rankDiff = this->alignedBits[RStart + 2] - rank;
            if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
            this->pointer = this->alignedBits + this->alignedBits[RStart + 1];
        }
        switch (i / 64) {
            case 7:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 2)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 4)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 6)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 10)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 12)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 14))) & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 2)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 4)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 6)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 10)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 12))) & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 2)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 4)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 6)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 10))) & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 2)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 4)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 6)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 8))) & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 2)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 4)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 6))) & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 2)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 4))) & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 2))) & masks[i - 64]);
            default:
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer))) & masks[i]);
        }
    }

    void save(FILE* outFile) {
        fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        fwrite(&this->border, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        fwrite(&this->blockBorder, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        fwrite(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        if (this->bitsLen > 0) fwrite(this->alignedBits, (size_t)sizeof(unsigned int), (size_t)this->bitsLen, outFile);
    }

    void load(FILE* inFile) {
        this->free();
        size_t result;
        result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->border, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->blockBorder, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->bitsLen > 0) {
            this->bits = new unsigned int[this->bitsLen + 32];
            this->alignedBits = this->bits;
            while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
            result = fread(this->alignedBits, (size_t)sizeof(unsigned int), (size_t)this->bitsLen, inFile);
            if (result != this->bitsLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
    }

    void free() {
        this->freeMemory();
        this->initialize();
    }

    unsigned int getSize() {
        unsigned int size = sizeof(this->border) + sizeof(this->blockBorder) + sizeof(this->bitsLen) +
                            sizeof(this->textLen) + sizeof(this->pointer);
        if (this->bitsLen > 0) size += (this->bitsLen + 32) * sizeof(unsigned int);
        return size;
    }

    unsigned int getTextSize() {
        return this->textLen * sizeof(unsigned char);
    }

    void testRank(unsigned char* text, unsigned int textLen) {
        unsigned int rankTest = 0;

        for (unsigned int i = 0; i < textLen; ++i) {
            unsigned int bits = (unsigned int)text[i];
            for (unsigned int j = 0; j < 8; ++j) {
                if (((bits >> (7 - j)) & 1) == 1) ++rankTest;
                if (rankTest != this->rank(8 * i + j + 1)) {
                    cout << (8 * i + j + 1) << " " << rankTest << " " << this->rank(8 * i + j + 1);
                    cin.ignore();
                }
            }
        }
    }
};

class RankCF64 {
  private:
    void freeMemory() {
        if (this->bits != NULL) delete[] this->bits;
    }

    void initialize() {
        this->bits = NULL;
        this->alignedBits = NULL;
        this->bitsLen = 0;
        this->textLen = 0;
        this->border = 0;
        this->blockBorder = 0;
        this->pointer = NULL;
    }

  public:
    unsigned long long* bits;
    unsigned long long* alignedBits;
    unsigned long long bitsLen;
    unsigned long long textLen;
    unsigned long long border;
    unsigned long long blockBorder;
    unsigned long long* pointer;

    const static unsigned long long LONG_BLOCK_LEN = 8;
    const static unsigned long long LONG_BLOCK_LEN_WITH_RANK = LONG_BLOCK_LEN + 1;
    const static unsigned long long BLOCK_LEN = sizeof(unsigned long long) * LONG_BLOCK_LEN;
    const static unsigned long long BLOCK_IN_BITS = 8 * BLOCK_LEN;

    const static unsigned long long BLOCK_MASK = (1 << 9) - 1;

    const unsigned long long masks[64] = {
        0x0000000000000000, 0x8000000000000000, 0xC000000000000000, 0xE000000000000000, 0xF000000000000000,
        0xF800000000000000, 0xFC00000000000000, 0xFE00000000000000, 0xFF00000000000000, 0xFF80000000000000,
        0xFFC0000000000000, 0xFFE0000000000000, 0xFFF0000000000000, 0xFFF8000000000000, 0xFFFC000000000000,
        0xFFFE000000000000, 0xFFFF000000000000, 0xFFFF800000000000, 0xFFFFC00000000000, 0xFFFFE00000000000,
        0xFFFFF00000000000, 0xFFFFF80000000000, 0xFFFFFC0000000000, 0xFFFFFE0000000000, 0xFFFFFF0000000000,
        0xFFFFFF8000000000, 0xFFFFFFC000000000, 0xFFFFFFE000000000, 0xFFFFFFF000000000, 0xFFFFFFF800000000,
        0xFFFFFFFC00000000, 0xFFFFFFFE00000000, 0xFFFFFFFF00000000, 0xFFFFFFFF80000000, 0xFFFFFFFFC0000000,
        0xFFFFFFFFE0000000, 0xFFFFFFFFF0000000, 0xFFFFFFFFF8000000, 0xFFFFFFFFFC000000, 0xFFFFFFFFFE000000,
        0xFFFFFFFFFF000000, 0xFFFFFFFFFF800000, 0xFFFFFFFFFFC00000, 0xFFFFFFFFFFE00000, 0xFFFFFFFFFFF00000,
        0xFFFFFFFFFFF80000, 0xFFFFFFFFFFFC0000, 0xFFFFFFFFFFFE0000, 0xFFFFFFFFFFFF0000, 0xFFFFFFFFFFFF8000,
        0xFFFFFFFFFFFFC000, 0xFFFFFFFFFFFFE000, 0xFFFFFFFFFFFFF000, 0xFFFFFFFFFFFFF800, 0xFFFFFFFFFFFFFC00,
        0xFFFFFFFFFFFFFE00, 0xFFFFFFFFFFFFFF00, 0xFFFFFFFFFFFFFF80, 0xFFFFFFFFFFFFFFC0, 0xFFFFFFFFFFFFFFE0,
        0xFFFFFFFFFFFFFFF0, 0xFFFFFFFFFFFFFFF8, 0xFFFFFFFFFFFFFFFC, 0xFFFFFFFFFFFFFFFE};

    RankCF64() {
        this->initialize();
    }

    ~RankCF64() {
        this->freeMemory();
    }

    void build(unsigned char* text, unsigned long long textLen) {
        this->free();
        this->textLen = textLen;
        unsigned long long extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned long long notEmptyBlocks = 0;
        for (unsigned long long i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        unsigned long long ranksLen = 2 * (extendedTextLen / BLOCK_LEN) + 1;
        unsigned long long* ranks = new unsigned long long[ranksLen];

        unsigned long long bitsTempLen = notEmptyBlocks * LONG_BLOCK_LEN;
        unsigned long long* bitsTemp = new unsigned long long[bitsTempLen];

        unsigned long long rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranks[rankCounter++] = rank;
            ranks[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (j % sizeof(unsigned long long) == 0) bitsTemp[bitsCounter] = 0;
                if (i + j < this->textLen)
                    bitsTemp[bitsCounter] += (((unsigned long long)text[i + j])
                                              << (sizeof(unsigned long long) *
                                                  (sizeof(unsigned long long) - j % sizeof(unsigned long long) - 1)));
                if (j % sizeof(unsigned long long) == 7) rank += __builtin_popcountll(bitsTemp[bitsCounter++]);
            }
        }
        ranks[rankCounter++] = rank;

        unsigned long long curEmptyBlocksL = 0;
        unsigned long long curNotEmptyBlocksR = notEmptyBlocks;

        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            if (emptyBlock[i / BLOCK_LEN])
                ++curEmptyBlocksL;
            else
                --curNotEmptyBlocksR;
            if (curEmptyBlocksL >= curNotEmptyBlocksR) {
                this->border = i / BLOCK_LEN + 1;
                this->blockBorder = this->border * LONG_BLOCK_LEN_WITH_RANK;
                break;
            }
        }

        this->bitsLen =
            (this->border - 1) * LONG_BLOCK_LEN_WITH_RANK + (extendedTextLen / BLOCK_LEN - (this->border - 1)) * 2 + 8;
        this->bits = new unsigned long long[this->bitsLen + 16];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned long long LCounter = 0;
        unsigned long long RCounter = this->border;
        rankCounter = 0;
        unsigned long long offsetCounter = 2 * this->border + 1;

        for (unsigned long long i = 0; i < (this->border * BLOCK_LEN); i += BLOCK_LEN) {
            this->alignedBits[LCounter++] = ranks[rankCounter];
            if (emptyBlock[i / BLOCK_LEN]) {
                while (true) {
                    if (!emptyBlock[RCounter]) break;
                    ++RCounter;
                    offsetCounter += 2;
                }
                unsigned long long offset = ranks[offsetCounter];
                ranks[offsetCounter] = LCounter;
                for (unsigned long long j = 0; j < LONG_BLOCK_LEN; ++j) {
                    this->alignedBits[LCounter++] = bitsTemp[offset + j];
                }
                ++RCounter;
                offsetCounter += 2;
            } else {
                for (unsigned long long j = 0; j < LONG_BLOCK_LEN; ++j) {
                    this->alignedBits[LCounter++] = bitsTemp[ranks[rankCounter + 1] + j];
                }
            }
            rankCounter += 2;
        }
        for (unsigned long long i = (2 * this->border); i < ranksLen; ++i) this->alignedBits[LCounter++] = ranks[i];

        delete[] ranks;
        delete[] bitsTemp;
        delete[] emptyBlock;
    }

    unsigned long long rank(unsigned long long i) {
        unsigned long long start = i / BLOCK_IN_BITS;
        unsigned long long rank;
        if (start < this->border) {
            unsigned long long LStart = start * LONG_BLOCK_LEN + start;
            this->pointer = this->alignedBits + LStart;
            rank = *(this->pointer);
            i %= BLOCK_IN_BITS;
            unsigned long long rankDiff = *(this->pointer + LONG_BLOCK_LEN_WITH_RANK) - rank;
            if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
            ++this->pointer;
        } else {
            unsigned long long RStart = this->blockBorder + 2 * (start - this->border);
            rank = this->alignedBits[RStart];
            i %= BLOCK_IN_BITS;
            unsigned long long rankDiff = this->alignedBits[RStart + 2] - rank;
            if ((rankDiff & BLOCK_MASK) == 0) return rank + i * (rankDiff >> 9);
            this->pointer = this->alignedBits + this->alignedBits[RStart + 1];
        }
        switch (i / 64) {
            case 7:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                rank += __builtin_popcountll(this->pointer[6]);
                return rank + __builtin_popcountll(this->pointer[7] & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                rank += __builtin_popcountll(this->pointer[5]);
                return rank + __builtin_popcountll(this->pointer[6] & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                rank += __builtin_popcountll(this->pointer[4]);
                return rank + __builtin_popcountll(this->pointer[5] & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                rank += __builtin_popcountll(this->pointer[3]);
                return rank + __builtin_popcountll(this->pointer[4] & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                rank += __builtin_popcountll(this->pointer[2]);
                return rank + __builtin_popcountll(this->pointer[3] & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(this->pointer[0]);
                rank += __builtin_popcountll(this->pointer[1]);
                return rank + __builtin_popcountll(this->pointer[2] & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(this->pointer[0]);
                return rank + __builtin_popcountll(this->pointer[1] & masks[i - 64]);
            default:
                return rank + __builtin_popcountll(this->pointer[0] & masks[i]);
        }
    }

    void save(FILE* outFile) {
        fwrite(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        fwrite(&this->border, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        fwrite(&this->blockBorder, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        fwrite(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        if (this->bitsLen > 0)
            fwrite(this->alignedBits, (size_t)sizeof(unsigned long long), (size_t)this->bitsLen, outFile);
    }

    void load(FILE* inFile) {
        this->free();
        size_t result;
        result = fread(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->border, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->blockBorder, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->bitsLen > 0) {
            this->bits = new unsigned long long[this->bitsLen + 16];
            this->alignedBits = this->bits;
            while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
            result = fread(this->alignedBits, (size_t)sizeof(unsigned long long), (size_t)this->bitsLen, inFile);
            if (result != this->bitsLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
    }

    void free() {
        this->freeMemory();
        this->initialize();
    }

    unsigned long long getSize() {
        unsigned long long size = sizeof(this->border) + sizeof(this->blockBorder) + sizeof(this->bitsLen) +
                                  sizeof(this->textLen) + sizeof(this->pointer);
        if (this->bitsLen > 0) size += (this->bitsLen + 16) * sizeof(unsigned long long);
        return size;
    }

    unsigned long long getTextSize() {
        return this->textLen * sizeof(unsigned char);
    }

    void testRank(unsigned char* text, unsigned long long textLen) {
        unsigned long long rankTest = 0;

        for (unsigned long long i = 0; i < textLen; ++i) {
            unsigned int bits = (unsigned int)text[i];
            for (unsigned int j = 0; j < 8; ++j) {
                if (((bits >> (7 - j)) & 1) == 1) ++rankTest;
                if (rankTest != this->rank(8 * i + j + 1)) {
                    cout << (8 * i + j + 1) << " " << rankTest << " " << this->rank(8 * i + j + 1);
                    cin.ignore();
                }
            }
        }
    }
};

enum RankMPEType { RANK_MPE1 = 1, RANK_MPE2 = 2, RANK_MPE3 = 3 };

template <RankMPEType T>
class RankMPE32 {
  private:
    void freeMemory() {
        if (this->ranks != NULL) delete[] this->ranks;
        if (this->bits != NULL) delete[] this->bits;
    }

    void initialize() {
        this->ranks = NULL;
        this->alignedRanks = NULL;
        this->ranksLen = 0;
        this->bits = NULL;
        this->alignedBits = NULL;
        this->bitsLen = 0;
        this->textLen = 0;
        this->pointer = NULL;
        this->pointer2 = NULL;
    }

    void build_v1(unsigned char* text, unsigned int textLen) {
        this->free();
        this->textLen = textLen;
        unsigned int extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned int notEmptyBlocks = 0;
        for (unsigned int i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        unsigned int bitsTempLen = notEmptyBlocks * BLOCK_LEN;
        unsigned char* bitsTemp = new unsigned char[bitsTempLen];

        unsigned int ranksTempLen = 2 * (extendedTextLen / BLOCK_LEN) + 2;
        if (ranksTempLen % (2 * SUPERBLOCKLEN) > 0)
            ranksTempLen += ((2 * SUPERBLOCKLEN) - ranksTempLen % (2 * SUPERBLOCKLEN));
        unsigned int* ranksTemp = new unsigned int[ranksTempLen];

        unsigned int rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter++] = rank;
            ranksTemp[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (i + j < this->textLen) {
                    bitsTemp[bitsCounter++] = (unsigned int)text[i + j];
                    rank += __builtin_popcount((unsigned int)text[i + j]);
                } else {
                    bitsTemp[bitsCounter++] = 0;
                }
            }
        }
        ranksTemp[rankCounter++] = rank;
        ranksTemp[rankCounter++] = 0;

        for (unsigned int i = rankCounter; i < ranksTempLen; ++i) {
            ranksTemp[i] = 0;
        }

        unsigned int pairsToRemove = 0;
        unsigned int pairsToRemoveInBlock = 0;
        unsigned int compressedBlocks = 0;
        unsigned int offset = 0;
        bool* blockToCompress = new bool[extendedTextLen / BLOCK_LEN];
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            blockToCompress[i / BLOCK_LEN] = false;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            pairsToRemoveInBlock = 0;
            for (unsigned int j = 0; j < BLOCK_LEN; j += 2) {
                if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) ||
                    (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255))
                    ++pairsToRemoveInBlock;
            }
            if (pairsToRemoveInBlock > 8) {
                pairsToRemove += pairsToRemoveInBlock;
                ++compressedBlocks;
                blockToCompress[i / BLOCK_LEN] = true;
            }
            offset += BLOCK_LEN;
        }

        this->bitsLen = compressedBlocks * (((BLOCK_LEN / 2) * 2) / 8) + (bitsTempLen - 2 * pairsToRemove) +
                        sizeof(unsigned long long);

        this->bits = new unsigned char[this->bitsLen + 128];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned long long bits1, bits2;
        bitsCounter = 0;
        rankCounter = 1;
        offset = 0;

        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter] = bitsCounter;
            rankCounter += 2;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            if (blockToCompress[i / BLOCK_LEN]) {
                bits1 = 0;
                bits2 = 0;
                bitsCounter += 16;
                for (unsigned int j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255) {
                        bits2 += (1ULL << (63 - (j / 2)));
                    }
                    if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) ||
                        (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255)) {
                        bits1 += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits1 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits1 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits1 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits1 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits1 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits1 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits1 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits1 >> 56);
                this->alignedBits[ranksTemp[rankCounter - 2] + 8] = (bits2 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 9] = ((bits2 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 10] = ((bits2 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 11] = ((bits2 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 12] = ((bits2 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 13] = ((bits2 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 14] = ((bits2 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 15] = (bits2 >> 56);
            } else {
                for (unsigned int j = 0; j < BLOCK_LEN; ++j) this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
            }
            offset += BLOCK_LEN;
        }
        for (unsigned int i = rankCounter; i < ranksTempLen; i += 2) ranksTemp[i] = ranksTemp[rankCounter - 2];

        delete[] bitsTemp;

        this->ranksLen = (ranksTempLen / (2 * SUPERBLOCKLEN)) * INTSINSUPERBLOCK;
        ranksTempLen /= 2;

        this->ranks = new unsigned int[this->ranksLen + 32];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned int bucket[INTSINSUPERBLOCK];
        rankCounter = 0;
        for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) bucket[j] = 0;

        for (unsigned int i = 0; i < ranksTempLen; i += SUPERBLOCKLEN) {
            bucket[0] = ranksTemp[2 * i];
            for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                bucket[j / 2 + 1] += ((ranksTemp[2 * (i + j + 1)] - bucket[0]) << (16 * (j % 2)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK] = ranksTemp[2 * i + 1];
            for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                unsigned int offsetDiff = (ranksTemp[2 * (i + j + 1) + 1] - bucket[OFFSETSTARTINSUPERBLOCK]);
                if (blockToCompress[i + j + 1]) offsetDiff += (1 << 15);
                bucket[OFFSETSTARTINSUPERBLOCK + 1 + j / 2] += (offsetDiff << (16 * (j % 2)));
            }
            if (blockToCompress[i]) bucket[OFFSETSTARTINSUPERBLOCK + 1] += (1 << 30);
            for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) {
                this->alignedRanks[rankCounter++] = bucket[j];
                bucket[j] = 0;
            }
        }

        delete[] blockToCompress;
        delete[] emptyBlock;
        delete[] ranksTemp;
    }

    void build_v2(unsigned char* text, unsigned int textLen) {
        this->free();
        this->textLen = textLen;
        unsigned int extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned int notEmptyBlocks = 0;
        for (unsigned int i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        unsigned int bitsTempLen = notEmptyBlocks * BLOCK_LEN;
        unsigned char* bitsTemp = new unsigned char[bitsTempLen];

        unsigned int ranksTempLen = 2 * (extendedTextLen / BLOCK_LEN) + 2;
        if (ranksTempLen % (2 * SUPERBLOCKLEN) > 0)
            ranksTempLen += ((2 * SUPERBLOCKLEN) - ranksTempLen % (2 * SUPERBLOCKLEN));
        unsigned int* ranksTemp = new unsigned int[ranksTempLen];

        unsigned int rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter++] = rank;
            ranksTemp[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned int j = 0; j < BLOCK_LEN; ++j) {
                if (i + j < this->textLen) {
                    bitsTemp[bitsCounter++] = (unsigned int)text[i + j];
                    rank += __builtin_popcount((unsigned int)text[i + j]);
                } else {
                    bitsTemp[bitsCounter++] = 0;
                }
            }
        }
        ranksTemp[rankCounter++] = rank;
        ranksTemp[rankCounter++] = 0;

        for (unsigned int i = rankCounter; i < ranksTempLen; ++i) {
            ranksTemp[i] = 0;
        }

        unsigned int pairsToRemove = 0;
        unsigned int pairs0ToRemoveInBlock = 0;
        unsigned int pairs255ToRemoveInBlock = 0;
        unsigned int compressedBlocks = 0;
        unsigned int compressed0Blocks = 0;
        unsigned int compressed255Blocks = 0;
        unsigned int offset = 0;
        unsigned char* blockToCompress = new unsigned char[extendedTextLen / BLOCK_LEN];
        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            blockToCompress[i / BLOCK_LEN] = 0;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            pairs0ToRemoveInBlock = 0;
            pairs255ToRemoveInBlock = 0;
            for (unsigned int j = 0; j < BLOCK_LEN; j += 2) {
                if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0)) ++pairs0ToRemoveInBlock;
                if ((bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255)) ++pairs255ToRemoveInBlock;
            }
            switch (T) {
                case RankMPEType::RANK_MPE2:
                    if (pairs0ToRemoveInBlock > 4 && pairs255ToRemoveInBlock > 4) {
                        pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                        ++compressedBlocks;
                        blockToCompress[i / BLOCK_LEN] = 3;
                    } else if (pairs0ToRemoveInBlock > 4 && pairs255ToRemoveInBlock <= 4) {
                        pairsToRemove += pairs0ToRemoveInBlock;
                        ++compressed0Blocks;
                        blockToCompress[i / BLOCK_LEN] = 1;
                    } else if (pairs0ToRemoveInBlock <= 4 && pairs255ToRemoveInBlock > 4) {
                        pairsToRemove += pairs255ToRemoveInBlock;
                        ++compressed255Blocks;
                        blockToCompress[i / BLOCK_LEN] = 2;
                    }
                    break;
                default:
                    if (pairs0ToRemoveInBlock > PAIRS0THR && pairs255ToRemoveInBlock > PAIRS255THR) {
                        pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                        ++compressedBlocks;
                        blockToCompress[i / BLOCK_LEN] = 3;
                    } else if (pairs0ToRemoveInBlock >= PAIRS0THR * 2 && pairs255ToRemoveInBlock <= PAIRS255THR) {
                        pairsToRemove += pairs0ToRemoveInBlock;
                        ++compressed0Blocks;
                        blockToCompress[i / BLOCK_LEN] = 1;
                    } else if (pairs0ToRemoveInBlock <= PAIRS0THR && pairs255ToRemoveInBlock >= PAIRS255THR * 2) {
                        pairsToRemove += pairs255ToRemoveInBlock;
                        ++compressed255Blocks;
                        blockToCompress[i / BLOCK_LEN] = 2;
                    }
                    break;
            }
            offset += BLOCK_LEN;
        }

        this->bitsLen = compressedBlocks * (((BLOCK_LEN / 2) * 2) / 8) + compressed0Blocks * ((BLOCK_LEN / 2) / 8) +
                        compressed255Blocks * ((BLOCK_LEN / 2) / 8) + (bitsTempLen - 2 * pairsToRemove) +
                        sizeof(unsigned long long);

        this->bits = new unsigned char[this->bitsLen + 128];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned long long bits1, bits2, bits;
        bitsCounter = 0;
        rankCounter = 1;
        offset = 0;

        for (unsigned int i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter] = bitsCounter;
            rankCounter += 2;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            if (blockToCompress[i / BLOCK_LEN] == 3) {
                bits1 = 0;
                bits2 = 0;
                bitsCounter += 16;
                for (unsigned int j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255) {
                        bits2 += (1ULL << (63 - (j / 2)));
                    }
                    if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) ||
                        (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255)) {
                        bits1 += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits1 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits1 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits1 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits1 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits1 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits1 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits1 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits1 >> 56);
                this->alignedBits[ranksTemp[rankCounter - 2] + 8] = (bits2 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 9] = ((bits2 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 10] = ((bits2 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 11] = ((bits2 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 12] = ((bits2 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 13] = ((bits2 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 14] = ((bits2 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 15] = (bits2 >> 56);
            } else if (blockToCompress[i / BLOCK_LEN] == 1) {
                bits = 0;
                bitsCounter += 8;
                for (unsigned int j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) {
                        bits += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits >> 56);
            } else if (blockToCompress[i / BLOCK_LEN] == 2) {
                bits = 0;
                bitsCounter += 8;
                for (unsigned int j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255) {
                        bits += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits >> 56);
            } else {
                for (unsigned int j = 0; j < BLOCK_LEN; ++j) this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
            }
            offset += BLOCK_LEN;
        }
        for (unsigned int i = rankCounter; i < ranksTempLen; i += 2) ranksTemp[i] = ranksTemp[rankCounter - 2];

        delete[] bitsTemp;

        this->ranksLen = (ranksTempLen / (2 * SUPERBLOCKLEN)) * INTSINSUPERBLOCK;
        ranksTempLen /= 2;

        this->ranks = new unsigned int[this->ranksLen + 32];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned int bucket[INTSINSUPERBLOCK];
        rankCounter = 0;
        for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) bucket[j] = 0;

        for (unsigned int i = 0; i < ranksTempLen; i += SUPERBLOCKLEN) {
            bucket[0] = ranksTemp[2 * i];
            for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                bucket[j / 2 + 1] += ((ranksTemp[2 * (i + j + 1)] - bucket[0]) << (16 * (j % 2)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK] = ranksTemp[2 * i + 1];
            for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                unsigned int offsetDiff = (ranksTemp[2 * (i + j + 1) + 1] - bucket[OFFSETSTARTINSUPERBLOCK]);
                offsetDiff += (blockToCompress[i + j + 1] << 14);
                bucket[OFFSETSTARTINSUPERBLOCK + 1 + j / 2] += (offsetDiff << (16 * (j % 2)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK + 1] += (blockToCompress[i] << 28);
            for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) {
                this->alignedRanks[rankCounter++] = bucket[j];
                bucket[j] = 0;
            }
        }

        delete[] blockToCompress;
        delete[] emptyBlock;
        delete[] ranksTemp;
    }

    unsigned int getRank_v1(unsigned int i) {
        unsigned int start = i / BLOCK_IN_BITS;
        this->pointer2 = this->alignedRanks + INTSINSUPERBLOCK * (start / SUPERBLOCKLEN);
        unsigned int rank = *(this->pointer2);
        unsigned int rankNumInCL = (start % SUPERBLOCKLEN);
        if (rankNumInCL > 0) {
            if (rankNumInCL % 2 == 1)
                rank += (this->pointer2[(rankNumInCL + 1) / 2] & 0xFFFF);
            else
                rank += (this->pointer2[(rankNumInCL + 1) / 2] >> 16);
        }
        i %= BLOCK_IN_BITS;
        unsigned int rank2;
        if (rankNumInCL == (SUPERBLOCKLEN - 1))
            rank2 = this->pointer2[INTSINSUPERBLOCK];
        else {
            rank2 = *(this->pointer2);
            if (rankNumInCL % 2 == 0)
                rank2 += (this->pointer2[(rankNumInCL + 2) / 2] & 0xFFFF);
            else
                rank2 += (this->pointer2[(rankNumInCL + 2) / 2] >> 16);
        }
        if (((rank2 - rank) & BLOCK_MASK) == 0) return rank + i * ((rank2 - rank) >> BLOCK_MASK_SHIFT);
        bool compressedBlock;
        if (rankNumInCL > 0) {
            unsigned int offset = this->pointer2[OFFSETSTARTINSUPERBLOCK + (rankNumInCL + 1) / 2];
            if (rankNumInCL % 2 == 1) {
                this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + (offset & 0x3FFF);
                compressedBlock = (offset >> 15) & 1;
            } else {
                this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 16) & 0x3FFF);
                compressedBlock = (offset >> 31);
            }
        } else {
            compressedBlock = ((this->pointer2[OFFSETSTARTINSUPERBLOCK + 1] >> 30) & 1);
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK];
        }
        if (compressedBlock) {
            unsigned int temp1 = i % 16;
            unsigned int temp2 = i / 16;
            unsigned long long bits1 = *((unsigned long long*)(this->pointer));
            unsigned long long bits2 = *((unsigned long long*)(this->pointer + 8));
            if ((bits1 & masks2[temp2]) > 0) {
                if ((bits2 & masks2[temp2]) > 0) rank += temp1;
                i -= temp1;
            }
            i -= 16 * __builtin_popcountll(bits1 & masks3[temp2]);
            rank += 16 * __builtin_popcountll(bits2 & masks3[temp2]);
            (this->pointer) += 16;
        }
        switch (i / 64) {
            case 15:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 112)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 120))) & masks[i - 960]);
            case 14:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 112))) & masks[i - 896]);
            case 13:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 104))) & masks[i - 832]);
            case 12:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 96))) & masks[i - 768]);
            case 11:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 88))) & masks[i - 704]);
            case 10:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 80))) & masks[i - 640]);
            case 9:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 72))) & masks[i - 576]);
            case 8:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 64))) & masks[i - 512]);
            case 7:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 56))) & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 48))) & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 40))) & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 32))) & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 24))) & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 16))) & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 8))) & masks[i - 64]);
            default:
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer))) & masks[i]);
        }
    }

    unsigned int getRank_v2(unsigned int i) {
        unsigned int start = i / BLOCK_IN_BITS;
        this->pointer2 = this->alignedRanks + INTSINSUPERBLOCK * (start / SUPERBLOCKLEN);
        unsigned int rank = *(this->pointer2);
        unsigned int rankNumInCL = (start % SUPERBLOCKLEN);
        if (rankNumInCL > 0) {
            if (rankNumInCL % 2 == 1)
                rank += (this->pointer2[(rankNumInCL + 1) / 2] & 0xFFFF);
            else
                rank += (this->pointer2[(rankNumInCL + 1) / 2] >> 16);
        }
        i %= BLOCK_IN_BITS;
        unsigned int rank2;
        if (rankNumInCL == (SUPERBLOCKLEN - 1))
            rank2 = this->pointer2[INTSINSUPERBLOCK];
        else {
            rank2 = *(this->pointer2);
            if (rankNumInCL % 2 == 0)
                rank2 += (this->pointer2[(rankNumInCL + 2) / 2] & 0xFFFF);
            else
                rank2 += (this->pointer2[(rankNumInCL + 2) / 2] >> 16);
        }
        if (((rank2 - rank) & BLOCK_MASK) == 0) return rank + i * ((rank2 - rank) >> BLOCK_MASK_SHIFT);
        unsigned int compressedBlock;
        if (rankNumInCL > 0) {
            unsigned int offset = this->pointer2[OFFSETSTARTINSUPERBLOCK + (rankNumInCL + 1) / 2];
            if (rankNumInCL % 2 == 1) {
                this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + (offset & 0x0FFF);
                compressedBlock = (offset >> 14) & 3;
            } else {
                this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 16) & 0x0FFF);
                compressedBlock = (offset >> 30);
            }
        } else {
            compressedBlock = ((this->pointer2[OFFSETSTARTINSUPERBLOCK + 1] >> 28) & 3);
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK];
        }
        unsigned int temp1, temp2, tempBits;
        unsigned long long bits;
        switch (compressedBlock) {
            case 1:
                temp1 = i % 16;
                temp2 = i / 16;
                bits = *((unsigned long long*)(this->pointer));
                if ((bits & masks2[temp2]) > 0) i -= temp1;
                i -= 16 * __builtin_popcountll(bits & masks3[temp2]);
                (this->pointer) += 8;
                break;
            case 2:
                temp1 = i % 16;
                temp2 = i / 16;
                bits = *((unsigned long long*)(this->pointer));
                if ((bits & masks2[temp2]) > 0) {
                    rank += temp1;
                    i -= temp1;
                }
                tempBits = 16 * __builtin_popcountll(bits & masks3[temp2]);
                i -= tempBits;
                rank += tempBits;
                (this->pointer) += 8;
                break;
            case 3:
                temp1 = i % 16;
                temp2 = i / 16;
                unsigned long long bits1 = *((unsigned long long*)(this->pointer));
                unsigned long long bits2 = *((unsigned long long*)(this->pointer + 8));
                if ((bits1 & masks2[temp2]) > 0) {
                    if ((bits2 & masks2[temp2]) > 0) rank += temp1;
                    i -= temp1;
                }
                i -= 16 * __builtin_popcountll(bits1 & masks3[temp2]);
                rank += 16 * __builtin_popcountll(bits2 & masks3[temp2]);
                (this->pointer) += 16;
                break;
        }
        switch (i / 64) {
            case 15:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 112)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 120))) & masks[i - 960]);
            case 14:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 112))) & masks[i - 896]);
            case 13:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 104))) & masks[i - 832]);
            case 12:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 96))) & masks[i - 768]);
            case 11:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 88))) & masks[i - 704]);
            case 10:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 80))) & masks[i - 640]);
            case 9:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 72))) & masks[i - 576]);
            case 8:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 64))) & masks[i - 512]);
            case 7:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 56))) & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 48))) & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 40))) & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 32))) & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 24))) & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 16))) & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 8))) & masks[i - 64]);
            default:
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer))) & masks[i]);
        }
    }

  public:
    unsigned int* ranks;
    unsigned int* alignedRanks;
    unsigned int ranksLen;
    unsigned char* bits;
    unsigned char* alignedBits;
    unsigned int bitsLen;
    unsigned int textLen;
    unsigned char* pointer;
    unsigned int* pointer2;

    const static unsigned int BLOCK_LEN = sizeof(unsigned long long) * 16;
    const static unsigned int BLOCK_IN_BITS = 8 * BLOCK_LEN;
    const static unsigned int PAIRS0THR = 9;
    const static unsigned int PAIRS255THR = 9;

    const static unsigned int BLOCK_MASK_SHIFT = 10;
    const static unsigned int BLOCK_MASK = (1 << BLOCK_MASK_SHIFT) - 1;

    const static unsigned int SUPERBLOCKLEN = 16;
    const static unsigned int OFFSETSTARTINSUPERBLOCK = 9;
    const static unsigned int INTSINSUPERBLOCK = 18;
    /*
    INTSINSUPERBLOCK = 2 + (SUPERBLOCKLEN - 1) / 2 + (SUPERBLOCKLEN - 1) / 2;
    if ((SUPERBLOCKLEN - 1) % 2 == 1) INTSINSUPERBLOCK += 2;
    *
    OFFSETSTARTINSUPERBLOCK = 1 + (SUPERBLOCKLEN - 1) / 2;
    if ((SUPERBLOCKLEN - 1) % 2 == 1) ++OFFSETSTARTINSUPERBLOCK;
    */

    const unsigned long long masks[64] = {
        0x0000000000000000ULL, 0x0000000000000080ULL, 0x00000000000000C0ULL, 0x00000000000000E0ULL,
        0x00000000000000F0ULL, 0x00000000000000F8ULL, 0x00000000000000FCULL, 0x00000000000000FEULL,
        0x00000000000000FFULL, 0x00000000000080FFULL, 0x000000000000C0FFULL, 0x000000000000E0FFULL,
        0x000000000000F0FFULL, 0x000000000000F8FFULL, 0x000000000000FCFFULL, 0x000000000000FEFFULL,
        0x000000000000FFFFULL, 0x000000000080FFFFULL, 0x0000000000C0FFFFULL, 0x0000000000E0FFFFULL,
        0x0000000000F0FFFFULL, 0x0000000000F8FFFFULL, 0x0000000000FCFFFFULL, 0x0000000000FEFFFFULL,
        0x0000000000FFFFFFULL, 0x0000000080FFFFFFULL, 0x00000000C0FFFFFFULL, 0x00000000E0FFFFFFULL,
        0x00000000F0FFFFFFULL, 0x00000000F8FFFFFFULL, 0x00000000FCFFFFFFULL, 0x00000000FEFFFFFFULL,
        0x00000000FFFFFFFFULL, 0x00000080FFFFFFFFULL, 0x000000C0FFFFFFFFULL, 0x000000E0FFFFFFFFULL,
        0x000000F0FFFFFFFFULL, 0x000000F8FFFFFFFFULL, 0x000000FCFFFFFFFFULL, 0x000000FEFFFFFFFFULL,
        0x000000FFFFFFFFFFULL, 0x000080FFFFFFFFFFULL, 0x0000C0FFFFFFFFFFULL, 0x0000E0FFFFFFFFFFULL,
        0x0000F0FFFFFFFFFFULL, 0x0000F8FFFFFFFFFFULL, 0x0000FCFFFFFFFFFFULL, 0x0000FEFFFFFFFFFFULL,
        0x0000FFFFFFFFFFFFULL, 0x0080FFFFFFFFFFFFULL, 0x00C0FFFFFFFFFFFFULL, 0x00E0FFFFFFFFFFFFULL,
        0x00F0FFFFFFFFFFFFULL, 0x00F8FFFFFFFFFFFFULL, 0x00FCFFFFFFFFFFFFULL, 0x00FEFFFFFFFFFFFFULL,
        0x00FFFFFFFFFFFFFFULL, 0x80FFFFFFFFFFFFFFULL, 0xC0FFFFFFFFFFFFFFULL, 0xE0FFFFFFFFFFFFFFULL,
        0xF0FFFFFFFFFFFFFFULL, 0xF8FFFFFFFFFFFFFFULL, 0xFCFFFFFFFFFFFFFFULL, 0xFEFFFFFFFFFFFFFFULL};

    const unsigned long long masks2[64] = {
        0x8000000000000000ULL, 0x4000000000000000ULL, 0x2000000000000000ULL, 0x1000000000000000ULL,
        0x0800000000000000ULL, 0x0400000000000000ULL, 0x0200000000000000ULL, 0x0100000000000000ULL,
        0x0080000000000000ULL, 0x0040000000000000ULL, 0x0020000000000000ULL, 0x0010000000000000ULL,
        0x0008000000000000ULL, 0x0004000000000000ULL, 0x0002000000000000ULL, 0x0001000000000000ULL,
        0x0000800000000000ULL, 0x0000400000000000ULL, 0x0000200000000000ULL, 0x0000100000000000ULL,
        0x0000080000000000ULL, 0x0000040000000000ULL, 0x0000020000000000ULL, 0x0000010000000000ULL,
        0x0000008000000000ULL, 0x0000004000000000ULL, 0x0000002000000000ULL, 0x0000001000000000ULL,
        0x0000000800000000ULL, 0x0000000400000000ULL, 0x0000000200000000ULL, 0x0000000100000000ULL,
        0x0000000080000000ULL, 0x0000000040000000ULL, 0x0000000020000000ULL, 0x0000000010000000ULL,
        0x0000000008000000ULL, 0x0000000004000000ULL, 0x0000000002000000ULL, 0x0000000001000000ULL,
        0x0000000000800000ULL, 0x0000000000400000ULL, 0x0000000000200000ULL, 0x0000000000100000ULL,
        0x0000000000080000ULL, 0x0000000000040000ULL, 0x0000000000020000ULL, 0x0000000000010000ULL,
        0x0000000000008000ULL, 0x0000000000004000ULL, 0x0000000000002000ULL, 0x0000000000001000ULL,
        0x0000000000000800ULL, 0x0000000000000400ULL, 0x0000000000000200ULL, 0x0000000000000100ULL,
        0x0000000000000080ULL, 0x0000000000000040ULL, 0x0000000000000020ULL, 0x0000000000000010ULL,
        0x0000000000000008ULL, 0x0000000000000004ULL, 0x0000000000000002ULL, 0x0000000000000001ULL};

    const unsigned long long masks3[64] = {
        0x0000000000000000ULL, 0x8000000000000000ULL, 0xC000000000000000ULL, 0xE000000000000000ULL,
        0xF000000000000000ULL, 0xF800000000000000ULL, 0xFC00000000000000ULL, 0xFE00000000000000ULL,
        0xFF00000000000000ULL, 0xFF80000000000000ULL, 0xFFC0000000000000ULL, 0xFFE0000000000000ULL,
        0xFFF0000000000000ULL, 0xFFF8000000000000ULL, 0xFFFC000000000000ULL, 0xFFFE000000000000ULL,
        0xFFFF000000000000ULL, 0xFFFF800000000000ULL, 0xFFFFC00000000000ULL, 0xFFFFE00000000000ULL,
        0xFFFFF00000000000ULL, 0xFFFFF80000000000ULL, 0xFFFFFC0000000000ULL, 0xFFFFFE0000000000ULL,
        0xFFFFFF0000000000ULL, 0xFFFFFF8000000000ULL, 0xFFFFFFC000000000ULL, 0xFFFFFFE000000000ULL,
        0xFFFFFFF000000000ULL, 0xFFFFFFF800000000ULL, 0xFFFFFFFC00000000ULL, 0xFFFFFFFE00000000ULL,
        0xFFFFFFFF00000000ULL, 0xFFFFFFFF80000000ULL, 0xFFFFFFFFC0000000ULL, 0xFFFFFFFFE0000000ULL,
        0xFFFFFFFFF0000000ULL, 0xFFFFFFFFF8000000ULL, 0xFFFFFFFFFC000000ULL, 0xFFFFFFFFFE000000ULL,
        0xFFFFFFFFFF000000ULL, 0xFFFFFFFFFF800000ULL, 0xFFFFFFFFFFC00000ULL, 0xFFFFFFFFFFE00000ULL,
        0xFFFFFFFFFFF00000ULL, 0xFFFFFFFFFFF80000ULL, 0xFFFFFFFFFFFC0000ULL, 0xFFFFFFFFFFFE0000ULL,
        0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFFFF8000ULL, 0xFFFFFFFFFFFFC000ULL, 0xFFFFFFFFFFFFE000ULL,
        0xFFFFFFFFFFFFF000ULL, 0xFFFFFFFFFFFFF800ULL, 0xFFFFFFFFFFFFFC00ULL, 0xFFFFFFFFFFFFFE00ULL,
        0xFFFFFFFFFFFFFF00ULL, 0xFFFFFFFFFFFFFF80ULL, 0xFFFFFFFFFFFFFFC0ULL, 0xFFFFFFFFFFFFFFE0ULL,
        0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFF8ULL, 0xFFFFFFFFFFFFFFFCULL, 0xFFFFFFFFFFFFFFFEULL};

    RankMPE32() {
        this->initialize();
    }

    ~RankMPE32() {
        this->freeMemory();
    }

    void build(unsigned char* text, unsigned int textLen) {
        switch (T) {
            case RankMPEType::RANK_MPE1:
                this->build_v1(text, textLen);
                break;
            default:
                this->build_v2(text, textLen);
                break;
        }
    }

    unsigned int rank(unsigned int i) {
        switch (T) {
            case RankMPEType::RANK_MPE1:
                return this->getRank_v1(i);
                break;
            default:
                return this->getRank_v2(i);
                break;
        }
    }

    void save(FILE* outFile) {
        fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        fwrite(&this->ranksLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        if (this->ranksLen > 0)
            fwrite(this->alignedRanks, (size_t)sizeof(unsigned int), (size_t)this->ranksLen, outFile);
        fwrite(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
        if (this->bitsLen > 0) fwrite(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, outFile);
    }

    void load(FILE* inFile) {
        this->free();
        size_t result;
        result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->ranksLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->ranksLen > 0) {
            this->ranks = new unsigned int[this->ranksLen + 32];
            this->alignedRanks = this->ranks;
            while ((unsigned long long)(this->alignedRanks) % 128) ++(this->alignedRanks);
            result = fread(this->alignedRanks, (size_t)sizeof(unsigned int), (size_t)this->ranksLen, inFile);
            if (result != this->ranksLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
        result = fread(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->bitsLen > 0) {
            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
            result = fread(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, inFile);
            if (result != this->bitsLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
    }

    void free() {
        this->freeMemory();
        this->initialize();
    }

    unsigned long long getSize() {
        unsigned long long size = sizeof(this->ranksLen) + sizeof(this->bitsLen) + sizeof(this->textLen) +
                                  sizeof(this->pointer) + sizeof(this->pointer2);
        if (this->ranksLen > 0) size += (this->ranksLen + 32) * sizeof(unsigned int);
        if (this->bitsLen > 0) size += (this->bitsLen + 128) * sizeof(unsigned char);
        return size;
    }

    unsigned int getTextSize() {
        return this->textLen * sizeof(unsigned char);
    }

    void testRank(unsigned char* text, unsigned int textLen) {
        unsigned int rankTest = 0;

        for (unsigned int i = 0; i < textLen; ++i) {
            unsigned int bits = (unsigned int)text[i];
            for (unsigned int j = 0; j < 8; ++j) {
                if (((bits >> (7 - j)) & 1) == 1) ++rankTest;
                if (rankTest != this->rank(8 * i + j + 1)) {
                    cout << (8 * i + j + 1) << " " << rankTest << " " << this->rank(8 * i + j + 1);
                    cin.ignore();
                }
            }
        }
    }
};

template <RankMPEType T>
class RankMPE64 {
  private:
    void freeMemory() {
        if (this->ranks != NULL) delete[] this->ranks;
        if (this->bits != NULL) delete[] this->bits;
    }

    void initialize() {
        this->ranks = NULL;
        this->alignedRanks = NULL;
        this->ranksLen = 0;
        this->bits = NULL;
        this->alignedBits = NULL;
        this->bitsLen = 0;
        this->textLen = 0;
        this->pointer = NULL;
        this->pointer2 = NULL;
    }

    void build_v1(unsigned char* text, unsigned long long textLen) {
        this->free();
        this->textLen = textLen;
        unsigned long long extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned long long notEmptyBlocks = 0;
        for (unsigned long long i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        unsigned long long bitsTempLen = notEmptyBlocks * BLOCK_LEN;
        unsigned char* bitsTemp = new unsigned char[bitsTempLen];

        unsigned long long ranksTempLen = 2 * (extendedTextLen / BLOCK_LEN) + 2;
        if (ranksTempLen % (2 * SUPERBLOCKLEN) > 0)
            ranksTempLen += ((2 * SUPERBLOCKLEN) - ranksTempLen % (2 * SUPERBLOCKLEN));
        unsigned long long* ranksTemp = new unsigned long long[ranksTempLen];

        unsigned long long rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter++] = rank;
            ranksTemp[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (i + j < this->textLen) {
                    bitsTemp[bitsCounter++] = (unsigned int)text[i + j];
                    rank += __builtin_popcount((unsigned int)text[i + j]);
                } else {
                    bitsTemp[bitsCounter++] = 0;
                }
            }
        }
        ranksTemp[rankCounter++] = rank;
        ranksTemp[rankCounter++] = 0;

        for (unsigned long long i = rankCounter; i < ranksTempLen; ++i) {
            ranksTemp[i] = 0;
        }

        unsigned long long pairsToRemove = 0;
        unsigned long long pairsToRemoveInBlock = 0;
        unsigned long long compressedBlocks = 0;
        unsigned long long offset = 0;
        bool* blockToCompress = new bool[extendedTextLen / BLOCK_LEN];
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            blockToCompress[i / BLOCK_LEN] = false;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            pairsToRemoveInBlock = 0;
            for (unsigned long long j = 0; j < BLOCK_LEN; j += 2) {
                if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) ||
                    (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255))
                    ++pairsToRemoveInBlock;
            }
            if (pairsToRemoveInBlock > 8) {
                pairsToRemove += pairsToRemoveInBlock;
                ++compressedBlocks;
                blockToCompress[i / BLOCK_LEN] = true;
            }
            offset += BLOCK_LEN;
        }

        this->bitsLen = compressedBlocks * (((BLOCK_LEN / 2) * 2) / 8) + (bitsTempLen - 2 * pairsToRemove) +
                        sizeof(unsigned long long);

        this->bits = new unsigned char[this->bitsLen + 128];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned long long bits1, bits2;
        bitsCounter = 0;
        rankCounter = 1;
        offset = 0;

        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter] = bitsCounter;
            rankCounter += 2;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            if (blockToCompress[i / BLOCK_LEN]) {
                bits1 = 0;
                bits2 = 0;
                bitsCounter += 16;
                for (unsigned long long j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255) {
                        bits2 += (1ULL << (63 - (j / 2)));
                    }
                    if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) ||
                        (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255)) {
                        bits1 += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits1 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits1 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits1 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits1 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits1 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits1 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits1 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits1 >> 56);
                this->alignedBits[ranksTemp[rankCounter - 2] + 8] = (bits2 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 9] = ((bits2 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 10] = ((bits2 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 11] = ((bits2 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 12] = ((bits2 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 13] = ((bits2 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 14] = ((bits2 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 15] = (bits2 >> 56);
            } else {
                for (unsigned long long j = 0; j < BLOCK_LEN; ++j)
                    this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
            }
            offset += BLOCK_LEN;
        }
        for (unsigned long long i = rankCounter; i < ranksTempLen; i += 2) ranksTemp[i] = ranksTemp[rankCounter - 2];

        delete[] bitsTemp;

        this->ranksLen = (ranksTempLen / (2 * SUPERBLOCKLEN)) * LONGSINSUPERBLOCK;
        ranksTempLen /= 2;

        this->ranks = new unsigned long long[this->ranksLen + 16];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned long long bucket[LONGSINSUPERBLOCK];
        rankCounter = 0;
        for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) bucket[j] = 0;

        for (unsigned long long i = 0; i < ranksTempLen; i += SUPERBLOCKLEN) {
            bucket[0] = ranksTemp[2 * i];
            for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                bucket[j / 4 + 1] += ((ranksTemp[2 * (i + j + 1)] - bucket[0]) << (16 * (j % 4)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK] = ranksTemp[2 * i + 1];
            for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                unsigned long long offsetDiff = (ranksTemp[2 * (i + j + 1) + 1] - bucket[OFFSETSTARTINSUPERBLOCK]);
                if (blockToCompress[i + j + 1]) offsetDiff += (1 << 15);
                bucket[OFFSETSTARTINSUPERBLOCK + 1 + j / 4] += (offsetDiff << (16 * (j % 4)));
            }
            if (blockToCompress[i]) bucket[OFFSETSTARTINSUPERBLOCK + 1] += (1ULL << 62);
            for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) {
                this->alignedRanks[rankCounter++] = bucket[j];
                bucket[j] = 0;
            }
        }

        delete[] blockToCompress;
        delete[] emptyBlock;
        delete[] ranksTemp;
    }

    void build_v2(unsigned char* text, unsigned long long textLen) {
        this->free();
        this->textLen = textLen;
        unsigned long long extendedTextLen = this->textLen;
        if (this->textLen % BLOCK_LEN > 0) extendedTextLen += (BLOCK_LEN - this->textLen % BLOCK_LEN);

        bool* emptyBlock = new bool[extendedTextLen / BLOCK_LEN];
        bool homoblock;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            homoblock = true;
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (i + j >= this->textLen || (unsigned int)text[i + j] > 0) {
                    homoblock = false;
                    break;
                }
            }
            if (!homoblock) {
                homoblock = true;
                for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                    if (i + j >= this->textLen || (unsigned int)text[i + j] < 255) {
                        homoblock = false;
                        break;
                    }
                }
            }
            emptyBlock[i / BLOCK_LEN] = homoblock;
        }

        unsigned long long notEmptyBlocks = 0;
        for (unsigned long long i = 0; i < extendedTextLen / BLOCK_LEN; ++i)
            if (!emptyBlock[i]) ++notEmptyBlocks;

        unsigned long long bitsTempLen = notEmptyBlocks * BLOCK_LEN;
        unsigned char* bitsTemp = new unsigned char[bitsTempLen];

        unsigned long long ranksTempLen = 2 * (extendedTextLen / BLOCK_LEN) + 2;
        if (ranksTempLen % (2 * SUPERBLOCKLEN) > 0)
            ranksTempLen += ((2 * SUPERBLOCKLEN) - ranksTempLen % (2 * SUPERBLOCKLEN));
        unsigned long long* ranksTemp = new unsigned long long[ranksTempLen];

        unsigned long long rank = 0, rankCounter = 0, bitsCounter = 0;
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter++] = rank;
            ranksTemp[rankCounter++] = bitsCounter;
            if (emptyBlock[i / BLOCK_LEN]) {
                if ((unsigned int)text[i] == 255) rank += BLOCK_IN_BITS;
                continue;
            }
            for (unsigned long long j = 0; j < BLOCK_LEN; ++j) {
                if (i + j < this->textLen) {
                    bitsTemp[bitsCounter++] = (unsigned int)text[i + j];
                    rank += __builtin_popcount((unsigned int)text[i + j]);
                } else {
                    bitsTemp[bitsCounter++] = 0;
                }
            }
        }
        ranksTemp[rankCounter++] = rank;
        ranksTemp[rankCounter++] = 0;

        for (unsigned long long i = rankCounter; i < ranksTempLen; ++i) {
            ranksTemp[i] = 0;
        }

        unsigned long long pairsToRemove = 0;
        unsigned long long pairs0ToRemoveInBlock = 0;
        unsigned long long pairs255ToRemoveInBlock = 0;
        unsigned long long compressedBlocks = 0;
        unsigned long long compressed0Blocks = 0;
        unsigned long long compressed255Blocks = 0;
        unsigned long long offset = 0;
        unsigned char* blockToCompress = new unsigned char[extendedTextLen / BLOCK_LEN];
        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            blockToCompress[i / BLOCK_LEN] = 0;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            pairs0ToRemoveInBlock = 0;
            pairs255ToRemoveInBlock = 0;
            for (unsigned long long j = 0; j < BLOCK_LEN; j += 2) {
                if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0)) ++pairs0ToRemoveInBlock;
                if ((bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255)) ++pairs255ToRemoveInBlock;
            }
            switch (T) {
                case RankMPEType::RANK_MPE2:
                    if (pairs0ToRemoveInBlock > 4 && pairs255ToRemoveInBlock > 4) {
                        pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                        ++compressedBlocks;
                        blockToCompress[i / BLOCK_LEN] = 3;
                    } else if (pairs0ToRemoveInBlock > 4 && pairs255ToRemoveInBlock <= 4) {
                        pairsToRemove += pairs0ToRemoveInBlock;
                        ++compressed0Blocks;
                        blockToCompress[i / BLOCK_LEN] = 1;
                    } else if (pairs0ToRemoveInBlock <= 4 && pairs255ToRemoveInBlock > 4) {
                        pairsToRemove += pairs255ToRemoveInBlock;
                        ++compressed255Blocks;
                        blockToCompress[i / BLOCK_LEN] = 2;
                    }
                    break;
                default:
                    if (pairs0ToRemoveInBlock > PAIRS0THR && pairs255ToRemoveInBlock > PAIRS255THR) {
                        pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                        ++compressedBlocks;
                        blockToCompress[i / BLOCK_LEN] = 3;
                    } else if (pairs0ToRemoveInBlock >= PAIRS0THR * 2 && pairs255ToRemoveInBlock <= PAIRS255THR) {
                        pairsToRemove += pairs0ToRemoveInBlock;
                        ++compressed0Blocks;
                        blockToCompress[i / BLOCK_LEN] = 1;
                    } else if (pairs0ToRemoveInBlock <= PAIRS0THR && pairs255ToRemoveInBlock >= PAIRS255THR * 2) {
                        pairsToRemove += pairs255ToRemoveInBlock;
                        ++compressed255Blocks;
                        blockToCompress[i / BLOCK_LEN] = 2;
                    }
                    break;
            }
            offset += BLOCK_LEN;
        }

        this->bitsLen = compressedBlocks * (((BLOCK_LEN / 2) * 2) / 8) + compressed0Blocks * ((BLOCK_LEN / 2) / 8) +
                        compressed255Blocks * ((BLOCK_LEN / 2) / 8) + (bitsTempLen - 2 * pairsToRemove) +
                        sizeof(unsigned long long);

        this->bits = new unsigned char[this->bitsLen + 128];
        this->alignedBits = this->bits;
        while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

        unsigned long long bits1, bits2, bits;
        bitsCounter = 0;
        rankCounter = 1;
        offset = 0;

        for (unsigned long long i = 0; i < extendedTextLen; i += BLOCK_LEN) {
            ranksTemp[rankCounter] = bitsCounter;
            rankCounter += 2;
            if (emptyBlock[i / BLOCK_LEN]) continue;
            if (blockToCompress[i / BLOCK_LEN] == 3) {
                bits1 = 0;
                bits2 = 0;
                bitsCounter += 16;
                for (unsigned long long j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255) {
                        bits2 += (1ULL << (63 - (j / 2)));
                    }
                    if ((bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) ||
                        (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255)) {
                        bits1 += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits1 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits1 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits1 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits1 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits1 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits1 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits1 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits1 >> 56);
                this->alignedBits[ranksTemp[rankCounter - 2] + 8] = (bits2 & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 9] = ((bits2 >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 10] = ((bits2 >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 11] = ((bits2 >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 12] = ((bits2 >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 13] = ((bits2 >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 14] = ((bits2 >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 15] = (bits2 >> 56);
            } else if (blockToCompress[i / BLOCK_LEN] == 1) {
                bits = 0;
                bitsCounter += 8;
                for (unsigned long long j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 0 && bitsTemp[offset + j + 1] == 0) {
                        bits += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits >> 56);
            } else if (blockToCompress[i / BLOCK_LEN] == 2) {
                bits = 0;
                bitsCounter += 8;
                for (unsigned long long j = 0; j < BLOCK_LEN; j += 2) {
                    if (bitsTemp[offset + j] == 255 && bitsTemp[offset + j + 1] == 255) {
                        bits += (1ULL << (63 - (j / 2)));
                    } else {
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
                        this->alignedBits[bitsCounter++] = bitsTemp[offset + j + 1];
                    }
                }
                this->alignedBits[ranksTemp[rankCounter - 2]] = (bits & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 1] = ((bits >> 8) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 2] = ((bits >> 16) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 3] = ((bits >> 24) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 4] = ((bits >> 32) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 5] = ((bits >> 40) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 6] = ((bits >> 48) & 0xFF);
                this->alignedBits[ranksTemp[rankCounter - 2] + 7] = (bits >> 56);
            } else {
                for (unsigned long long j = 0; j < BLOCK_LEN; ++j)
                    this->alignedBits[bitsCounter++] = bitsTemp[offset + j];
            }
            offset += BLOCK_LEN;
        }
        for (unsigned long long i = rankCounter; i < ranksTempLen; i += 2) ranksTemp[i] = ranksTemp[rankCounter - 2];

        delete[] bitsTemp;

        this->ranksLen = (ranksTempLen / (2 * SUPERBLOCKLEN)) * LONGSINSUPERBLOCK;
        ranksTempLen /= 2;

        this->ranks = new unsigned long long[this->ranksLen + 16];
        this->alignedRanks = this->ranks;
        while ((unsigned long long)this->alignedRanks % 128) ++this->alignedRanks;

        unsigned long long bucket[LONGSINSUPERBLOCK];
        rankCounter = 0;
        for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) bucket[j] = 0;

        for (unsigned long long i = 0; i < ranksTempLen; i += SUPERBLOCKLEN) {
            bucket[0] = ranksTemp[2 * i];
            for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                bucket[j / 4 + 1] += ((ranksTemp[2 * (i + j + 1)] - bucket[0]) << (16 * (j % 4)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK] = ranksTemp[2 * i + 1];
            for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                unsigned long long offsetDiff = (ranksTemp[2 * (i + j + 1) + 1] - bucket[OFFSETSTARTINSUPERBLOCK]);
                offsetDiff += (blockToCompress[i + j + 1] << 14);
                bucket[OFFSETSTARTINSUPERBLOCK + 1 + j / 4] += (offsetDiff << (16 * (j % 4)));
            }
            bucket[OFFSETSTARTINSUPERBLOCK + 1] += ((unsigned long long)blockToCompress[i] << 60);
            for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) {
                this->alignedRanks[rankCounter++] = bucket[j];
                bucket[j] = 0;
            }
        }

        delete[] blockToCompress;
        delete[] emptyBlock;
        delete[] ranksTemp;
    }

    unsigned long long getRank_v1(unsigned long long i) {
        unsigned long long start = i / BLOCK_IN_BITS;
        this->pointer2 = this->alignedRanks + LONGSINSUPERBLOCK * (start / SUPERBLOCKLEN);
        unsigned long long rank = *(this->pointer2);
        unsigned long long rankNumInCL = (start % SUPERBLOCKLEN);
        if (rankNumInCL > 0) {
            switch (rankNumInCL % 4) {
                case 3:
                    rank += ((this->pointer2[(rankNumInCL + 3) / 4] >> 32) & 0xFFFF);
                    break;
                case 2:
                    rank += ((this->pointer2[(rankNumInCL + 3) / 4] >> 16) & 0xFFFF);
                    break;
                case 1:
                    rank += (this->pointer2[(rankNumInCL + 3) / 4] & 0xFFFF);
                    break;
                default:
                    rank += (this->pointer2[(rankNumInCL + 3) / 4] >> 48);
                    break;
            }
        }
        i %= BLOCK_IN_BITS;
        unsigned long long rank2;
        if (rankNumInCL == (SUPERBLOCKLEN - 1))
            rank2 = this->pointer2[LONGSINSUPERBLOCK];
        else {
            rank2 = *(this->pointer2);
            switch (rankNumInCL % 4) {
                case 3:
                    rank2 += (this->pointer2[(rankNumInCL + 3) / 4] >> 48);
                    break;
                case 2:
                    rank2 += ((this->pointer2[(rankNumInCL + 3) / 4] >> 32) & 0xFFFF);
                    break;
                case 1:
                    rank2 += ((this->pointer2[(rankNumInCL + 3) / 4] >> 16) & 0xFFFF);
                    break;
                default:
                    rank2 += (this->pointer2[(rankNumInCL + 4) / 4] & 0xFFFF);
                    break;
            }
        }
        if (((rank2 - rank) & BLOCK_MASK) == 0) return rank + i * ((rank2 - rank) >> BLOCK_MASK_SHIFT);
        bool compressedBlock;
        if (rankNumInCL > 0) {
            unsigned long long offset = this->pointer2[OFFSETSTARTINSUPERBLOCK + (rankNumInCL + 3) / 4];
            switch (rankNumInCL % 4) {
                case 3:
                    this->pointer =
                        this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 32) & 0x3FFF);
                    compressedBlock = (offset >> 47) & 1;
                    break;
                case 2:
                    this->pointer =
                        this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 16) & 0x3FFF);
                    compressedBlock = (offset >> 31) & 1;
                    break;
                case 1:
                    this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + (offset & 0x3FFF);
                    compressedBlock = (offset >> 15) & 1;
                    break;
                default:
                    this->pointer =
                        this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 48) & 0x3FFF);
                    compressedBlock = (offset >> 63);
                    break;
            }
        } else {
            compressedBlock = ((this->pointer2[OFFSETSTARTINSUPERBLOCK + 1] >> 62) & 1);
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK];
        }
        if (compressedBlock) {
            unsigned int temp1 = i % 16;
            unsigned int temp2 = i / 16;
            unsigned long long bits1 = *((unsigned long long*)(this->pointer));
            unsigned long long bits2 = *((unsigned long long*)(this->pointer + 8));
            if ((bits1 & masks2[temp2]) > 0) {
                if ((bits2 & masks2[temp2]) > 0) rank += temp1;
                i -= temp1;
            }
            i -= 16 * __builtin_popcountll(bits1 & masks3[temp2]);
            rank += 16 * __builtin_popcountll(bits2 & masks3[temp2]);
            (this->pointer) += 16;
        }
        switch (i / 64) {
            case 15:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 112)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 120))) & masks[i - 960]);
            case 14:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 112))) & masks[i - 896]);
            case 13:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 104))) & masks[i - 832]);
            case 12:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 96))) & masks[i - 768]);
            case 11:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 88))) & masks[i - 704]);
            case 10:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 80))) & masks[i - 640]);
            case 9:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 72))) & masks[i - 576]);
            case 8:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 64))) & masks[i - 512]);
            case 7:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 56))) & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 48))) & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 40))) & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 32))) & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 24))) & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 16))) & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 8))) & masks[i - 64]);
            default:
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer))) & masks[i]);
        }
    }

    unsigned long long getRank_v2(unsigned long long i) {
        unsigned long long start = i / BLOCK_IN_BITS;
        this->pointer2 = this->alignedRanks + LONGSINSUPERBLOCK * (start / SUPERBLOCKLEN);
        unsigned long long rank = *(this->pointer2);
        unsigned long long rankNumInCL = (start % SUPERBLOCKLEN);
        if (rankNumInCL > 0) {
            switch (rankNumInCL % 4) {
                case 3:
                    rank += ((this->pointer2[(rankNumInCL + 3) / 4] >> 32) & 0xFFFF);
                    break;
                case 2:
                    rank += ((this->pointer2[(rankNumInCL + 3) / 4] >> 16) & 0xFFFF);
                    break;
                case 1:
                    rank += (this->pointer2[(rankNumInCL + 3) / 4] & 0xFFFF);
                    break;
                default:
                    rank += (this->pointer2[(rankNumInCL + 3) / 4] >> 48);
                    break;
            }
        }
        i %= BLOCK_IN_BITS;
        unsigned long long rank2;
        if (rankNumInCL == (SUPERBLOCKLEN - 1))
            rank2 = this->pointer2[LONGSINSUPERBLOCK];
        else {
            rank2 = *(this->pointer2);
            switch (rankNumInCL % 4) {
                case 3:
                    rank2 += (this->pointer2[(rankNumInCL + 3) / 4] >> 48);
                    break;
                case 2:
                    rank2 += ((this->pointer2[(rankNumInCL + 3) / 4] >> 32) & 0xFFFF);
                    break;
                case 1:
                    rank2 += ((this->pointer2[(rankNumInCL + 3) / 4] >> 16) & 0xFFFF);
                    break;
                default:
                    rank2 += (this->pointer2[(rankNumInCL + 4) / 4] & 0xFFFF);
                    break;
            }
        }
        if (((rank2 - rank) & BLOCK_MASK) == 0) return rank + i * ((rank2 - rank) >> BLOCK_MASK_SHIFT);
        unsigned int compressedBlock;
        if (rankNumInCL > 0) {
            unsigned long long offset = this->pointer2[OFFSETSTARTINSUPERBLOCK + (rankNumInCL + 3) / 4];
            switch (rankNumInCL % 4) {
                case 3:
                    this->pointer =
                        this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 32) & 0x0FFF);
                    compressedBlock = (offset >> 46) & 3;
                    break;
                case 2:
                    this->pointer =
                        this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 16) & 0x0FFF);
                    compressedBlock = (offset >> 30) & 3;
                    break;
                case 1:
                    this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + (offset & 0x0FFF);
                    compressedBlock = (offset >> 14) & 3;
                    break;
                default:
                    this->pointer =
                        this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK] + ((offset >> 48) & 0x0FFF);
                    compressedBlock = (offset >> 62);
                    break;
            }
        } else {
            compressedBlock = ((this->pointer2[OFFSETSTARTINSUPERBLOCK + 1] >> 60) & 3);
            this->pointer = this->alignedBits + this->pointer2[OFFSETSTARTINSUPERBLOCK];
        }
        unsigned int temp1, temp2, tempBits;
        unsigned long long bits;
        switch (compressedBlock) {
            case 1:
                temp1 = i % 16;
                temp2 = i / 16;
                bits = *((unsigned long long*)(this->pointer));
                if ((bits & masks2[temp2]) > 0) i -= temp1;
                i -= 16 * __builtin_popcountll(bits & masks3[temp2]);
                (this->pointer) += 8;
                break;
            case 2:
                temp1 = i % 16;
                temp2 = i / 16;
                bits = *((unsigned long long*)(this->pointer));
                if ((bits & masks2[temp2]) > 0) {
                    rank += temp1;
                    i -= temp1;
                }
                tempBits = 16 * __builtin_popcountll(bits & masks3[temp2]);
                i -= tempBits;
                rank += tempBits;
                (this->pointer) += 8;
                break;
            case 3:
                temp1 = i % 16;
                temp2 = i / 16;
                unsigned long long bits1 = *((unsigned long long*)(this->pointer));
                unsigned long long bits2 = *((unsigned long long*)(this->pointer + 8));
                if ((bits1 & masks2[temp2]) > 0) {
                    if ((bits2 & masks2[temp2]) > 0) rank += temp1;
                    i -= temp1;
                }
                i -= 16 * __builtin_popcountll(bits1 & masks3[temp2]);
                rank += 16 * __builtin_popcountll(bits2 & masks3[temp2]);
                (this->pointer) += 16;
                break;
        }
        switch (i / 64) {
            case 15:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 112)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 120))) & masks[i - 960]);
            case 14:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 104)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 112))) & masks[i - 896]);
            case 13:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 96)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 104))) & masks[i - 832]);
            case 12:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 88)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 96))) & masks[i - 768]);
            case 11:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 80)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 88))) & masks[i - 704]);
            case 10:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 72)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 80))) & masks[i - 640]);
            case 9:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 64)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 72))) & masks[i - 576]);
            case 8:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 56)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 64))) & masks[i - 512]);
            case 7:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 48)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 56))) & masks[i - 448]);
            case 6:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 40)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 48))) & masks[i - 384]);
            case 5:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 32)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 40))) & masks[i - 320]);
            case 4:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 24)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 32))) & masks[i - 256]);
            case 3:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 16)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 24))) & masks[i - 192]);
            case 2:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer + 8)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 16))) & masks[i - 128]);
            case 1:
                rank += __builtin_popcountll(*((unsigned long long*)(this->pointer)));
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer + 8))) & masks[i - 64]);
            default:
                return rank + __builtin_popcountll((*((unsigned long long*)(this->pointer))) & masks[i]);
        }
    }

  public:
    unsigned long long* ranks;
    unsigned long long* alignedRanks;
    unsigned long long ranksLen;
    unsigned char* bits;
    unsigned char* alignedBits;
    unsigned long long bitsLen;
    unsigned long long textLen;
    unsigned char* pointer;
    unsigned long long* pointer2;

    const static unsigned long long BLOCK_LEN = sizeof(unsigned long long) * 16;
    const static unsigned long long BLOCK_IN_BITS = 8 * BLOCK_LEN;
    const static unsigned long long PAIRS0THR = 9;
    const static unsigned long long PAIRS255THR = 9;

    const static unsigned int BLOCK_MASK_SHIFT = 10;
    const static unsigned long long BLOCK_MASK = (1 << BLOCK_MASK_SHIFT) - 1;

    const static unsigned long long SUPERBLOCKLEN = 16;
    const static unsigned long long OFFSETSTARTINSUPERBLOCK = 5;
    const static unsigned long long LONGSINSUPERBLOCK = 10;
    /*
    LONGSINSUPERBLOCK = 2 + (SUPERBLOCKLEN - 1) / 4 + (SUPERBLOCKLEN - 1) / 4;
    if ((SUPERBLOCKLEN - 1) % 4 > 0) LONGSINSUPERBLOCK += 2;
    *
    OFFSETSTARTINSUPERBLOCK = 1 + (SUPERBLOCKLEN - 1) / 2;
    if ((SUPERBLOCKLEN - 1) % 4 > 0) ++OFFSETSTARTINSUPERBLOCK;
    */

    const unsigned long long masks[64] = {
        0x0000000000000000ULL, 0x0000000000000080ULL, 0x00000000000000C0ULL, 0x00000000000000E0ULL,
        0x00000000000000F0ULL, 0x00000000000000F8ULL, 0x00000000000000FCULL, 0x00000000000000FEULL,
        0x00000000000000FFULL, 0x00000000000080FFULL, 0x000000000000C0FFULL, 0x000000000000E0FFULL,
        0x000000000000F0FFULL, 0x000000000000F8FFULL, 0x000000000000FCFFULL, 0x000000000000FEFFULL,
        0x000000000000FFFFULL, 0x000000000080FFFFULL, 0x0000000000C0FFFFULL, 0x0000000000E0FFFFULL,
        0x0000000000F0FFFFULL, 0x0000000000F8FFFFULL, 0x0000000000FCFFFFULL, 0x0000000000FEFFFFULL,
        0x0000000000FFFFFFULL, 0x0000000080FFFFFFULL, 0x00000000C0FFFFFFULL, 0x00000000E0FFFFFFULL,
        0x00000000F0FFFFFFULL, 0x00000000F8FFFFFFULL, 0x00000000FCFFFFFFULL, 0x00000000FEFFFFFFULL,
        0x00000000FFFFFFFFULL, 0x00000080FFFFFFFFULL, 0x000000C0FFFFFFFFULL, 0x000000E0FFFFFFFFULL,
        0x000000F0FFFFFFFFULL, 0x000000F8FFFFFFFFULL, 0x000000FCFFFFFFFFULL, 0x000000FEFFFFFFFFULL,
        0x000000FFFFFFFFFFULL, 0x000080FFFFFFFFFFULL, 0x0000C0FFFFFFFFFFULL, 0x0000E0FFFFFFFFFFULL,
        0x0000F0FFFFFFFFFFULL, 0x0000F8FFFFFFFFFFULL, 0x0000FCFFFFFFFFFFULL, 0x0000FEFFFFFFFFFFULL,
        0x0000FFFFFFFFFFFFULL, 0x0080FFFFFFFFFFFFULL, 0x00C0FFFFFFFFFFFFULL, 0x00E0FFFFFFFFFFFFULL,
        0x00F0FFFFFFFFFFFFULL, 0x00F8FFFFFFFFFFFFULL, 0x00FCFFFFFFFFFFFFULL, 0x00FEFFFFFFFFFFFFULL,
        0x00FFFFFFFFFFFFFFULL, 0x80FFFFFFFFFFFFFFULL, 0xC0FFFFFFFFFFFFFFULL, 0xE0FFFFFFFFFFFFFFULL,
        0xF0FFFFFFFFFFFFFFULL, 0xF8FFFFFFFFFFFFFFULL, 0xFCFFFFFFFFFFFFFFULL, 0xFEFFFFFFFFFFFFFFULL};

    const unsigned long long masks2[64] = {
        0x8000000000000000ULL, 0x4000000000000000ULL, 0x2000000000000000ULL, 0x1000000000000000ULL,
        0x0800000000000000ULL, 0x0400000000000000ULL, 0x0200000000000000ULL, 0x0100000000000000ULL,
        0x0080000000000000ULL, 0x0040000000000000ULL, 0x0020000000000000ULL, 0x0010000000000000ULL,
        0x0008000000000000ULL, 0x0004000000000000ULL, 0x0002000000000000ULL, 0x0001000000000000ULL,
        0x0000800000000000ULL, 0x0000400000000000ULL, 0x0000200000000000ULL, 0x0000100000000000ULL,
        0x0000080000000000ULL, 0x0000040000000000ULL, 0x0000020000000000ULL, 0x0000010000000000ULL,
        0x0000008000000000ULL, 0x0000004000000000ULL, 0x0000002000000000ULL, 0x0000001000000000ULL,
        0x0000000800000000ULL, 0x0000000400000000ULL, 0x0000000200000000ULL, 0x0000000100000000ULL,
        0x0000000080000000ULL, 0x0000000040000000ULL, 0x0000000020000000ULL, 0x0000000010000000ULL,
        0x0000000008000000ULL, 0x0000000004000000ULL, 0x0000000002000000ULL, 0x0000000001000000ULL,
        0x0000000000800000ULL, 0x0000000000400000ULL, 0x0000000000200000ULL, 0x0000000000100000ULL,
        0x0000000000080000ULL, 0x0000000000040000ULL, 0x0000000000020000ULL, 0x0000000000010000ULL,
        0x0000000000008000ULL, 0x0000000000004000ULL, 0x0000000000002000ULL, 0x0000000000001000ULL,
        0x0000000000000800ULL, 0x0000000000000400ULL, 0x0000000000000200ULL, 0x0000000000000100ULL,
        0x0000000000000080ULL, 0x0000000000000040ULL, 0x0000000000000020ULL, 0x0000000000000010ULL,
        0x0000000000000008ULL, 0x0000000000000004ULL, 0x0000000000000002ULL, 0x0000000000000001ULL};

    const unsigned long long masks3[64] = {
        0x0000000000000000ULL, 0x8000000000000000ULL, 0xC000000000000000ULL, 0xE000000000000000ULL,
        0xF000000000000000ULL, 0xF800000000000000ULL, 0xFC00000000000000ULL, 0xFE00000000000000ULL,
        0xFF00000000000000ULL, 0xFF80000000000000ULL, 0xFFC0000000000000ULL, 0xFFE0000000000000ULL,
        0xFFF0000000000000ULL, 0xFFF8000000000000ULL, 0xFFFC000000000000ULL, 0xFFFE000000000000ULL,
        0xFFFF000000000000ULL, 0xFFFF800000000000ULL, 0xFFFFC00000000000ULL, 0xFFFFE00000000000ULL,
        0xFFFFF00000000000ULL, 0xFFFFF80000000000ULL, 0xFFFFFC0000000000ULL, 0xFFFFFE0000000000ULL,
        0xFFFFFF0000000000ULL, 0xFFFFFF8000000000ULL, 0xFFFFFFC000000000ULL, 0xFFFFFFE000000000ULL,
        0xFFFFFFF000000000ULL, 0xFFFFFFF800000000ULL, 0xFFFFFFFC00000000ULL, 0xFFFFFFFE00000000ULL,
        0xFFFFFFFF00000000ULL, 0xFFFFFFFF80000000ULL, 0xFFFFFFFFC0000000ULL, 0xFFFFFFFFE0000000ULL,
        0xFFFFFFFFF0000000ULL, 0xFFFFFFFFF8000000ULL, 0xFFFFFFFFFC000000ULL, 0xFFFFFFFFFE000000ULL,
        0xFFFFFFFFFF000000ULL, 0xFFFFFFFFFF800000ULL, 0xFFFFFFFFFFC00000ULL, 0xFFFFFFFFFFE00000ULL,
        0xFFFFFFFFFFF00000ULL, 0xFFFFFFFFFFF80000ULL, 0xFFFFFFFFFFFC0000ULL, 0xFFFFFFFFFFFE0000ULL,
        0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFFFF8000ULL, 0xFFFFFFFFFFFFC000ULL, 0xFFFFFFFFFFFFE000ULL,
        0xFFFFFFFFFFFFF000ULL, 0xFFFFFFFFFFFFF800ULL, 0xFFFFFFFFFFFFFC00ULL, 0xFFFFFFFFFFFFFE00ULL,
        0xFFFFFFFFFFFFFF00ULL, 0xFFFFFFFFFFFFFF80ULL, 0xFFFFFFFFFFFFFFC0ULL, 0xFFFFFFFFFFFFFFE0ULL,
        0xFFFFFFFFFFFFFFF0ULL, 0xFFFFFFFFFFFFFFF8ULL, 0xFFFFFFFFFFFFFFFCULL, 0xFFFFFFFFFFFFFFFEULL};

    RankMPE64() {
        this->initialize();
    }

    ~RankMPE64() {
        this->freeMemory();
    }

    void build(unsigned char* text, unsigned long long textLen) {
        switch (T) {
            case RankMPEType::RANK_MPE1:
                this->build_v1(text, textLen);
                break;
            default:
                this->build_v2(text, textLen);
                break;
        }
    }

    unsigned long long rank(unsigned long long i) {
        switch (T) {
            case RankMPEType::RANK_MPE1:
                return this->getRank_v1(i);
                break;
            default:
                return this->getRank_v2(i);
                break;
        }
    }

    void save(FILE* outFile) {
        fwrite(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        fwrite(&this->ranksLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        if (this->ranksLen > 0)
            fwrite(this->alignedRanks, (size_t)sizeof(unsigned long long), (size_t)this->ranksLen, outFile);
        fwrite(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
        if (this->bitsLen > 0) fwrite(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, outFile);
    }

    void load(FILE* inFile) {
        this->free();
        size_t result;
        result = fread(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        result = fread(&this->ranksLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->ranksLen > 0) {
            this->ranks = new unsigned long long[this->ranksLen + 16];
            this->alignedRanks = this->ranks;
            while ((unsigned long long)(this->alignedRanks) % 128) ++(this->alignedRanks);
            result = fread(this->alignedRanks, (size_t)sizeof(unsigned long long), (size_t)this->ranksLen, inFile);
            if (result != this->ranksLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
        result = fread(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
        if (result != 1) {
            cout << "Error loading rank" << endl;
            exit(1);
        }
        if (this->bitsLen > 0) {
            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
            result = fread(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, inFile);
            if (result != this->bitsLen) {
                cout << "Error loading rank" << endl;
                exit(1);
            }
        }
    }

    void free() {
        this->freeMemory();
        this->initialize();
    }

    unsigned long long getSize() {
        unsigned long long size = sizeof(this->ranksLen) + sizeof(this->bitsLen) + sizeof(this->textLen) +
                                  sizeof(this->pointer) + sizeof(this->pointer2);
        if (this->ranksLen > 0) size += (this->ranksLen + 16) * sizeof(unsigned long long);
        if (this->bitsLen > 0) size += (this->bitsLen + 128) * sizeof(unsigned char);
        return size;
    }

    unsigned long long getTextSize() {
        return this->textLen * sizeof(unsigned char);
    }

    void testRank(unsigned char* text, unsigned long long textLen) {
        unsigned long long rankTest = 0;

        for (unsigned long long i = 0; i < textLen; ++i) {
            unsigned int bits = (unsigned int)text[i];
            for (unsigned int j = 0; j < 8; ++j) {
                if (((bits >> (7 - j)) & 1) == 1) ++rankTest;
                if (rankTest != this->rank(8 * i + j + 1)) {
                    cout << (8 * i + j + 1) << " " << rankTest << " " << this->rank(8 * i + j + 1);
                    cin.ignore();
                }
            }
        }
    }
};

}  // namespace shared

#endif /* RANK_HPP */
