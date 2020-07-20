/**
Copyright 2013 Carnegie Mellon University

Authors: Dong Zhou, David G. Andersen and Michale Kaminsky

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -* -*/

#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "shared.h"
#include "popcount.h"

namespace poppy {

const int kWordSize = 64;
const int kBasicBlockSize = 512;
const int kBasicBlockBits = 9;
const int kBasicBlockMask = kBasicBlockSize - 1;
const int kWordCountPerBasicBlock = kBasicBlockSize / kWordSize;

class Bitmap {
public:
    Bitmap() {
        pCount_ = 0;
    }

    virtual ~Bitmap() {}

    virtual uint64 rank(uint64 pos) = 0;
    virtual uint64 select(uint64 rank) = 0;

    uint64 pCount() {
        return pCount_;
    }

protected:
    uint64 pCount_;
};

class BitmapPoppy : public Bitmap {
public:
    BitmapPoppy(uint64* bits, uint64 nbits) {
        bits_ = bits;
        nbits_ = nbits;

        l1EntryCount_ = std::max(nbits_ >> 32, (uint64)1);
        l2EntryCount_ = nbits_ >> 11;
        basicBlockCount_ = nbits_ / kBasicBlockSize;

        // assert(posix_memalign((void**)&l1Entries_, kCacheLineSize,
        //                       l1EntryCount_ * sizeof(uint64)) >= 0);
        // assert(posix_memalign((void**)&l2Entries_, kCacheLineSize,
        //                       l2EntryCount_ * sizeof(uint64)) >= 0);
        if (posix_memalign((void**)&l1Entries_, kCacheLineSize,
                           l1EntryCount_ * sizeof(uint64)) < 0) {
            exit(1);
        }
        if (posix_memalign((void**)&l2Entries_, kCacheLineSize,
                           l2EntryCount_ * sizeof(uint64)) < 0) {
            exit(1);
        }

        uint64 l2Id = 0;
        uint64 basicBlockId = 0;

        pCount_ = 0;
        memset(locCount_, 0, sizeof(locCount_));

        for (uint64 i = 0; i < l1EntryCount_; i++) {
            l1Entries_[i] = pCount_;

            uint32 cum = 0;
            for (int k = 0; k < kL2EntryCountPerL1Entry; k++) {
                l2Entries_[l2Id] = cum;

                for (int offset = 0; offset < 30; offset += 10) {
                    int c = popcountLinear(
                        bits_, basicBlockId * kWordCountPerBasicBlock,
                        kBasicBlockSize);
                    cum += c;
                    basicBlockId++;
                    l2Entries_[l2Id] |= (uint64)c << (32 + offset);
                }
                cum += popcountLinear(bits_,
                                      basicBlockId * kWordCountPerBasicBlock,
                                      kBasicBlockSize);
                basicBlockId++;

                if (++l2Id >= l2EntryCount_) break;
            }

            locCount_[i] = (cum + kLocFreq - 1) / kLocFreq;
            pCount_ += cum;
        }

        basicBlockId = 0;

        for (uint64 i = 0; i < l1EntryCount_; i++) {
            loc_[i] = new uint32[locCount_[i]];
            locCount_[i] = 0;

            uint32 oneCount = 0;

            for (uint32 k = 0; k < kBasicBlockCountPerL1Entry; k++) {
                uint64 woff = basicBlockId * kWordCountPerBasicBlock;
                for (int widx = 0; widx < kWordCountPerBasicBlock; widx++)
                    for (int bit = 0; bit < kWordSize; bit++)
                        if (bits_[woff + widx] & (1ULL << (63 - bit))) {
                            oneCount++;
                            if ((oneCount & kLocFreqMask) == 1) {
                                loc_[i][locCount_[i]] = k * kBasicBlockSize +
                                                        widx * kWordSize + bit;
                                locCount_[i]++;
                            }
                        }

                basicBlockId++;
                if (basicBlockId >= basicBlockCount_) break;
            }
        }
    }

    ~BitmapPoppy() {
        free(l1Entries_);
        free(l2Entries_);
        for (uint64 i = 0; i < l1EntryCount_; i++) { delete[] loc_[i]; }
    }

    uint64 rank(uint64 pos) {
        assert(pos <= nbits_);
        //--pos;

        uint64 l1Id = pos >> 32;
        uint64 l2Id = pos >> 11;
        uint64 x = l2Entries_[l2Id];

        uint64 res = l1Entries_[l1Id] + (x & 0xFFFFFFFFULL);
        x >>= 32;

        int groupId = (pos & 2047) / 512;
        for (int i = 0; i < groupId; i++) {
            res += x & 1023;
            x >>= 10;
        }
        res += popcountLinear(
            bits_, (l2Id * 4 + groupId) * kWordCountPerBasicBlock, (pos & 511));

        return res;
    }
    uint64 select(uint64 rank) {
        assert(rank <= pCount_);

        uint64 l1Id;
        // for (l1Id = l1EntryCount_ - 1; l1Id >= 0; l1Id--) {
        for (l1Id = l1EntryCount_ - 1;; l1Id--) {
            if (l1Entries_[l1Id] < rank) {
                rank -= l1Entries_[l1Id];
                break;
            }
        }

        uint32 offset = l1Id * kL2EntryCountPerL1Entry;
        uint32 maxL2Id = kL2EntryCountPerL1Entry;
        if (l1Id == l1EntryCount_ - 1) maxL2Id = l2EntryCount_ - offset;

        uint32 pos = loc_[l1Id][(rank - 1) / kLocFreq];
        uint32 l2Id = pos >> 11;

        while (l2Id + 1 < maxL2Id &&
               (l2Entries_[l2Id + 1] & 0xFFFFFFFFULL) < rank)
            l2Id++;
        rank -= l2Entries_[l2Id] & 0xFFFFFFFFULL;

        uint32 x = l2Entries_[l2Id] >> 32;
        int groupId;

        for (groupId = 0; groupId < 3; groupId++) {
            uint64 k = x & 1023;
            if (rank > k)
                rank -= k;
            else
                break;
            x >>= 10;
        }

        return (l1Id << 32) + (l2Id << 11) + (groupId << 9) +
               select512(
                   bits_,
                   ((offset + l2Id) * 4 + groupId) * kWordCountPerBasicBlock,
                   rank);
    }

    uint64 size() {
        return nbits_;
    }

    uint64 bytes() {
        uint64 ret = 0;
        ret += sizeof(nbits_);                       // nbits_
        ret += sizeof(*l2Entries_) * l2EntryCount_;  // l2Entries_
        ret += sizeof(l2EntryCount_);                // l2EntryCount_
        ret += sizeof(*l1Entries_) * l1EntryCount_;  // l1Entries_
        ret += sizeof(l1EntryCount_);                // l1EntryCount_
        ret += sizeof(basicBlockCount_);             // basicBlockCount_
        for (uint32 i = 0; i < l1EntryCount_; i++) {
            ret += sizeof(*loc_[i]) * locCount_[i];  // loc_
            ret += sizeof(locCount_[i]);             // locCount_
        }
        ret += sizeof(pCount_);  // pCount_
        return ret;
    }

private:
    uint64* bits_;
    uint64 nbits_;

    uint64* l2Entries_;
    uint64 l2EntryCount_;
    uint64* l1Entries_;
    uint64 l1EntryCount_;
    uint64 basicBlockCount_;

    uint32* loc_[1 << 16];
    uint32 locCount_[1 << 16];

    static const int kLocFreq = 8192;
    static const int kLocFreqMask = 8191;
    static const int kL2EntryCountPerL1Entry = 1 << 21;
    static const int kBasicBlockCountPerL1Entry = 1 << 23;
};

}  // namespace poppy

#endif /* _BITMAP_H_ */
