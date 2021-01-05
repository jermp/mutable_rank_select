#ifndef SELECT_HPP
#define	SELECT_HPP

#include <iostream>

using namespace std;
    
namespace shared {
    
enum SelectBasicType {
        SELECT_BASIC_STANDARD = 1,
        SELECT_BASIC_COMPRESSED_HEADERS = 2
};
    
template<SelectBasicType T, unsigned int L, unsigned int THRESHOLD> class SelectBasic32 {
private:
	void freeMemory() {
            if (this->selects != NULL) delete[] this->selects;
            if (this->bits != NULL) delete[] this->bits;
        }
        
	void initialize() {
            this->selects = NULL;
            this->alignedSelects = NULL;
            this->selectsLen = 0;
            this->bits = NULL;
            this->alignedBits = NULL;
            this->bitsLen = 0;
            this->textLen = 0;
            this->pointer = NULL;
            this->pointer2 = NULL;
        }
        
        void build_std(unsigned char *text, unsigned int textLen) {
            this->free();
            this->textLen = textLen;

            unsigned int numberOfOnes = 0;
            for (unsigned int i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            this->selectsLen = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) this->selectsLen += 2;
            this->selects = new unsigned int[this->selectsLen + 32];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned int select = 0;
            unsigned int selectCounter = 0;
            unsigned int lastSelectPos = 0;
            //unsigned int prevSelectPos = 0;
            unsigned int blockLenInBits;
            this->bitsLen = 0;
            this->alignedSelects[selectCounter++] = 0;
            this->alignedSelects[selectCounter++] = 0;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        //prevSelectPos = lastSelectPos;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            this->alignedSelects[selectCounter++] = lastSelectPos;
                            this->alignedSelects[selectCounter++] = 0;
                            blockLenInBits = this->alignedSelects[selectCounter - 2] - this->alignedSelects[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (L - 1);
                            else if (blockLenInBits > L) {
                                //blockLenInBits -= (lastSelectPos - prevSelectPos - 1);
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                this->alignedSelects[selectCounter++] = lastSelectPos;
                this->alignedSelects[selectCounter++] = 0;
                blockLenInBits = this->alignedSelects[selectCounter - 2] - this->alignedSelects[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
            }

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            selectCounter = 0;
            unsigned int bitsCounter = 0;
            unsigned char bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            //unsigned int currOnes = 0;
            unsigned int upToBitPos = this->alignedSelects[2];
            blockLenInBits = this->alignedSelects[2] - this->alignedSelects[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == this->selectsLen - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned int position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    this->alignedBits[bitsCounter++] = bitsToWrite;
                                }
                            } else if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                                this->alignedBits[bitsCounter++] = bitsToWrite;
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                        }
                        this->alignedSelects[selectCounter + 1] = bitsCounter;
                        upToBitPos = this->alignedSelects[selectCounter + 2];
                        blockLenInBits = this->alignedSelects[selectCounter + 2] - this->alignedSelects[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        //currOnes = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned int position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24);
                        }
                    } else {
                        //if (currOnes >= l - 1) continue;
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            //++currOnes;
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }

            for (unsigned int i = 2; i < this->selectsLen; i += 2) ++this->alignedSelects[i];
        }
        
        void build_bch(unsigned char *text, unsigned int textLen) {
            this->free();
            this->textLen = textLen;

            unsigned int numberOfOnes = 0;
            for (unsigned int i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            unsigned int selectLenTemp = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) selectLenTemp += 2;
            unsigned int selectLenTempNotExtended = selectLenTemp;
            if (selectLenTemp % (2 * SUPERBLOCKLEN) > 0) selectLenTemp += ((2 * SUPERBLOCKLEN) - selectLenTemp % (2 * SUPERBLOCKLEN));
            unsigned int *selectsTemp = new unsigned int[selectLenTemp];

            unsigned int select = 0;
            unsigned int selectCounter = 0;
            unsigned int lastSelectPos = 0;
            //unsigned int prevSelectPos = 0;
            unsigned int blockLenInBits;
            this->bitsLen = 0;
            selectsTemp[selectCounter++] = 0;
            selectsTemp[selectCounter++] = 0;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        //prevSelectPos = lastSelectPos;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            selectsTemp[selectCounter++] = lastSelectPos;
                            selectsTemp[selectCounter++] = 0;
                            blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (L - 1);
                            else if (blockLenInBits > L) {
                                //blockLenInBits -= (lastSelectPos - prevSelectPos - 1);
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                selectsTemp[selectCounter++] = lastSelectPos;
                selectsTemp[selectCounter++] = 0;
                blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
            }
            for (unsigned int i = selectCounter; i < selectLenTemp; ++i) {
                selectsTemp[i] = 0;
            }

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            selectCounter = 0;
            unsigned int bitsCounter = 0;
            unsigned char bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            //unsigned int currOnes = 0;
            unsigned int upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned int position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    this->alignedBits[bitsCounter++] = bitsToWrite;
                                }
                            } else if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                                this->alignedBits[bitsCounter++] = bitsToWrite;
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                        }
                        selectsTemp[selectCounter + 1] = bitsCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        //currOnes = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned int position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24);
                        }
                    } else {
                        //if (currOnes >= l - 1) continue;
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            //++currOnes;
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }

            for (unsigned int i = 2; i < selectLenTempNotExtended; i += 2) ++selectsTemp[i];

            this->selectsLen = selectLenTemp / 2;
            this->selectsLen = (this->selectsLen / SUPERBLOCKLEN) * INTSINSUPERBLOCK;
            selectLenTemp /= 2;

            this->selects = new unsigned int[this->selectsLen + 32];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned int bucket[INTSINSUPERBLOCK];
            selectCounter = 0;
            for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) bucket[j] = 0;

            for (unsigned int i = 0; i < selectLenTemp; i += SUPERBLOCKLEN) {
                for (unsigned int j = 0; j < SUPERBLOCKLEN; ++j) {
                    bucket[j] = selectsTemp[2 * (i + j)];
                }
                bucket[SUPERBLOCKLEN] = selectsTemp[2 * i + 1];
                for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                    bucket[(SUPERBLOCKLEN + 1) + j / 2] += ((selectsTemp[2 * (i + j + 1) + 1] - bucket[SUPERBLOCKLEN]) << (16 * (j % 2)));
                }
                for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) {
                    this->alignedSelects[selectCounter++] = bucket[j];
                    bucket[j] = 0;
                }
            }

            delete[] selectsTemp;
        }
        
        unsigned int getSelect_std(unsigned int i) {
            unsigned int start = i / L;
            unsigned int select = this->alignedSelects[2 * start];
            i %= L;
            if (i == 0) return select - 1;
            unsigned int selectDiff = this->alignedSelects[2 * start + 2] - select;
            if (selectDiff == L) return select - 1 + i;
            if (selectDiff > THRESHOLD) return *((unsigned int *)(this->alignedBits + this->alignedSelects[2 * start + 1] + 4 * (i - 1)));
            unsigned int popcnt;
            this->pointer = this->alignedBits + this->alignedSelects[2 * start + 1];
            for (unsigned int j = 64; j < selectDiff; j += 64) {
                popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                if (popcnt >= i) break;
                i -= popcnt;
                select += 64;
                this->pointer += 8;
            }
            while(true) {
                popcnt = popcntLUT[*this->pointer];
                if (popcnt >= i) break;
                i -= popcnt;
                select += 8;
                ++this->pointer;
            }
            for (unsigned int j = 0; j < 8; ++j) {
                if ((this->pointer[0] & masks[j]) > 0) {
                    if (i == 1) return select + j;
                    --i;
                }
            }
            return 0;
        }

        unsigned int getSelect_bch(unsigned int i) {
            unsigned int start = i / L;
            this->pointer2 = this->alignedSelects + INTSINSUPERBLOCK * (start / SUPERBLOCKLEN);
            unsigned int selectFromStart = start % SUPERBLOCKLEN;
            unsigned int select = this->pointer2[selectFromStart];
            i %= L;
            if (i == 0) return select - 1;
            unsigned int selectDiff;
            if (selectFromStart == (SUPERBLOCKLEN - 1)) {
                selectDiff = this->pointer2[INTSINSUPERBLOCK] - select;
            } else {
                selectDiff = this->pointer2[selectFromStart + 1] - select;
            }
            if (selectDiff == L) return select - 1 + i;
            unsigned int offset = this->pointer2[SUPERBLOCKLEN];
            if (selectFromStart > 0) {
                if (selectFromStart % 2 == 1) offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2] & 0xFFFF);
                else offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2] >> 16);
            }
            if (selectDiff > THRESHOLD) return *((unsigned int *)(this->alignedBits + offset + 4 * (i - 1)));
            unsigned int popcnt;
            this->pointer = this->alignedBits + offset;
            for (unsigned int j = 64; j < selectDiff; j += 64) {
                popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                if (popcnt >= i) break;
                i -= popcnt;
                select += 64;
                this->pointer += 8;
            }
            while(true) {
                popcnt = popcntLUT[*this->pointer];
                if (popcnt >= i) break;
                i -= popcnt;
                select += 8;
                ++this->pointer;
            }
            for (unsigned int j = 0; j < 8; ++j) {
                if ((this->pointer[0] & masks[j]) > 0) {
                    if (i == 1) return select + j;
                    --i;
                }
            }
            return 0;
        }

public:
	unsigned int *selects;
        unsigned int *alignedSelects;
        unsigned int selectsLen;
        unsigned char *bits;
        unsigned char *alignedBits;
        unsigned int bitsLen;
        unsigned int textLen;
        unsigned char *pointer;
        unsigned int *pointer2;
        
        const static unsigned int SUPERBLOCKLEN = 16;
        const static unsigned int INTSINSUPERBLOCK = 25;
        /*
        INTSINSUPERBLOCK = SUPERBLOCKLEN + 1 + (SUPERBLOCKLEN - 1) / 2;
        if ((SUPERBLOCKLEN - 1) % 2 == 1) ++INTSINSUPERBLOCK;
        */

        const unsigned int popcntLUT[256] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};
        const unsigned char masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

	SelectBasic32() {
            if (L >= THRESHOLD) {
                cout << "Error: not valid L and THRESHOLD values (THRESHOLD should be greater than L)" << endl;
                exit(1);
            }
            if (T == SelectBasicType::SELECT_BASIC_COMPRESSED_HEADERS) {
                if ((L - 1) * sizeof(unsigned int) > (65535 / (SUPERBLOCKLEN - 1)) || (THRESHOLD - 1) > (8 * (65535 / (SUPERBLOCKLEN - 1)))) {
                    cout << "Error: not valid L and THRESHOLD values" << endl;
                    exit(1);
                }
            }
            this->initialize();
	}

	~SelectBasic32() {
            this->freeMemory();
	}

        void build(unsigned char *text, unsigned int textLen) {
            switch(T) {
                case SelectBasicType::SELECT_BASIC_COMPRESSED_HEADERS:
                    this->build_bch(text, textLen);
                    break;
                default:
                    this->build_std(text, textLen);
                    break;      
            }
        }
        
        unsigned int select(unsigned int i) {
            switch(T) {
            case SelectBasicType::SELECT_BASIC_COMPRESSED_HEADERS:
                return this->getSelect_bch(i);
                break;
            default:
                return this->getSelect_std(i);
                break;
            }
        } 
           
	void save(FILE *outFile) {
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(&this->selectsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->selectsLen > 0) fwrite(this->alignedSelects, (size_t)sizeof(unsigned int), (size_t)this->selectsLen, outFile);
            fwrite(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->bitsLen > 0) fwrite(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, outFile);
        }
	
        void load(FILE *inFile) {
            this->free();
            size_t result;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            result = fread(&this->selectsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->selectsLen > 0) {
                    this->selects = new unsigned int[this->selectsLen + 32];
                    this->alignedSelects = this->selects;
                    while ((unsigned long long)(this->alignedSelects) % 128) ++(this->alignedSelects);
                    result = fread(this->alignedSelects, (size_t)sizeof(unsigned int), (size_t)this->selectsLen, inFile);
                    if (result != this->selectsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
            result = fread(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->bitsLen > 0) {
                    this->bits = new unsigned char[this->bitsLen + 128];
                    this->alignedBits = this->bits;
                    while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
                    result = fread(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, inFile);
                    if (result != this->bitsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
	
        unsigned long long getSize() {
            unsigned long long size = sizeof(this->selectsLen) + sizeof(this->bitsLen) + sizeof(this->textLen) + sizeof(this->pointer) + sizeof(this->pointer2);
            if (this->selectsLen > 0) size += (this->selectsLen + 32) * sizeof(unsigned int);
            if (this->bitsLen > 0) size += (this->bitsLen + 128) * sizeof(unsigned char);
            return size;
        }
        
        unsigned int getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }
        
        void testSelect(unsigned char* text, unsigned int textLen) {
            unsigned int selectTest = 0;

            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++selectTest;
                        if ((8 * i + j) != this->select(selectTest)) {
                            cout << selectTest << " " << (8 * i + j) << " " << this->select(selectTest);
                            cin.ignore();
                        }
                    }
                }
            }
        }
};

template<SelectBasicType T, unsigned int L, unsigned int THRESHOLD> class SelectBasic64 {
private:
	void freeMemory() {
            if (this->selects != NULL) delete[] this->selects;
            if (this->bits != NULL) delete[] this->bits;
        }
        
	void initialize() {
            this->selects = NULL;
            this->alignedSelects = NULL;
            this->selectsLen = 0;
            this->bits = NULL;
            this->alignedBits = NULL;
            this->bitsLen = 0;
            this->textLen = 0;
            this->pointer = NULL;
            this->pointer2 = NULL;
        }
        
        void build_std(unsigned char *text, unsigned long long textLen) {
            this->free();
            this->textLen = textLen;

            unsigned long long numberOfOnes = 0;
            for (unsigned long long i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            this->selectsLen = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) this->selectsLen += 2;
            this->selects = new unsigned long long[this->selectsLen + 16];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned long long select = 0;
            unsigned long long selectCounter = 0;
            unsigned long long lastSelectPos = 0;
            //unsigned long long prevSelectPos = 0;
            unsigned long long blockLenInBits;
            this->bitsLen = 0;
            this->alignedSelects[selectCounter++] = 0;
            this->alignedSelects[selectCounter++] = 0;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        //prevSelectPos = lastSelectPos;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            this->alignedSelects[selectCounter++] = lastSelectPos;
                            this->alignedSelects[selectCounter++] = 0;
                            blockLenInBits = this->alignedSelects[selectCounter - 2] - this->alignedSelects[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (L - 1);
                            else if (blockLenInBits > L) {
                                //blockLenInBits -= (lastSelectPos - prevSelectPos - 1);
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                this->alignedSelects[selectCounter++] = lastSelectPos;
                this->alignedSelects[selectCounter++] = 0;
                blockLenInBits = this->alignedSelects[selectCounter - 2] - this->alignedSelects[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
            }

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            selectCounter = 0;
            unsigned long long bitsCounter = 0;
            unsigned char bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            //unsigned int currOnes = 0;
            unsigned long long upToBitPos = this->alignedSelects[2];
            blockLenInBits = this->alignedSelects[2] - this->alignedSelects[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == this->selectsLen - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned long long position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 56);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    this->alignedBits[bitsCounter++] = bitsToWrite;
                                }
                            } else if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                                this->alignedBits[bitsCounter++] = bitsToWrite;
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                        }
                        this->alignedSelects[selectCounter + 1] = bitsCounter;
                        upToBitPos = this->alignedSelects[selectCounter + 2];
                        blockLenInBits = this->alignedSelects[selectCounter + 2] - this->alignedSelects[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        //currOnes = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned long long position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 56);
                        }
                    } else {
                        //if (currOnes >= l - 1) continue;
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            //++currOnes;
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }

            for (unsigned long long i = 2; i < this->selectsLen; i += 2) ++this->alignedSelects[i];
        }
        
        void build_bch(unsigned char *text, unsigned long long textLen) {
            this->free();
            this->textLen = textLen;

            unsigned long long numberOfOnes = 0;
            for (unsigned long long i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            unsigned long long selectLenTemp = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) selectLenTemp += 2;
            unsigned int selectLenTempNotExtended = selectLenTemp;
            if (selectLenTemp % (2 * SUPERBLOCKLEN) > 0) selectLenTemp += ((2 * SUPERBLOCKLEN) - selectLenTemp % (2 * SUPERBLOCKLEN));
            unsigned long long *selectsTemp = new unsigned long long[selectLenTemp];

            unsigned long long select = 0;
            unsigned long long selectCounter = 0;
            unsigned long long lastSelectPos = 0;
            //unsigned long long prevSelectPos = 0;
            unsigned long long blockLenInBits;
            this->bitsLen = 0;
            selectsTemp[selectCounter++] = 0;
            selectsTemp[selectCounter++] = 0;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        //prevSelectPos = lastSelectPos;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            selectsTemp[selectCounter++] = lastSelectPos;
                            selectsTemp[selectCounter++] = 0;
                            blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (L - 1);
                            else if (blockLenInBits > L) {
                                //blockLenInBits -= (lastSelectPos - prevSelectPos - 1);
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                selectsTemp[selectCounter++] = lastSelectPos;
                selectsTemp[selectCounter++] = 0;
                blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
            }
            for (unsigned long long i = selectCounter; i < selectLenTemp; ++i) {
                selectsTemp[i] = 0;
            }

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            selectCounter = 0;
            unsigned long long bitsCounter = 0;
            unsigned char bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            //unsigned int currOnes = 0;
            unsigned long long upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned long long position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 56);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    this->alignedBits[bitsCounter++] = bitsToWrite;
                                }
                            } else if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                                this->alignedBits[bitsCounter++] = bitsToWrite;
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                        }
                        selectsTemp[selectCounter + 1] = bitsCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        //currOnes = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned long long position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 56);
                        }
                    } else {
                        //if (currOnes >= l - 1) continue;
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            //++currOnes;
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            this->alignedBits[bitsCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }

            for (unsigned long long i = 2; i < selectLenTempNotExtended; i += 2) ++selectsTemp[i];

            this->selectsLen = selectLenTemp / 2;
            this->selectsLen = (this->selectsLen / SUPERBLOCKLEN) * LONGSINSUPERBLOCK;
            selectLenTemp /= 2;

            this->selects = new unsigned long long[this->selectsLen + 16];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned long long bucket[LONGSINSUPERBLOCK];
            selectCounter = 0;
            for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) bucket[j] = 0;

            for (unsigned long long i = 0; i < selectLenTemp; i += SUPERBLOCKLEN) {
                for (unsigned int j = 0; j < SUPERBLOCKLEN; ++j) {
                    bucket[j] = selectsTemp[2 * (i + j)];
                }
                bucket[SUPERBLOCKLEN] = selectsTemp[2 * i + 1];
                for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                    bucket[(SUPERBLOCKLEN + 1) + j / 4] += ((selectsTemp[2 * (i + j + 1) + 1] - bucket[SUPERBLOCKLEN]) << (16 * (j % 4)));
                }
                for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) {
                    this->alignedSelects[selectCounter++] = bucket[j];
                    bucket[j] = 0;
                }
            }

            delete[] selectsTemp;
        }
        
        unsigned long long getSelect_std(unsigned long long i) {
            unsigned long long start = i / L;
            unsigned long long select = this->alignedSelects[2 * start];
            i %= L;
            if (i == 0) return select - 1;
            unsigned long long selectDiff = this->alignedSelects[2 * start + 2] - select;
            if (selectDiff == L) return select - 1 + i;
            if (selectDiff > THRESHOLD) return *((unsigned long long *)(this->alignedBits + this->alignedSelects[2 * start + 1] + 8 * (i - 1)));
            unsigned long long popcnt;
            this->pointer = this->alignedBits + this->alignedSelects[2 * start + 1];
            for (unsigned long long j = 64; j < selectDiff; j += 64) {
                popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                if (popcnt >= i) break;
                i -= popcnt;
                select += 64;
                this->pointer += 8;
            }
            while(true) {
                popcnt = popcntLUT[*this->pointer];
                if (popcnt >= i) break;
                i -= popcnt;
                select += 8;
                ++this->pointer;
            }
            for (unsigned int j = 0; j < 8; ++j) {
                if ((this->pointer[0] & masks[j]) > 0) {
                    if (i == 1) return select + j;
                    --i;
                }
            }
            return 0;
        }

        unsigned long long getSelect_bch(unsigned long long i) {
            unsigned long long start = i / L;
            this->pointer2 = this->alignedSelects + LONGSINSUPERBLOCK * (start / SUPERBLOCKLEN);
            unsigned long long selectFromStart = start % SUPERBLOCKLEN;
            unsigned long long select = this->pointer2[selectFromStart];
            i %= L;
            if (i == 0) return select - 1;
            unsigned long long selectDiff;
            if (selectFromStart == (SUPERBLOCKLEN - 1)) {
                selectDiff = this->pointer2[LONGSINSUPERBLOCK] - select;
            } else {
                selectDiff = this->pointer2[selectFromStart + 1] - select;
            }
            if (selectDiff == L) return select - 1 + i;
            unsigned long long offset = this->pointer2[SUPERBLOCKLEN];
            if (selectFromStart > 0) {
                switch(selectFromStart % 4) {
                    case 3:
                        offset += ((this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 32) & 0xFFFF);
                        break;
                    case 2:
                        offset += ((this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 16) & 0xFFFF);
                        break;
                    case 1:
                        offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] & 0xFFFF);
                        break;
                    default:
                        offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 48);
                        break;
                }
            }
            if (selectDiff > THRESHOLD) return *((unsigned long long *)(this->alignedBits + offset + 8 * (i - 1)));
            unsigned long long popcnt;
            this->pointer = this->alignedBits + offset;
            for (unsigned long long j = 64; j < selectDiff; j += 64) {
                popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                if (popcnt >= i) break;
                i -= popcnt;
                select += 64;
                this->pointer += 8;
            }
            while(true) {
                popcnt = popcntLUT[*this->pointer];
                if (popcnt >= i) break;
                i -= popcnt;
                select += 8;
                ++this->pointer;
            }
            for (unsigned int j = 0; j < 8; ++j) {
                if ((this->pointer[0] & masks[j]) > 0) {
                    if (i == 1) return select + j;
                    --i;
                }
            }
            return 0;
        }

public:
	unsigned long long *selects;
        unsigned long long *alignedSelects;
        unsigned long long selectsLen;
        unsigned char *bits;
        unsigned char *alignedBits;
        unsigned long long bitsLen;
        unsigned long long textLen;
        unsigned char *pointer;
        unsigned long long *pointer2;
        
        const static unsigned long long SUPERBLOCKLEN = 16;
        const static unsigned long long LONGSINSUPERBLOCK = 21;
        /*
        LONGSINSUPERBLOCK = SUPERBLOCKLEN + 1 + (SUPERBLOCKLEN - 1) / 4;
        if ((SUPERBLOCKLEN - 1) % 4 > 0) ++LONGSINSUPERBLOCK;
        */

        const unsigned long long popcntLUT[256] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};
        const unsigned char masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

	SelectBasic64() {
            if (L >= THRESHOLD) {
                cout << "Error: not valid L and THRESHOLD values (THRESHOLD should be greater than L)" << endl;
                exit(1);
            }
            if (T == SelectBasicType::SELECT_BASIC_COMPRESSED_HEADERS) {
                if ((L - 1) * sizeof(unsigned long long) > (65535 / (SUPERBLOCKLEN - 1)) || (THRESHOLD - 1) > (8 * (65535 / (SUPERBLOCKLEN - 1)))) {
                    cout << "Error: not valid L and THRESHOLD values" << endl;
                    exit(1);
                }
            }
            this->initialize();
	}

	~SelectBasic64() {
            this->freeMemory();
	}

        void build(unsigned char *text, unsigned long long textLen) {
            switch(T) {
                case SelectBasicType::SELECT_BASIC_COMPRESSED_HEADERS:
                    this->build_bch(text, textLen);
                    break;
                default:
                    this->build_std(text, textLen);
                    break;      
            }
        }
        
        unsigned long long select(unsigned long long i) {
            switch(T) {
            case SelectBasicType::SELECT_BASIC_COMPRESSED_HEADERS:
                return this->getSelect_bch(i);
                break;
            default:
                return this->getSelect_std(i);
                break;
            }
        } 
           
	void save(FILE *outFile) {
            fwrite(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
            fwrite(&this->selectsLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
            if (this->selectsLen > 0) fwrite(this->alignedSelects, (size_t)sizeof(unsigned long long), (size_t)this->selectsLen, outFile);
            fwrite(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
            if (this->bitsLen > 0) fwrite(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, outFile);
        }
	
        void load(FILE *inFile) {
            this->free();
            size_t result;
            result = fread(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            result = fread(&this->selectsLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->selectsLen > 0) {
                    this->selects = new unsigned long long[this->selectsLen + 16];
                    this->alignedSelects = this->selects;
                    while ((unsigned long long)(this->alignedSelects) % 128) ++(this->alignedSelects);
                    result = fread(this->alignedSelects, (size_t)sizeof(unsigned long long), (size_t)this->selectsLen, inFile);
                    if (result != this->selectsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
            result = fread(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->bitsLen > 0) {
                    this->bits = new unsigned char[this->bitsLen + 128];
                    this->alignedBits = this->bits;
                    while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
                    result = fread(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, inFile);
                    if (result != this->bitsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
	
        unsigned long long getSize() {
            unsigned long long size = sizeof(this->selectsLen) + sizeof(this->bitsLen) + sizeof(this->textLen) + sizeof(this->pointer) + sizeof(this->pointer2);
            if (this->selectsLen > 0) size += (this->selectsLen + 16) * sizeof(unsigned long long);
            if (this->bitsLen > 0) size += (this->bitsLen + 128) * sizeof(unsigned char);
            return size;
        }
        
        unsigned long long getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }
        
        void testSelect(unsigned char* text, unsigned long long textLen) {
            unsigned long long selectTest = 0;

            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++selectTest;
                        if ((8 * i + j) != this->select(selectTest)) {
                            cout << selectTest << " " << (8 * i + j) << " " << this->select(selectTest);
                            cin.ignore();
                        }
                    }
                }
            }
        }
};

enum SelectMPEType {
        SELECT_MPE1 = 1,
        SELECT_MPE2 = 2,
        SELECT_MPE3 = 3
};

template<SelectMPEType T, unsigned int L, unsigned int THRESHOLD> class SelectMPE32 {
private:
	void freeMemory() {
            if (this->selects != NULL) delete[] this->selects;
            if (this->bits != NULL) delete[] this->bits;
            if (this->buffer != NULL) delete[] this->buffer;
        }
        
	void initialize() {
            this->selects = NULL;
            this->alignedSelects = NULL;
            this->selectsLen = 0;
            this->bits = NULL;
            this->alignedBits = NULL;
            this->bitsLen = 0;
            this->textLen = 0;
            this->pointer = NULL;
            this->pointerBits2 = NULL;
            this->pointerData = NULL;
            this->pointer2 = NULL;
            this->buffer = new unsigned char[8 + 128];
            this->alignedBuffer = this->buffer;
            while ((unsigned long long)this->alignedBuffer % 128) ++this->alignedBuffer;
        }
        
        void build_v1(unsigned char* text, unsigned int textLen) {
            this->free();
            this->textLen = textLen;

            unsigned int numberOfOnes = 0;
            for (unsigned int i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            unsigned int selectLenTemp = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) selectLenTemp += 2;
            unsigned int selectLenTempNotExtended = selectLenTemp;
            if (selectLenTemp % (2 * SUPERBLOCKLEN) > 0) selectLenTemp += ((2 * SUPERBLOCKLEN) - selectLenTemp % (2 * SUPERBLOCKLEN));
            unsigned int *selectsTemp = new unsigned int[selectLenTemp];

            unsigned int select = 0;
            unsigned int selectCounter = 0;
            unsigned int lastSelectPos = 0;
            unsigned int blockLenInBits;
            unsigned int blockCounter = 0;
            this->bitsLen = 0;
            selectsTemp[selectCounter++] = 0;
            selectsTemp[selectCounter++] = 0;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            ++blockCounter;
                            selectsTemp[selectCounter++] = lastSelectPos;
                            selectsTemp[selectCounter++] = 0;
                            blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (L - 1);
                            else if (blockLenInBits > L) {
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                selectsTemp[selectCounter++] = lastSelectPos;
                selectsTemp[selectCounter++] = 0;
                blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
                ++blockCounter;
            }
            for (unsigned int i = selectCounter; i < selectLenTemp; ++i) {
                selectsTemp[i] = 0;
            }
            
            bool *blockToCompress = new bool[blockCounter];
            unsigned int bitsContainer[THRESHOLD / 8 + 1];
            
            selectCounter = 0;
            blockCounter = 0;
            unsigned int addedBitTablesInBytes = 0;
            unsigned int pairsToRemove = 0;
            unsigned int pairsToRemoveInBlock = 0;
            unsigned int bytesCounter = 0;
            unsigned char bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            unsigned int upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                blockToCompress[blockCounter++] = false;
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        blockToCompress[blockCounter] = false;
                        if (!sparseBlock && !monoBlock) {
                            pairsToRemoveInBlock = 0;
                            for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                if (k + 1 == bytesCounter) break;
                                if ((bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) || (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255)) ++pairsToRemoveInBlock;
                            }
                            if (pairsToRemoveInBlock > (((blockLenInBits - 1) / 128) + 1)) {
                                pairsToRemove += pairsToRemoveInBlock;
                                addedBitTablesInBytes += (2 * (((blockLenInBits - 1) / 128) + 1));
                                blockToCompress[blockCounter] = true;
                            }
                        }
                        ++blockCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (!sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            
            this->bitsLen += addedBitTablesInBytes;
            this->bitsLen -= (2 * pairsToRemove);

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            unsigned char bits1[((THRESHOLD / 8) / 2) / 8 + 1];
            unsigned char bits2[((THRESHOLD / 8) / 2) / 8 + 1];
            selectCounter = 0;
            bytesCounter = 0;
            blockCounter = 0;
            unsigned int bitsCounter = 0;
            unsigned int bitTableLen = 0;
            unsigned int bitTableOffset = 0;
            bitsToWrite = 0;
            bitsToWritePos = 7;
            upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            sparseBlock = false;
            monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            bool compressedBlock = blockToCompress[blockCounter++];
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned int position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                    for (unsigned int k = 0; k < bytesCounter; ++k) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                    }
                                }
                            } else if (!monoBlock && !sparseBlock) {
                                if (bitsToWritePos != 7) bitsContainer[bytesCounter++] = bitsToWrite;
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        if (!sparseBlock && !monoBlock) {
                            if (compressedBlock) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) {
                                    bits1[k] = 0;
                                    bits2[k] = 0;
                                }
                                bitsCounter += (2 * bitTableLen);
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) {
                                        bits2[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    }
                                    if ((bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) || (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255)) {
                                        bits1[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits1[k];
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits2[k];
                            } else {
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                        }
                        selectsTemp[selectCounter + 1] = bitsCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        compressedBlock = blockToCompress[blockCounter++];
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned int position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24);
                        }
                    } else {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            for (unsigned int i = selectCounter + 1; i < selectLenTemp; i += 2) selectsTemp[i] = selectsTemp[selectCounter - 1];

            for (unsigned int i = 2; i < selectLenTempNotExtended; i += 2) ++selectsTemp[i];

            this->selectsLen = selectLenTemp / 2;
            this->selectsLen = (this->selectsLen / SUPERBLOCKLEN) * INTSINSUPERBLOCK;
            selectLenTemp /= 2;

            this->selects = new unsigned int[this->selectsLen + 32];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned int bucket[INTSINSUPERBLOCK];
            selectCounter = 0;
            for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) bucket[j] = 0;

            for (unsigned int i = 0; i < selectLenTemp; i += SUPERBLOCKLEN) {
                for (unsigned int j = 0; j < SUPERBLOCKLEN; ++j) {
                    bucket[j] = selectsTemp[2 * (i + j)];
                }
                bucket[SUPERBLOCKLEN] = selectsTemp[2 * i + 1];
                for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                    unsigned int offsetDiff = (selectsTemp[2 * (i + j + 1) + 1] - bucket[SUPERBLOCKLEN]);
                    if (blockToCompress[i + j + 1]) offsetDiff += (1 << 15);
                    bucket[(SUPERBLOCKLEN + 1) + j / 2] += (offsetDiff << (16 * (j % 2)));   
                }
                if (blockToCompress[i]) bucket[(SUPERBLOCKLEN + 1)] += (1 << 30);
                for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) {
                    this->alignedSelects[selectCounter++] = bucket[j];
                    bucket[j] = 0;
                }
            }

            delete[] selectsTemp;
            delete[] blockToCompress;
        }
        
        void build_v2(unsigned char* text, unsigned int textLen) {
            this->free();
            this->textLen = textLen;

            unsigned int numberOfOnes = 0;
            for (unsigned int i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            unsigned int selectLenTemp = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) selectLenTemp += 2;
            unsigned int selectLenTempNotExtended = selectLenTemp;
            if (selectLenTemp % (2 * SUPERBLOCKLEN) > 0) selectLenTemp += ((2 * SUPERBLOCKLEN) - selectLenTemp % (2 * SUPERBLOCKLEN));
            unsigned int *selectsTemp = new unsigned int[selectLenTemp];

            unsigned int select = 0;
            unsigned int selectCounter = 0;
            unsigned int lastSelectPos = 0;
            unsigned int blockLenInBits;
            unsigned int blockCounter = 0;
            this->bitsLen = 0;
            selectsTemp[selectCounter++] = 0;
            selectsTemp[selectCounter++] = 0;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            ++blockCounter;
                            selectsTemp[selectCounter++] = lastSelectPos;
                            selectsTemp[selectCounter++] = 0;
                            blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (L - 1);
                            else if (blockLenInBits > L) {
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                selectsTemp[selectCounter++] = lastSelectPos;
                selectsTemp[selectCounter++] = 0;
                blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned int) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
                ++blockCounter;
            }
            for (unsigned int i = selectCounter; i < selectLenTemp; ++i) {
                selectsTemp[i] = 0;
            }
            
            unsigned char *blockToCompress = new unsigned char[blockCounter];
            unsigned int bitsContainer[THRESHOLD / 8 + 1];
            
            selectCounter = 0;
            blockCounter = 0;
            unsigned int addedBitTablesInBytes = 0;
            unsigned int pairsToRemove = 0;
            unsigned int pairs0ToRemoveInBlock = 0;
            unsigned int pairs255ToRemoveInBlock = 0;
            unsigned int bytesCounter = 0;
            unsigned char bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            unsigned int bitTableLen = 0;
            unsigned int upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                blockToCompress[blockCounter++] = 0;
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        blockToCompress[blockCounter] = 0;
                        if (!sparseBlock && !monoBlock) {
                            pairs0ToRemoveInBlock = 0;
                            pairs255ToRemoveInBlock = 0;
                            for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                if (k + 1 == bytesCounter) break;
                                if (bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) ++pairs0ToRemoveInBlock;
                                if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) ++pairs255ToRemoveInBlock;
                            }
                            bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                            switch(T) {
                                case SelectMPEType::SELECT_MPE2:
                                    if (2 * pairs0ToRemoveInBlock > bitTableLen && 2 * pairs255ToRemoveInBlock > bitTableLen) {
                                            pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                                            addedBitTablesInBytes += (2 * bitTableLen);
                                            blockToCompress[blockCounter] = 3;
                                        } else if (2 * pairs0ToRemoveInBlock > bitTableLen && 2 * pairs255ToRemoveInBlock <= bitTableLen) {
                                            pairsToRemove += pairs0ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 1;
                                        } else if (2 * pairs0ToRemoveInBlock <= bitTableLen && 2 * pairs255ToRemoveInBlock > bitTableLen) {
                                            pairsToRemove += pairs255ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 2;
                                        }
                                    break;
                                default:
                                    if (pairs0ToRemoveInBlock > PAIRS0THR && pairs255ToRemoveInBlock > PAIRS255THR) {
                                            pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                                            addedBitTablesInBytes += (2 * bitTableLen);
                                            blockToCompress[blockCounter] = 3;
                                        } else if (pairs0ToRemoveInBlock >= PAIRS0THR * 2 && pairs255ToRemoveInBlock <= PAIRS255THR) {
                                            pairsToRemove += pairs0ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 1;
                                        } else if (pairs0ToRemoveInBlock <= PAIRS0THR && pairs255ToRemoveInBlock >= PAIRS255THR * 2) {
                                            pairsToRemove += pairs255ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 2;
                                        }
                                    break;
                            }
                        }
                        ++blockCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (!sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            
            this->bitsLen += addedBitTablesInBytes;
            this->bitsLen -= (2 * pairsToRemove);

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            unsigned char bits1[((THRESHOLD / 8) / 2) / 8 + 1];
            unsigned char bits2[((THRESHOLD / 8) / 2) / 8 + 1];
            unsigned char bits[((THRESHOLD / 8) / 2) / 8 + 1];
            selectCounter = 0;
            bytesCounter = 0;
            blockCounter = 0;
            unsigned int bitsCounter = 0;
            bitTableLen = 0;
            unsigned int bitTableOffset = 0;
            bitsToWrite = 0;
            bitsToWritePos = 7;
            upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            sparseBlock = false;
            monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            unsigned int compressedBlock = blockToCompress[blockCounter++];
            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned int position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                    for (unsigned int k = 0; k < bytesCounter; ++k) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                    }
                                }
                            } else if (!monoBlock && !sparseBlock) {
                                if (bitsToWritePos != 7) bitsContainer[bytesCounter++] = bitsToWrite;
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        if (!sparseBlock && !monoBlock) {
                            if (compressedBlock == 3) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) {
                                    bits1[k] = 0;
                                    bits2[k] = 0;
                                }
                                bitsCounter += (2 * bitTableLen);
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) {
                                        bits2[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    }
                                    if ((bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) || (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255)) {
                                        bits1[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits1[k];
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits2[k];
                            } else if (compressedBlock == 1) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) bits[k] = 0;
                                bitsCounter += bitTableLen;
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) {
                                        bits[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits[k];
                            } else if (compressedBlock == 2) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) bits[k] = 0;
                                bitsCounter += bitTableLen;
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) {
                                        bits[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits[k];
                            } else {
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                        }
                        selectsTemp[selectCounter + 1] = bitsCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        compressedBlock = blockToCompress[blockCounter++];
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned int position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24);
                        }
                    } else {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            for (unsigned int i = selectCounter + 1; i < selectLenTemp; i += 2) selectsTemp[i] = selectsTemp[selectCounter - 1];

            for (unsigned int i = 2; i < selectLenTempNotExtended; i += 2) ++selectsTemp[i];

            this->selectsLen = selectLenTemp / 2;
            this->selectsLen = (this->selectsLen / SUPERBLOCKLEN) * INTSINSUPERBLOCK;
            selectLenTemp /= 2;

            this->selects = new unsigned int[this->selectsLen + 32];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned int bucket[INTSINSUPERBLOCK];
            selectCounter = 0;
            for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) bucket[j] = 0;

            for (unsigned int i = 0; i < selectLenTemp; i += SUPERBLOCKLEN) {
                for (unsigned int j = 0; j < SUPERBLOCKLEN; ++j) {
                    bucket[j] = selectsTemp[2 * (i + j)];
                }
                bucket[SUPERBLOCKLEN] = selectsTemp[2 * i + 1];
                for (unsigned int j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                    unsigned int offsetDiff = (selectsTemp[2 * (i + j + 1) + 1] - bucket[SUPERBLOCKLEN]);
                    offsetDiff += (blockToCompress[i + j + 1] << 14);
                    bucket[(SUPERBLOCKLEN + 1) + j / 2] += (offsetDiff << (16 * (j % 2)));   
                }
                bucket[SUPERBLOCKLEN + 1] += (blockToCompress[i] << 28);
                for (unsigned int j = 0; j < INTSINSUPERBLOCK; ++j) {
                    this->alignedSelects[selectCounter++] = bucket[j];
                    bucket[j] = 0;
                }
            }

            delete[] selectsTemp;
            delete[] blockToCompress;
        }
        
        unsigned int getSelect_v1(unsigned int i) {
            unsigned int start = i / L;
            this->pointer2 = this->alignedSelects + INTSINSUPERBLOCK * (start / SUPERBLOCKLEN);
            unsigned int selectFromStart = start % SUPERBLOCKLEN;
            unsigned int select = this->pointer2[selectFromStart];
            i %= L;
            if (i == 0) return select - 1;
            unsigned int selectDiff;
            if (selectFromStart == (SUPERBLOCKLEN - 1)) {
                selectDiff = this->pointer2[INTSINSUPERBLOCK] - select;
            } else {
                selectDiff = this->pointer2[selectFromStart + 1] - select;
            }
            if (selectDiff == L) return select - 1 + i;
            unsigned int offset = this->pointer2[SUPERBLOCKLEN];
            if (selectDiff > THRESHOLD) {
                switch(selectFromStart) {
                    case 0:
                        break;
                    case 1:
                        offset += (this->pointer2[SUPERBLOCKLEN + 1] & 0x3FFF);
                        break;
                    case 2:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 16) & 0x3FFF);
                        break;
                    default:
                        if (selectFromStart % 2 == 1) offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2] & 0xFFFF);
                        else offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2] >> 16);
                }
                return *((unsigned int *)(this->alignedBits + offset + 4 * (i - 1)));
            }
            bool compressedBlock;
            unsigned int offsetAdd;
            switch(selectFromStart) {
                case 0:
                    compressedBlock = ((this->pointer2[SUPERBLOCKLEN + 1] >> 30) & 1);
                    break;
                case 1:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += (offsetAdd & 0x3FFF);
                    compressedBlock = (offsetAdd >> 15) & 1;
                    break;
                case 2:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 16) & 0x3FFF);
                    compressedBlock = (offsetAdd >> 31);
                    break;
                default:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2];
                    if (selectFromStart % 2 == 1) {
                        offset += (offsetAdd & 0x7FFF);
                        compressedBlock = (offsetAdd >> 15) & 1;
                    } else {
                        offset += ((offsetAdd >> 16) & 0x7FFF);
                        compressedBlock = (offsetAdd >> 31);
                    }
            }
            this->pointer = this->alignedBits + offset;
            unsigned int popcnt;
            if (compressedBlock) {
                unsigned int bitTableLen = (((selectDiff - 1) / 128) + 1);
                unsigned int byteCounter = 0;
                int bitsToTake = selectDiff;
                this->pointerBits2 = this->pointer + bitTableLen;
                this->pointerData = this->pointerBits2 + bitTableLen;
                for (unsigned int m = 0; m < bitTableLen; ++m) {
                    for (unsigned int k = 0; k < 8; ++k) {
                        if ((this->pointer[m] & masks[k]) > 0) {
                            if ((this->pointerBits2[m] & masks[k]) > 0) {
                                this->alignedBuffer[byteCounter++] = 255;
                                this->alignedBuffer[byteCounter++] = 255;
                            } else {
                                this->alignedBuffer[byteCounter++] = 0;
                                this->alignedBuffer[byteCounter++] = 0;
                            }
                        } else {
                            this->alignedBuffer[byteCounter++] = this->pointerData[0];
                            if (bitsToTake <= 8) {
                                m = bitTableLen;
                                break;
                            }
                            this->alignedBuffer[byteCounter++] = this->pointerData[1];
                            this->pointerData += 2;
                        }
                        if (bitsToTake <= 16) {
                            m = bitTableLen;
                            break;
                        }
                        bitsToTake -= 16;
                        if (byteCounter % 8 == 0) {
                            popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                            if (popcnt >= i) {
                                byteCounter = 0;
                                while(true) {
                                    popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                    if (popcnt >= i) break;
                                    i -= popcnt;
                                    select += 8;
                                    ++byteCounter;
                                }
                                for (unsigned int j = 0; j < 8; ++j) {
                                    if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                        if (i == 1) return select + j;
                                        --i;
                                    }
                                }
                            }
                            i -= popcnt;
                            select += 64;
                            byteCounter = 0;
                        }
                    }
                }
                byteCounter = 0;
                while(true) {
                    popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                    if (popcnt >= i) break;
                    i -= popcnt;
                    select += 8;
                    ++byteCounter;
                }
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                        if (i == 1) return select + j;
                        --i;
                    }
                }
            }
            for (unsigned int j = 64; j < selectDiff; j += 64) {
                popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                if (popcnt >= i) break;
                i -= popcnt;
                select += 64;
                this->pointer += 8;
            }
            while(true) {
                popcnt = popcntLUT[*this->pointer];
                if (popcnt >= i) break;
                i -= popcnt;
                select += 8;
                ++this->pointer;
            }
            for (unsigned int j = 0; j < 8; ++j) {
                if ((this->pointer[0] & masks[j]) > 0) {
                    if (i == 1) return select + j;
                    --i;
                }
            }
            return 0;
        }

        unsigned int getSelect_v2(unsigned int i) {
            unsigned int start = i / L;
            this->pointer2 = this->alignedSelects + INTSINSUPERBLOCK * (start / SUPERBLOCKLEN);
            unsigned int selectFromStart = start % SUPERBLOCKLEN;
            unsigned int select = this->pointer2[selectFromStart];
            i %= L;
            if (i == 0) return select - 1;
            unsigned int selectDiff;
            if (selectFromStart == (SUPERBLOCKLEN - 1)) {
                selectDiff = this->pointer2[INTSINSUPERBLOCK] - select;
            } else {
                selectDiff = this->pointer2[selectFromStart + 1] - select;
            }
            if (selectDiff == L) return select - 1 + i;
            unsigned int offset = this->pointer2[SUPERBLOCKLEN];
            if (selectDiff > THRESHOLD) {
                switch(selectFromStart) {
                    case 0:
                        break;
                    case 1:
                        offset += (this->pointer2[SUPERBLOCKLEN + 1] & 0x0FFF);
                        break;
                    case 2:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 16) & 0x0FFF);
                        break;
                    default:
                        if (selectFromStart % 2 == 1) offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2] & 0xFFFF);
                        else offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2] >> 16);
                }
                return *((unsigned int *)(this->alignedBits + offset + 4 * (i - 1)));
            }
            unsigned int compressedBlock;
            unsigned int offsetAdd;
            switch(selectFromStart) {
                case 0:
                    compressedBlock = ((this->pointer2[SUPERBLOCKLEN + 1] >> 28) & 3);
                    break;
                case 1:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += (offsetAdd & 0x0FFF);
                    compressedBlock = (offsetAdd >> 14) & 3;
                    break;
                case 2:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 16) & 0x0FFF);
                    compressedBlock = (offsetAdd >> 30);
                    break;
                default:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + (selectFromStart + 1) / 2];
                    if (selectFromStart % 2 == 1) {
                        offset += (offsetAdd & 0x3FFF);
                        compressedBlock = (offsetAdd >> 14) & 3;
                    } else {
                        offset += ((offsetAdd >> 16) & 0x3FFF);
                        compressedBlock = (offsetAdd >> 30);
                    }
            }
            this->pointer = this->alignedBits + offset;
            unsigned int popcnt, bitTableLen, byteCounter;
            int bitsToTake;
            switch(compressedBlock) {
                case 1:
                    bitTableLen = (((selectDiff - 1) / 128) + 1);
                    byteCounter = 0;
                    bitsToTake = selectDiff;
                    this->pointerData = this->pointer + bitTableLen;
                    for (unsigned int m = 0; m < bitTableLen; ++m) {
                        for (unsigned int k = 0; k < 8; ++k) {
                            if ((this->pointer[m] & masks[k]) > 0) {
                                this->alignedBuffer[byteCounter++] = 0;
                                this->alignedBuffer[byteCounter++] = 0;
                            } else {
                                this->alignedBuffer[byteCounter++] = this->pointerData[0];
                                if (bitsToTake <= 8) {
                                    m = bitTableLen;
                                    break;
                                }
                                this->alignedBuffer[byteCounter++] = this->pointerData[1];
                                this->pointerData += 2;
                            }
                            if (bitsToTake <= 16) {
                                m = bitTableLen;
                                break;
                            }
                            bitsToTake -= 16;
                            if (byteCounter % 8 == 0) {
                                popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                                if (popcnt >= i) {
                                    byteCounter = 0;
                                    while(true) {
                                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                        if (popcnt >= i) break;
                                        i -= popcnt;
                                        select += 8;
                                        ++byteCounter;
                                    }
                                    for (unsigned int j = 0; j < 8; ++j) {
                                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                            if (i == 1) return select + j;
                                            --i;
                                        }
                                    }
                                }
                                i -= popcnt;
                                select += 64;
                                byteCounter = 0;
                            }
                        }
                    }
                    byteCounter = 0;
                    while(true) {
                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++byteCounter;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
                    break;
                case 2:
                    bitTableLen = (((selectDiff - 1) / 128) + 1);
                    byteCounter = 0;
                    bitsToTake = selectDiff;
                    this->pointerData = this->pointer + bitTableLen;
                    for (unsigned int m = 0; m < bitTableLen; ++m) {
                        for (unsigned int k = 0; k < 8; ++k) {
                            if ((this->pointer[m] & masks[k]) > 0) {
                                this->alignedBuffer[byteCounter++] = 255;
                                this->alignedBuffer[byteCounter++] = 255;
                            } else {
                                this->alignedBuffer[byteCounter++] = this->pointerData[0];
                                if (bitsToTake <= 8) {
                                    m = bitTableLen;
                                    break;
                                }
                                this->alignedBuffer[byteCounter++] = this->pointerData[1];
                                this->pointerData += 2;
                            }
                            if (bitsToTake <= 16) {
                                m = bitTableLen;
                                break;
                            }
                            bitsToTake -= 16;
                            if (byteCounter % 8 == 0) {
                                popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                                if (popcnt >= i) {
                                    byteCounter = 0;
                                    while(true) {
                                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                        if (popcnt >= i) break;
                                        i -= popcnt;
                                        select += 8;
                                        ++byteCounter;
                                    }
                                    for (unsigned int j = 0; j < 8; ++j) {
                                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                            if (i == 1) return select + j;
                                            --i;
                                        }
                                    }
                                }
                                i -= popcnt;
                                select += 64;
                                byteCounter = 0;
                            }
                        }
                    }
                    byteCounter = 0;
                    while(true) {
                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++byteCounter;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
                    break;
                case 3:
                    bitTableLen = (((selectDiff - 1) / 128) + 1);
                    byteCounter = 0;
                    bitsToTake = selectDiff;
                    this->pointerBits2 = this->pointer + bitTableLen;
                    this->pointerData = this->pointerBits2 + bitTableLen;
                    for (unsigned int m = 0; m < bitTableLen; ++m) {
                        for (unsigned int k = 0; k < 8; ++k) {
                            if ((this->pointer[m] & masks[k]) > 0) {
                                if ((this->pointerBits2[m] & masks[k]) > 0) {
                                    this->alignedBuffer[byteCounter++] = 255;
                                    this->alignedBuffer[byteCounter++] = 255;
                                } else {
                                    this->alignedBuffer[byteCounter++] = 0;
                                    this->alignedBuffer[byteCounter++] = 0;
                                }
                            } else {
                                this->alignedBuffer[byteCounter++] = this->pointerData[0];
                                if (bitsToTake <= 8) {
                                    m = bitTableLen;
                                    break;
                                }
                                this->alignedBuffer[byteCounter++] = this->pointerData[1];
                                this->pointerData += 2;
                            }
                            if (bitsToTake <= 16) {
                                m = bitTableLen;
                                break;
                            }
                            bitsToTake -= 16;
                            if (byteCounter % 8 == 0) {
                                popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                                if (popcnt >= i) {
                                    byteCounter = 0;
                                    while(true) {
                                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                        if (popcnt >= i) break;
                                        i -= popcnt;
                                        select += 8;
                                        ++byteCounter;
                                    }
                                    for (unsigned int j = 0; j < 8; ++j) {
                                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                            if (i == 1) return select + j;
                                            --i;
                                        }
                                    }
                                }
                                i -= popcnt;
                                select += 64;
                                byteCounter = 0;
                            }
                        }
                    }
                    byteCounter = 0;
                    while(true) {
                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++byteCounter;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
                    break;
                default:
                    for (unsigned int j = 64; j < selectDiff; j += 64) {
                        popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 64;
                        this->pointer += 8;
                    }
                    while(true) {
                        popcnt = popcntLUT[*this->pointer];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++this->pointer;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->pointer[0] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
            }
            return 0;
        }

public:
	unsigned int *selects;
        unsigned int *alignedSelects;
        unsigned int selectsLen;
        unsigned char *bits;
        unsigned char *alignedBits;
        unsigned int bitsLen;
        unsigned int textLen;
        unsigned char *pointer;
        unsigned char *pointerBits2;
        unsigned char *pointerData;
        unsigned int *pointer2;
        
        unsigned char *buffer;
        unsigned char *alignedBuffer;
        
        const static unsigned int SUPERBLOCKLEN = 16;
        const static unsigned int INTSINSUPERBLOCK = 25;
        const static unsigned int PAIRS0THR = 9;
        const static unsigned int PAIRS255THR = 9;
        /*
        INTSINSUPERBLOCK = SUPERBLOCKLEN + 1 + (SUPERBLOCKLEN - 1) / 2;
        if ((SUPERBLOCKLEN - 1) % 2 == 1) ++INTSINSUPERBLOCK;
        */

        const unsigned int popcntLUT[256] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};
        const unsigned char masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

	SelectMPE32() {
		if (L >= THRESHOLD) {
                    cout << "Error: not valid L and THRESHOLD values (THRESHOLD should be greater than L)" << endl;
                    exit(1);
                }
                switch(T) {
                    case SelectMPEType::SELECT_MPE1:
                        if ((L - 1) * sizeof(unsigned int) > (32767 / (SUPERBLOCKLEN - 1)) || (THRESHOLD - 1) > (8 * (32767 / (SUPERBLOCKLEN - 1)))) {
                            cout << "Error: not valid L and THRESHOLD values" << endl;
                            exit(1);
                        }
                        break;
                    default:
                        if ((L - 1) * sizeof(unsigned int) > (16383 / (SUPERBLOCKLEN - 1)) || (THRESHOLD - 1) > (8 * (16383 / (SUPERBLOCKLEN - 1)))) {
                            cout << "Error: not valid L and THRESHOLD values" << endl;
                            exit(1);
                        }
                }
		this->initialize();
	}

	~SelectMPE32() {
		this->freeMemory();
	}

        void build(unsigned char *text, unsigned int textLen) {
            switch(T) {
                case SelectMPEType::SELECT_MPE1:
                    this->build_v1(text, textLen);
                    break;
                default:
                    this->build_v2(text, textLen);
                    break;      
            }
        }
        
        unsigned int select(unsigned int i) {
                switch(T) {
                case SelectMPEType::SELECT_MPE1:
                        return this->getSelect_v1(i);
                        break;
                default:
                        return this->getSelect_v2(i);
                        break;
                }
        } 
           
	void save(FILE *outFile) {
            fwrite(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            fwrite(&this->selectsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->selectsLen > 0) fwrite(this->alignedSelects, (size_t)sizeof(unsigned int), (size_t)this->selectsLen, outFile);
            fwrite(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, outFile);
            if (this->bitsLen > 0) fwrite(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, outFile);
        }
	
        void load(FILE *inFile) {
            this->free();
            size_t result;
            result = fread(&this->textLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            result = fread(&this->selectsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->selectsLen > 0) {
                    this->selects = new unsigned int[this->selectsLen + 32];
                    this->alignedSelects = this->selects;
                    while ((unsigned long long)(this->alignedSelects) % 128) ++(this->alignedSelects);
                    result = fread(this->alignedSelects, (size_t)sizeof(unsigned int), (size_t)this->selectsLen, inFile);
                    if (result != this->selectsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
            result = fread(&this->bitsLen, (size_t)sizeof(unsigned int), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->bitsLen > 0) {
                    this->bits = new unsigned char[this->bitsLen + 128];
                    this->alignedBits = this->bits;
                    while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
                    result = fread(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, inFile);
                    if (result != this->bitsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
	
        unsigned long long getSize() {
            unsigned long long size = sizeof(this->selectsLen) + sizeof(this->bitsLen) + sizeof(this->textLen) + sizeof(this->pointer) + sizeof(this->pointer2) + sizeof(this->pointerData) + sizeof(this->pointerBits2);
            size += (8 + 128) * sizeof(unsigned char);
            if (this->selectsLen > 0) size += (this->selectsLen + 32) * sizeof(unsigned int);
            if (this->bitsLen > 0) size += (this->bitsLen + 128) * sizeof(unsigned char);
            return size;
        }
        
        unsigned int getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }
        
        void testSelect(unsigned char* text, unsigned int textLen) {
            unsigned int selectTest = 0;

            for (unsigned int i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++selectTest;
                        if ((8 * i + j) != this->select(selectTest)) {
                            cout << selectTest << " " << (8 * i + j) << " " << this->select(selectTest);
                            cin.ignore();
                        }
                    }
                }
            }
        }
};

template<SelectMPEType T, unsigned int L, unsigned int THRESHOLD> class SelectMPE64 {
private:
	void freeMemory() {
            if (this->selects != NULL) delete[] this->selects;
            if (this->bits != NULL) delete[] this->bits;
            if (this->buffer != NULL) delete[] this->buffer;
        }
        
	void initialize() {
            this->selects = NULL;
            this->alignedSelects = NULL;
            this->selectsLen = 0;
            this->bits = NULL;
            this->alignedBits = NULL;
            this->bitsLen = 0;
            this->textLen = 0;
            this->pointer = NULL;
            this->pointerBits2 = NULL;
            this->pointerData = NULL;
            this->pointer2 = NULL;
            this->buffer = new unsigned char[8 + 128];
            this->alignedBuffer = this->buffer;
            while ((unsigned long long)this->alignedBuffer % 128) ++this->alignedBuffer;
        }
        
        void build_v1(unsigned char* text, unsigned long long textLen) {
            this->free();
            this->textLen = textLen;

            unsigned long long numberOfOnes = 0;
            for (unsigned long long i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            unsigned long long selectLenTemp = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) selectLenTemp += 2;
            unsigned long long selectLenTempNotExtended = selectLenTemp;
            if (selectLenTemp % (2 * SUPERBLOCKLEN) > 0) selectLenTemp += ((2 * SUPERBLOCKLEN) - selectLenTemp % (2 * SUPERBLOCKLEN));
            unsigned long long *selectsTemp = new unsigned long long[selectLenTemp];

            unsigned long long select = 0;
            unsigned long long selectCounter = 0;
            unsigned long long lastSelectPos = 0;
            unsigned long long blockLenInBits;
            unsigned long long blockCounter = 0;
            this->bitsLen = 0;
            selectsTemp[selectCounter++] = 0;
            selectsTemp[selectCounter++] = 0;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            ++blockCounter;
                            selectsTemp[selectCounter++] = lastSelectPos;
                            selectsTemp[selectCounter++] = 0;
                            blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (L - 1);
                            else if (blockLenInBits > L) {
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                selectsTemp[selectCounter++] = lastSelectPos;
                selectsTemp[selectCounter++] = 0;
                blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
                ++blockCounter;
            }
            for (unsigned long long i = selectCounter; i < selectLenTemp; ++i) {
                selectsTemp[i] = 0;
            }
            
            bool *blockToCompress = new bool[blockCounter];
            unsigned int bitsContainer[THRESHOLD / 8 + 1];
            
            selectCounter = 0;
            blockCounter = 0;
            unsigned long long addedBitTablesInBytes = 0;
            unsigned long long pairsToRemove = 0;
            unsigned long long pairsToRemoveInBlock = 0;
            unsigned long long bytesCounter = 0;
            unsigned long long bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            unsigned long long upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                blockToCompress[blockCounter++] = false;
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        blockToCompress[blockCounter] = false;
                        if (!sparseBlock && !monoBlock) {
                            pairsToRemoveInBlock = 0;
                            for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                if (k + 1 == bytesCounter) break;
                                if ((bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) || (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255)) ++pairsToRemoveInBlock;
                            }
                            if (pairsToRemoveInBlock > (((blockLenInBits - 1) / 128) + 1)) {
                                pairsToRemove += pairsToRemoveInBlock;
                                addedBitTablesInBytes += (2 * (((blockLenInBits - 1) / 128) + 1));
                                blockToCompress[blockCounter] = true;
                            }
                        }
                        ++blockCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (!sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            
            this->bitsLen += addedBitTablesInBytes;
            this->bitsLen -= (2 * pairsToRemove);

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            unsigned char bits1[((THRESHOLD / 8) / 2) / 8 + 1];
            unsigned char bits2[((THRESHOLD / 8) / 2) / 8 + 1];
            selectCounter = 0;
            bytesCounter = 0;
            blockCounter = 0;
            unsigned long long bitsCounter = 0;
            unsigned long long bitTableLen = 0;
            unsigned long long bitTableOffset = 0;
            bitsToWrite = 0;
            bitsToWritePos = 7;
            upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            sparseBlock = false;
            monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            bool compressedBlock = blockToCompress[blockCounter++];
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned long long position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 56);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                    for (unsigned int k = 0; k < bytesCounter; ++k) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                    }
                                }
                            } else if (!monoBlock && !sparseBlock) {
                                if (bitsToWritePos != 7) bitsContainer[bytesCounter++] = bitsToWrite;
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        if (!sparseBlock && !monoBlock) {
                            if (compressedBlock) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) {
                                    bits1[k] = 0;
                                    bits2[k] = 0;
                                }
                                bitsCounter += (2 * bitTableLen);
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) {
                                        bits2[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    }
                                    if ((bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) || (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255)) {
                                        bits1[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits1[k];
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits2[k];
                            } else {
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                        }
                        selectsTemp[selectCounter + 1] = bitsCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        compressedBlock = blockToCompress[blockCounter++];
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned long long position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 56);
                        }
                    } else {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            for (unsigned long long i = selectCounter + 1; i < selectLenTemp; i += 2) selectsTemp[i] = selectsTemp[selectCounter - 1];

            for (unsigned long long i = 2; i < selectLenTempNotExtended; i += 2) ++selectsTemp[i];

            this->selectsLen = selectLenTemp / 2;
            this->selectsLen = (this->selectsLen / SUPERBLOCKLEN) * LONGSINSUPERBLOCK;
            selectLenTemp /= 2;

            this->selects = new unsigned long long[this->selectsLen + 16];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned long long bucket[LONGSINSUPERBLOCK];
            selectCounter = 0;
            for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) bucket[j] = 0;

            for (unsigned long long i = 0; i < selectLenTemp; i += SUPERBLOCKLEN) {
                for (unsigned long long j = 0; j < SUPERBLOCKLEN; ++j) {
                    bucket[j] = selectsTemp[2 * (i + j)];
                }
                bucket[SUPERBLOCKLEN] = selectsTemp[2 * i + 1];
                for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                    unsigned long long offsetDiff = (selectsTemp[2 * (i + j + 1) + 1] - bucket[SUPERBLOCKLEN]);
                    if (blockToCompress[i + j + 1]) offsetDiff += (1 << 15);
                    bucket[(SUPERBLOCKLEN + 1) + j / 4] += (offsetDiff << (16 * (j % 4)));   
                }
                if (blockToCompress[i]) bucket[(SUPERBLOCKLEN + 1)] += (1ULL << 62);
                for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) {
                    this->alignedSelects[selectCounter++] = bucket[j];
                    bucket[j] = 0;
                }
            }

            delete[] selectsTemp;
            delete[] blockToCompress;
        }
        
        void build_v2(unsigned char* text, unsigned long long textLen) {
            this->free();
            this->textLen = textLen;

            unsigned long long numberOfOnes = 0;
            for (unsigned long long i = 0; i < textLen; ++i) numberOfOnes += __builtin_popcount(text[i]);

            unsigned long long selectLenTemp = 2 * (numberOfOnes / L) + 2;
            if ((numberOfOnes % L) > 0) selectLenTemp += 2;
            unsigned long long selectLenTempNotExtended = selectLenTemp;
            if (selectLenTemp % (2 * SUPERBLOCKLEN) > 0) selectLenTemp += ((2 * SUPERBLOCKLEN) - selectLenTemp % (2 * SUPERBLOCKLEN));
            unsigned long long *selectsTemp = new unsigned long long[selectLenTemp];

            unsigned long long select = 0;
            unsigned long long selectCounter = 0;
            unsigned long long lastSelectPos = 0;
            unsigned long long blockLenInBits;
            unsigned long long blockCounter = 0;
            this->bitsLen = 0;
            selectsTemp[selectCounter++] = 0;
            selectsTemp[selectCounter++] = 0;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++select;
                        lastSelectPos = 8 * i + j;
                        if (select % L == 0) {
                            ++blockCounter;
                            selectsTemp[selectCounter++] = lastSelectPos;
                            selectsTemp[selectCounter++] = 0;
                            blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                            if (selectCounter == 4) ++blockLenInBits;
                            if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (L - 1);
                            else if (blockLenInBits > L) {
                                if (selectCounter == 4) { //for select[0] = 0
                                    this->bitsLen += (blockLenInBits / 8);
                                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                                } else {
                                    this->bitsLen += ((blockLenInBits - 1) / 8);
                                    if ((blockLenInBits - 1) % 8 > 0) ++this->bitsLen;
                                }
                            }
                        }
                    }
                }
            }
            if ((numberOfOnes % L) > 0) {
                selectsTemp[selectCounter++] = lastSelectPos;
                selectsTemp[selectCounter++] = 0;
                blockLenInBits = selectsTemp[selectCounter - 2] - selectsTemp[selectCounter - 4];
                if (blockLenInBits > THRESHOLD) this->bitsLen += sizeof(unsigned long long) * (select % L);
                else {
                    this->bitsLen += (blockLenInBits / 8);
                    if (blockLenInBits % 8 > 0) ++this->bitsLen;
                }
                ++blockCounter;
            }
            for (unsigned long long i = selectCounter; i < selectLenTemp; ++i) {
                selectsTemp[i] = 0;
            }
            
            unsigned char *blockToCompress = new unsigned char[blockCounter];
            unsigned int bitsContainer[THRESHOLD / 8 + 1];
            
            selectCounter = 0;
            blockCounter = 0;
            unsigned long long addedBitTablesInBytes = 0;
            unsigned long long pairsToRemove = 0;
            unsigned long long pairs0ToRemoveInBlock = 0;
            unsigned long long pairs255ToRemoveInBlock = 0;
            unsigned long long bytesCounter = 0;
            unsigned char bitsToWrite = 0;
            unsigned int bitsToWritePos = 7;
            unsigned long long bitTableLen = 0;
            unsigned long long upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            bool sparseBlock = false, monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                blockToCompress[blockCounter++] = 0;
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        blockToCompress[blockCounter] = 0;
                        if (!sparseBlock && !monoBlock) {
                            pairs0ToRemoveInBlock = 0;
                            pairs255ToRemoveInBlock = 0;
                            for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                if (k + 1 == bytesCounter) break;
                                if (bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) ++pairs0ToRemoveInBlock;
                                if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) ++pairs255ToRemoveInBlock;
                            }
                            bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                            switch(T) {
                                case SelectMPEType::SELECT_MPE2:
                                    if (2 * pairs0ToRemoveInBlock > bitTableLen && 2 * pairs255ToRemoveInBlock > bitTableLen) {
                                            pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                                            addedBitTablesInBytes += (2 * bitTableLen);
                                            blockToCompress[blockCounter] = 3;
                                        } else if (2 * pairs0ToRemoveInBlock > bitTableLen && 2 * pairs255ToRemoveInBlock <= bitTableLen) {
                                            pairsToRemove += pairs0ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 1;
                                        } else if (2 * pairs0ToRemoveInBlock <= bitTableLen && 2 * pairs255ToRemoveInBlock > bitTableLen) {
                                            pairsToRemove += pairs255ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 2;
                                        }
                                    break;
                                default:
                                    if (pairs0ToRemoveInBlock > PAIRS0THR && pairs255ToRemoveInBlock > PAIRS255THR) {
                                            pairsToRemove += (pairs0ToRemoveInBlock + pairs255ToRemoveInBlock);
                                            addedBitTablesInBytes += (2 * bitTableLen);
                                            blockToCompress[blockCounter] = 3;
                                        } else if (pairs0ToRemoveInBlock >= PAIRS0THR * 2 && pairs255ToRemoveInBlock <= PAIRS255THR) {
                                            pairsToRemove += pairs0ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 1;
                                        } else if (pairs0ToRemoveInBlock <= PAIRS0THR && pairs255ToRemoveInBlock >= PAIRS255THR * 2) {
                                            pairsToRemove += pairs255ToRemoveInBlock;
                                            addedBitTablesInBytes += bitTableLen;
                                            blockToCompress[blockCounter] = 2;
                                        }
                                    break;
                            }
                        }
                        ++blockCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (!sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            
            this->bitsLen += addedBitTablesInBytes;
            this->bitsLen -= (2 * pairsToRemove);

            this->bits = new unsigned char[this->bitsLen + 128];
            this->alignedBits = this->bits;
            while ((unsigned long long)this->alignedBits % 128) ++this->alignedBits;

            unsigned char bits1[((THRESHOLD / 8) / 2) / 8 + 1];
            unsigned char bits2[((THRESHOLD / 8) / 2) / 8 + 1];
            unsigned char bits[((THRESHOLD / 8) / 2) / 8 + 1];
            selectCounter = 0;
            bytesCounter = 0;
            blockCounter = 0;
            unsigned long long bitsCounter = 0;
            bitTableLen = 0;
            unsigned long long bitTableOffset = 0;
            bitsToWrite = 0;
            bitsToWritePos = 7;
            upToBitPos = selectsTemp[2];
            blockLenInBits = selectsTemp[2] - selectsTemp[0] + 1;
            sparseBlock = false;
            monoBlock = false;
            if (blockLenInBits > THRESHOLD) sparseBlock = true;
            else if (blockLenInBits == L) monoBlock = true;
            unsigned int compressedBlock = blockToCompress[blockCounter++];
            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((8 * i + j) == upToBitPos) {
                        selectCounter += 2;
                        if (selectCounter == selectLenTempNotExtended - 2) {
                            if ((numberOfOnes % L) > 0) {
                                if (sparseBlock && (((text[i] >> (7 - j)) & 1) == 1)) {
                                    unsigned long long position = 8 * i + j;
                                    this->alignedBits[bitsCounter++] = position & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                                    this->alignedBits[bitsCounter++] = (position >> 56);
                                }
                                if (!sparseBlock && !monoBlock) {
                                    if (((text[i] >> (7 - j)) & 1) == 1) {
                                        bitsToWrite += (1 << bitsToWritePos);
                                    }
                                    bitsContainer[bytesCounter++] = bitsToWrite;
                                    for (unsigned int k = 0; k < bytesCounter; ++k) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                    }
                                }
                            } else if (!monoBlock && !sparseBlock) {
                                if (bitsToWritePos != 7) bitsContainer[bytesCounter++] = bitsToWrite;
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                            i = textLen;
                            break;
                        }
                        if (!monoBlock && !sparseBlock && bitsToWritePos != 7) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                        }
                        if (!sparseBlock && !monoBlock) {
                            if (compressedBlock == 3) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) {
                                    bits1[k] = 0;
                                    bits2[k] = 0;
                                }
                                bitsCounter += (2 * bitTableLen);
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) {
                                        bits2[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    }
                                    if ((bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) || (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255)) {
                                        bits1[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits1[k];
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits2[k];
                            } else if (compressedBlock == 1) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) bits[k] = 0;
                                bitsCounter += bitTableLen;
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 0 && bitsContainer[k + 1] == 0) {
                                        bits[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits[k];
                            } else if (compressedBlock == 2) {
                                bitTableOffset = bitsCounter;
                                bitTableLen = (((blockLenInBits - 1) / 128) + 1);
                                for (unsigned int k = 0; k < bitTableLen; ++k) bits[k] = 0;
                                bitsCounter += bitTableLen;
                                for (unsigned int k = 0; k < bytesCounter; k += 2) {
                                    if (k + 1 == bytesCounter) {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        break;
                                    }
                                    if (bitsContainer[k] == 255 && bitsContainer[k + 1] == 255) {
                                        bits[k / 16] += (1 << (7 - ((k % 16) / 2)));
                                    } else {
                                        this->alignedBits[bitsCounter++] = bitsContainer[k];
                                        this->alignedBits[bitsCounter++] = bitsContainer[k + 1];
                                    }
                                }
                                for (unsigned int k = 0; k < bitTableLen; ++k) this->alignedBits[bitTableOffset++] = bits[k];
                            } else {
                                for (unsigned int k = 0; k < bytesCounter; ++k) {
                                    this->alignedBits[bitsCounter++] = bitsContainer[k];
                                }
                            }
                        }
                        selectsTemp[selectCounter + 1] = bitsCounter;
                        upToBitPos = selectsTemp[selectCounter + 2];
                        blockLenInBits = selectsTemp[selectCounter + 2] - selectsTemp[selectCounter];
                        sparseBlock = false;
                        monoBlock = false;
                        if (blockLenInBits > THRESHOLD) sparseBlock = true;
                        else if (blockLenInBits == L) monoBlock = true;
                        compressedBlock = blockToCompress[blockCounter++];
                        bitsToWrite = 0;
                        bitsToWritePos = 7;
                        bytesCounter = 0;
                        continue;
                    }
                    if (monoBlock) continue;
                    if (sparseBlock) {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            unsigned long long position = 8 * i + j;
                            this->alignedBits[bitsCounter++] = position & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 8) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 16) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 24) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 32) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 40) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 48) & 0xFF;
                            this->alignedBits[bitsCounter++] = (position >> 56);
                        }
                    } else {
                        if (((text[i] >> (7 - j)) & 1) == 1) {
                            bitsToWrite += (1 << bitsToWritePos);
                        }
                        if (bitsToWritePos == 0) {
                            bitsContainer[bytesCounter++] = bitsToWrite;
                            bitsToWrite = 0;
                            bitsToWritePos = 7;
                        } else {
                            --bitsToWritePos;
                        }
                    }
                }
            }
            for (unsigned long long i = selectCounter + 1; i < selectLenTemp; i += 2) selectsTemp[i] = selectsTemp[selectCounter - 1];

            for (unsigned long long i = 2; i < selectLenTempNotExtended; i += 2) ++selectsTemp[i];

            this->selectsLen = selectLenTemp / 2;
            this->selectsLen = (this->selectsLen / SUPERBLOCKLEN) * LONGSINSUPERBLOCK;
            selectLenTemp /= 2;

            this->selects = new unsigned long long[this->selectsLen + 16];
            this->alignedSelects = this->selects;
            while ((unsigned long long)this->alignedSelects % 128) ++this->alignedSelects;

            unsigned long long bucket[LONGSINSUPERBLOCK];
            selectCounter = 0;
            for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) bucket[j] = 0;

            for (unsigned long long i = 0; i < selectLenTemp; i += SUPERBLOCKLEN) {
                for (unsigned long long j = 0; j < SUPERBLOCKLEN; ++j) {
                    bucket[j] = selectsTemp[2 * (i + j)];
                }
                bucket[SUPERBLOCKLEN] = selectsTemp[2 * i + 1];
                for (unsigned long long j = 0; j < SUPERBLOCKLEN - 1; ++j) {
                    unsigned long long offsetDiff = (selectsTemp[2 * (i + j + 1) + 1] - bucket[SUPERBLOCKLEN]);
                    offsetDiff += (blockToCompress[i + j + 1] << 14);
                    bucket[(SUPERBLOCKLEN + 1) + j / 4] += (offsetDiff << (16 * (j % 4)));   
                }
                bucket[SUPERBLOCKLEN + 1] += ((unsigned long long)blockToCompress[i] << 60);
                for (unsigned long long j = 0; j < LONGSINSUPERBLOCK; ++j) {
                    this->alignedSelects[selectCounter++] = bucket[j];
                    bucket[j] = 0;
                }
            }

            delete[] selectsTemp;
            delete[] blockToCompress;
        }
        
        unsigned long long getSelect_v1(unsigned long long i) {
            unsigned long long start = i / L;
            this->pointer2 = this->alignedSelects + LONGSINSUPERBLOCK * (start / SUPERBLOCKLEN);
            unsigned long long selectFromStart = start % SUPERBLOCKLEN;
            unsigned long long select = this->pointer2[selectFromStart];
            i %= L;
            if (i == 0) return select - 1;
            unsigned long long selectDiff;
            if (selectFromStart == (SUPERBLOCKLEN - 1)) {
                selectDiff = this->pointer2[LONGSINSUPERBLOCK] - select;
            } else {
                selectDiff = this->pointer2[selectFromStart + 1] - select;
            }
            if (selectDiff == L) return select - 1 + i;
            unsigned long long offset = this->pointer2[SUPERBLOCKLEN];
            if (selectDiff > THRESHOLD) {
                switch(selectFromStart) {
                    case 0:
                        break;
                    case 1:
                        offset += (this->pointer2[SUPERBLOCKLEN + 1] & 0x3FFF);
                        break;
                    case 2:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 16) & 0x3FFF);
                        break;
                    case 3:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 32) & 0x3FFF);
                        break;
                    case 4:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 48) & 0x3FFF);
                        break;
                    default:
                        switch(selectFromStart % 4) {
                            case 3:
                                offset += ((this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 32) & 0xFFFF);
                                break;
                            case 2:
                                offset += ((this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 16) & 0xFFFF);
                                break;
                            case 1:
                                offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] & 0xFFFF);
                                break;
                            default:
                                offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 48);
                                break;
                        }
                }
                return *((unsigned long long *)(this->alignedBits + offset + 8 * (i - 1)));
            }
            bool compressedBlock;
            unsigned long long offsetAdd;
            switch(selectFromStart) {
                case 0:
                    compressedBlock = ((this->pointer2[SUPERBLOCKLEN + 1] >> 62) & 1);
                    break;
                case 1:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += (offsetAdd & 0x3FFF);
                    compressedBlock = (offsetAdd >> 15) & 1;
                    break;
                case 2:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 16) & 0x3FFF);
                    compressedBlock = (offsetAdd >> 31) & 1;
                    break;
                case 3:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 32) & 0x3FFF);
                    compressedBlock = (offsetAdd >> 47) & 1;
                    break;
                case 4:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 48) & 0x3FFF);
                    compressedBlock = (offsetAdd >> 63);
                    break;
                default:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4];
                    switch(selectFromStart % 4) {
                        case 3:
                            offset += ((offsetAdd >> 32) & 0x7FFF);
                            compressedBlock = (offsetAdd >> 47) & 1;
                            break;
                        case 2:
                            offset += ((offsetAdd >> 16) & 0x7FFF);
                            compressedBlock = (offsetAdd >> 31) & 1;
                            break;
                        case 1:
                            offset += (offsetAdd & 0x7FFF);
                            compressedBlock = (offsetAdd >> 15) & 1;
                            break;
                        default:
                            offset += ((offsetAdd >> 48) & 0x7FFF);
                            compressedBlock = (offsetAdd >> 63);
                            break;
                    }
            }
            this->pointer = this->alignedBits + offset;
            unsigned long long popcnt;
            if (compressedBlock) {
                unsigned long long bitTableLen = (((selectDiff - 1) / 128) + 1);
                unsigned int byteCounter = 0;
                long long bitsToTake = selectDiff;
                this->pointerBits2 = this->pointer + bitTableLen;
                this->pointerData = this->pointerBits2 + bitTableLen;
                for (unsigned long long m = 0; m < bitTableLen; ++m) {
                    for (unsigned int k = 0; k < 8; ++k) {
                        if ((this->pointer[m] & masks[k]) > 0) {
                            if ((this->pointerBits2[m] & masks[k]) > 0) {
                                this->alignedBuffer[byteCounter++] = 255;
                                this->alignedBuffer[byteCounter++] = 255;
                            } else {
                                this->alignedBuffer[byteCounter++] = 0;
                                this->alignedBuffer[byteCounter++] = 0;
                            }
                        } else {
                            this->alignedBuffer[byteCounter++] = this->pointerData[0];
                            if (bitsToTake <= 8) {
                                m = bitTableLen;
                                break;
                            }
                            this->alignedBuffer[byteCounter++] = this->pointerData[1];
                            this->pointerData += 2;
                        }
                        if (bitsToTake <= 16) {
                            m = bitTableLen;
                            break;
                        }
                        bitsToTake -= 16;
                        if (byteCounter % 8 == 0) {
                            popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                            if (popcnt >= i) {
                                byteCounter = 0;
                                while(true) {
                                    popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                    if (popcnt >= i) break;
                                    i -= popcnt;
                                    select += 8;
                                    ++byteCounter;
                                }
                                for (unsigned int j = 0; j < 8; ++j) {
                                    if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                        if (i == 1) return select + j;
                                        --i;
                                    }
                                }
                            }
                            i -= popcnt;
                            select += 64;
                            byteCounter = 0;
                        }
                    }
                }
                byteCounter = 0;
                while(true) {
                    popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                    if (popcnt >= i) break;
                    i -= popcnt;
                    select += 8;
                    ++byteCounter;
                }
                for (unsigned int j = 0; j < 8; ++j) {
                    if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                        if (i == 1) return select + j;
                        --i;
                    }
                }
            }
            for (unsigned long long j = 64; j < selectDiff; j += 64) {
                popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                if (popcnt >= i) break;
                i -= popcnt;
                select += 64;
                this->pointer += 8;
            }
            while(true) {
                popcnt = popcntLUT[*this->pointer];
                if (popcnt >= i) break;
                i -= popcnt;
                select += 8;
                ++this->pointer;
            }
            for (unsigned int j = 0; j < 8; ++j) {
                if ((this->pointer[0] & masks[j]) > 0) {
                    if (i == 1) return select + j;
                    --i;
                }
            }
            return 0;
        }

        unsigned long long getSelect_v2(unsigned long long i) {
            unsigned long long start = i / L;
            this->pointer2 = this->alignedSelects + LONGSINSUPERBLOCK * (start / SUPERBLOCKLEN);
            unsigned long long selectFromStart = start % SUPERBLOCKLEN;
            unsigned long long select = this->pointer2[selectFromStart];
            i %= L;
            if (i == 0) return select - 1;
            unsigned long long selectDiff;
            if (selectFromStart == (SUPERBLOCKLEN - 1)) {
                selectDiff = this->pointer2[LONGSINSUPERBLOCK] - select;
            } else {
                selectDiff = this->pointer2[selectFromStart + 1] - select;
            }
            if (selectDiff == L) return select - 1 + i;
            unsigned long long offset = this->pointer2[SUPERBLOCKLEN];
            if (selectDiff > THRESHOLD) {
                switch(selectFromStart) {
                    case 0:
                        break;
                    case 1:
                        offset += (this->pointer2[SUPERBLOCKLEN + 1] & 0x0FFF);
                        break;
                    case 2:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 16) & 0x0FFF);
                        break;
                    case 3:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 32) & 0x0FFF);
                        break;
                    case 4:
                        offset += ((this->pointer2[SUPERBLOCKLEN + 1] >> 48) & 0x0FFF);
                        break;
                    default:
                        switch(selectFromStart % 4) {
                            case 3:
                                offset += ((this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 32) & 0xFFFF);
                                break;
                            case 2:
                                offset += ((this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 16) & 0xFFFF);
                                break;
                            case 1:
                                offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] & 0xFFFF);
                                break;
                            default:
                                offset += (this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4] >> 48);
                                break;
                        }
                }
                return *((unsigned long long *)(this->alignedBits + offset + 8 * (i - 1)));
            }
            unsigned int compressedBlock;
            unsigned long long offsetAdd;
            switch(selectFromStart) {
                case 0:
                    compressedBlock = ((this->pointer2[SUPERBLOCKLEN + 1] >> 60) & 3);
                    break;
                case 1:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += (offsetAdd & 0x0FFF);
                    compressedBlock = (offsetAdd >> 14) & 3;
                    break;
                case 2:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 16) & 0x0FFF);
                    compressedBlock = (offsetAdd >> 30) & 3;
                    break;
                case 3:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 32) & 0x0FFF);
                    compressedBlock = (offsetAdd >> 46) & 3;
                    break;
                case 4:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + 1];
                    offset += ((offsetAdd >> 48) & 0x0FFF);
                    compressedBlock = (offsetAdd >> 62);
                    break;
                default:
                    offsetAdd = this->pointer2[SUPERBLOCKLEN + (selectFromStart + 3) / 4];
                    switch(selectFromStart % 4) {
                        case 3:
                            offset += ((offsetAdd >> 32) & 0x3FFF);
                            compressedBlock = (offsetAdd >> 46) & 3;
                            break;
                        case 2:
                            offset += ((offsetAdd >> 16) & 0x3FFF);
                            compressedBlock = (offsetAdd >> 30) & 3;
                            break;
                        case 1:
                            offset += (offsetAdd & 0x3FFF);
                            compressedBlock = (offsetAdd >> 14) & 3;
                            break;
                        default:
                            offset += ((offsetAdd >> 48) & 0x3FFF);
                            compressedBlock = (offsetAdd >> 62);
                            break;
                    }
            }
            this->pointer = this->alignedBits + offset;
            unsigned long long popcnt, bitTableLen, byteCounter;
            long long bitsToTake;
            switch(compressedBlock) {
                case 1:
                    bitTableLen = (((selectDiff - 1) / 128) + 1);
                    byteCounter = 0;
                    bitsToTake = selectDiff;
                    this->pointerData = this->pointer + bitTableLen;
                    for (unsigned long long m = 0; m < bitTableLen; ++m) {
                        for (unsigned int k = 0; k < 8; ++k) {
                            if ((this->pointer[m] & masks[k]) > 0) {
                                this->alignedBuffer[byteCounter++] = 0;
                                this->alignedBuffer[byteCounter++] = 0;
                            } else {
                                this->alignedBuffer[byteCounter++] = this->pointerData[0];
                                if (bitsToTake <= 8) {
                                    m = bitTableLen;
                                    break;
                                }
                                this->alignedBuffer[byteCounter++] = this->pointerData[1];
                                this->pointerData += 2;
                            }
                            if (bitsToTake <= 16) {
                                m = bitTableLen;
                                break;
                            }
                            bitsToTake -= 16;
                            if (byteCounter % 8 == 0) {
                                popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                                if (popcnt >= i) {
                                    byteCounter = 0;
                                    while(true) {
                                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                        if (popcnt >= i) break;
                                        i -= popcnt;
                                        select += 8;
                                        ++byteCounter;
                                    }
                                    for (unsigned int j = 0; j < 8; ++j) {
                                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                            if (i == 1) return select + j;
                                            --i;
                                        }
                                    }
                                }
                                i -= popcnt;
                                select += 64;
                                byteCounter = 0;
                            }
                        }
                    }
                    byteCounter = 0;
                    while(true) {
                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++byteCounter;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
                    break;
                case 2:
                    bitTableLen = (((selectDiff - 1) / 128) + 1);
                    byteCounter = 0;
                    bitsToTake = selectDiff;
                    this->pointerData = this->pointer + bitTableLen;
                    for (unsigned long long m = 0; m < bitTableLen; ++m) {
                        for (unsigned int k = 0; k < 8; ++k) {
                            if ((this->pointer[m] & masks[k]) > 0) {
                                this->alignedBuffer[byteCounter++] = 255;
                                this->alignedBuffer[byteCounter++] = 255;
                            } else {
                                this->alignedBuffer[byteCounter++] = this->pointerData[0];
                                if (bitsToTake <= 8) {
                                    m = bitTableLen;
                                    break;
                                }
                                this->alignedBuffer[byteCounter++] = this->pointerData[1];
                                this->pointerData += 2;
                            }
                            if (bitsToTake <= 16) {
                                m = bitTableLen;
                                break;
                            }
                            bitsToTake -= 16;
                            if (byteCounter % 8 == 0) {
                                popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                                if (popcnt >= i) {
                                    byteCounter = 0;
                                    while(true) {
                                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                        if (popcnt >= i) break;
                                        i -= popcnt;
                                        select += 8;
                                        ++byteCounter;
                                    }
                                    for (unsigned int j = 0; j < 8; ++j) {
                                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                            if (i == 1) return select + j;
                                            --i;
                                        }
                                    }
                                }
                                i -= popcnt;
                                select += 64;
                                byteCounter = 0;
                            }
                        }
                    }
                    byteCounter = 0;
                    while(true) {
                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++byteCounter;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
                    break;
                case 3:
                    bitTableLen = (((selectDiff - 1) / 128) + 1);
                    byteCounter = 0;
                    bitsToTake = selectDiff;
                    this->pointerBits2 = this->pointer + bitTableLen;
                    this->pointerData = this->pointerBits2 + bitTableLen;
                    for (unsigned long long m = 0; m < bitTableLen; ++m) {
                        for (unsigned int k = 0; k < 8; ++k) {
                            if ((this->pointer[m] & masks[k]) > 0) {
                                if ((this->pointerBits2[m] & masks[k]) > 0) {
                                    this->alignedBuffer[byteCounter++] = 255;
                                    this->alignedBuffer[byteCounter++] = 255;
                                } else {
                                    this->alignedBuffer[byteCounter++] = 0;
                                    this->alignedBuffer[byteCounter++] = 0;
                                }
                            } else {
                                this->alignedBuffer[byteCounter++] = this->pointerData[0];
                                if (bitsToTake <= 8) {
                                    m = bitTableLen;
                                    break;
                                }
                                this->alignedBuffer[byteCounter++] = this->pointerData[1];
                                this->pointerData += 2;
                            }
                            if (bitsToTake <= 16) {
                                m = bitTableLen;
                                break;
                            }
                            bitsToTake -= 16;
                            if (byteCounter % 8 == 0) {
                                popcnt = __builtin_popcountll(*((unsigned long long*)this->alignedBuffer));
                                if (popcnt >= i) {
                                    byteCounter = 0;
                                    while(true) {
                                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                                        if (popcnt >= i) break;
                                        i -= popcnt;
                                        select += 8;
                                        ++byteCounter;
                                    }
                                    for (unsigned int j = 0; j < 8; ++j) {
                                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                                            if (i == 1) return select + j;
                                            --i;
                                        }
                                    }
                                }
                                i -= popcnt;
                                select += 64;
                                byteCounter = 0;
                            }
                        }
                    }
                    byteCounter = 0;
                    while(true) {
                        popcnt = popcntLUT[this->alignedBuffer[byteCounter]];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++byteCounter;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->alignedBuffer[byteCounter] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
                    break;
                default:
                    for (unsigned long long j = 64; j < selectDiff; j += 64) {
                        popcnt = __builtin_popcountll(*((unsigned long long*)this->pointer));
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 64;
                        this->pointer += 8;
                    }
                    while(true) {
                        popcnt = popcntLUT[*this->pointer];
                        if (popcnt >= i) break;
                        i -= popcnt;
                        select += 8;
                        ++this->pointer;
                    }
                    for (unsigned int j = 0; j < 8; ++j) {
                        if ((this->pointer[0] & masks[j]) > 0) {
                            if (i == 1) return select + j;
                            --i;
                        }
                    }
            }
            return 0;
        }

public:
	unsigned long long *selects;
        unsigned long long *alignedSelects;
        unsigned long long selectsLen;
        unsigned char *bits;
        unsigned char *alignedBits;
        unsigned long long bitsLen;
        unsigned long long textLen;
        unsigned char *pointer;
        unsigned char *pointerBits2;
        unsigned char *pointerData;
        unsigned long long *pointer2;
        
        unsigned char *buffer;
        unsigned char *alignedBuffer;
        
        const static unsigned long long SUPERBLOCKLEN = 16;
        const static unsigned long long LONGSINSUPERBLOCK = 21;
        const static unsigned long long PAIRS0THR = 9;
        const static unsigned long long PAIRS255THR = 9;
        /*
        LONGSINSUPERBLOCK = SUPERBLOCKLEN + 1 + (SUPERBLOCKLEN - 1) / 4;
        if ((SUPERBLOCKLEN - 1) % 4 > 0) ++LONGSINSUPERBLOCK;
        */

        const unsigned long long popcntLUT[256] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};
        const unsigned char masks[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

	SelectMPE64() {
		if (L >= THRESHOLD) {
                    cout << "Error: not valid L and THRESHOLD values (THRESHOLD should be greater than L)" << endl;
                    exit(1);
                }
                switch(T) {
                    case SelectMPEType::SELECT_MPE1:
                        if ((L - 1) * sizeof(unsigned long long) > (32767 / (SUPERBLOCKLEN - 1)) || (THRESHOLD - 1) > (8 * (32767 / (SUPERBLOCKLEN - 1)))) {
                            cout << "Error: not valid L and THRESHOLD values" << endl;
                            exit(1);
                        }
                        break;
                    default:
                        if ((L - 1) * sizeof(unsigned long long) > (16383 / (SUPERBLOCKLEN - 1)) || (THRESHOLD - 1) > (8 * (16383 / (SUPERBLOCKLEN - 1)))) {
                            cout << "Error: not valid L and THRESHOLD values" << endl;
                            exit(1);
                        }
                }
		this->initialize();
	}

	~SelectMPE64() {
		this->freeMemory();
	}

        void build(unsigned char *text, unsigned long long textLen) {
            switch(T) {
                case SelectMPEType::SELECT_MPE1:
                    this->build_v1(text, textLen);
                    break;
                default:
                    this->build_v2(text, textLen);
                    break;      
            }
        }
        
        unsigned long long select(unsigned long long i) {
                switch(T) {
                case SelectMPEType::SELECT_MPE1:
                        return this->getSelect_v1(i);
                        break;
                default:
                        return this->getSelect_v2(i);
                        break;
                }
        } 
           
	void save(FILE *outFile) {
            fwrite(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
            fwrite(&this->selectsLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
            if (this->selectsLen > 0) fwrite(this->alignedSelects, (size_t)sizeof(unsigned long long), (size_t)this->selectsLen, outFile);
            fwrite(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, outFile);
            if (this->bitsLen > 0) fwrite(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, outFile);
        }
	
        void load(FILE *inFile) {
            this->free();
            size_t result;
            result = fread(&this->textLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            result = fread(&this->selectsLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->selectsLen > 0) {
                    this->selects = new unsigned long long[this->selectsLen + 16];
                    this->alignedSelects = this->selects;
                    while ((unsigned long long)(this->alignedSelects) % 128) ++(this->alignedSelects);
                    result = fread(this->alignedSelects, (size_t)sizeof(unsigned long long), (size_t)this->selectsLen, inFile);
                    if (result != this->selectsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
            result = fread(&this->bitsLen, (size_t)sizeof(unsigned long long), (size_t)1, inFile);
            if (result != 1) {
                    cout << "Error loading select" << endl;
                    exit(1);
            }
            if (this->bitsLen > 0) {
                    this->bits = new unsigned char[this->bitsLen + 128];
                    this->alignedBits = this->bits;
                    while ((unsigned long long)(this->alignedBits) % 128) ++(this->alignedBits);
                    result = fread(this->alignedBits, (size_t)sizeof(unsigned char), (size_t)this->bitsLen, inFile);
                    if (result != this->bitsLen) {
                            cout << "Error loading select" << endl;
                            exit(1);
                    }
            }
        }
        
	void free() {
            this->freeMemory();
            this->initialize();
        }
	
        unsigned long long getSize() {
            unsigned long long size = sizeof(this->selectsLen) + sizeof(this->bitsLen) + sizeof(this->textLen) + sizeof(this->pointer) + sizeof(this->pointer2) + sizeof(this->pointerData) + sizeof(this->pointerBits2);
            size += (8 + 128) * sizeof(unsigned char);
            if (this->selectsLen > 0) size += (this->selectsLen + 16) * sizeof(unsigned long long);
            if (this->bitsLen > 0) size += (this->bitsLen + 128) * sizeof(unsigned char);
            return size;
        }
        
        unsigned long long getTextSize() {
            return this->textLen * sizeof(unsigned char);
        }
        
        void testSelect(unsigned char* text, unsigned long long textLen) {
            unsigned long long selectTest = 0;

            for (unsigned long long i = 0; i < textLen; ++i) {
                for (unsigned int j = 0; j < 8; ++j) {
                    if (((text[i] >> (7 - j)) & 1) == 1) {
                        ++selectTest;
                        if ((8 * i + j) != this->select(selectTest)) {
                            cout << selectTest << " " << (8 * i + j) << " " << this->select(selectTest);
                            cin.ignore();
                        }
                    }
                }
            }
        }
};

}

#endif	/* SELECT_HPP */

