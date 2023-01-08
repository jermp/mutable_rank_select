Rank/Select Queries over Mutable Bitmaps
========================================

A C++ library providing rank/select queries over mutable bitmaps.

Given a *mutable* bitmap B[0..u) where n bits are set, the *rank/select problem* asks for a data structure built from B that supports:

- rank(i) is the number of bits set in B[0..i], for 0 ≤ i < u.
- select(i) is the position of the i-th bit set, for 0 ≤ i < n.
- flip(i) toggles B[i], for 0 ≤ i < u.
- access(i) = B[i], for 0 ≤ i < u.

The input bitmap is partitioned into blocks and a tree index is built over them. 
The tree index implemented in the library is an optimized b-ary Segment-Tree with
SIMD AVX2/AVX-512 instructions.
You can test a block size of 256 or 512 bits, and various rank/select algorithms for the blocks such as broadword techniques, CPU intrinsics, and SIMD instructions.


For a description and anlysis of all these data structures, see the paper [Rank/Select Queries over Mutable Bitmaps](https://www.sciencedirect.com/science/article/pii/S0306437921000235), by Giulio Ermanno Pibiri and Shunsuke Kanda, Information Systems (INFOSYS), 2021.

Please cite this paper if you use the library.

Compiling the code <a name="compiling"></a>
------------------

The code is tested on Linux with `gcc` 7.4 and 9.2.1; on Mac 10.14 with `clang` 10.0.0 and 11.0.0.
To build the code, [`CMake`](https://cmake.org/) is required.

Clone the repository with

	git clone --recursive https://github.com/jermp/mutable_rank_select.git

If you have cloned the repository without `--recursive`, you will need to perform the following commands before
compiling:

    git submodule init
    git submodule update

To compile the code for a release environment (see file `CMakeLists.txt` for the used compilation flags), it is sufficient to do the following:

    mkdir build
    cd build
    cmake ..
    make -j

By default, SIMD AVX instructions are enabled (flag `-DDISABLE_AVX=Off`). If you want to
disable them (although your compiler has proper support), you can compile with

	cmake .. -DDISABLE_AVX=On
	make -j

The library also exploits the new AVX512 instruction set. If you have proper support,
you can enable those instructions with

	cmake .. -DAVX512=On
	make -j

For the best of performance, we recommend compiling with:

	cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SANITIZERS=Off -DDISABLE_AVX=Off -DAVX512=On
	make -j

For a testing environment, use the following instead:

    mkdir debug_build
    cd debug_build
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZERS=On
    make -j

Benchmarks
---------

To benchmark the running time of rank, select, and flip for the disired data structure, use the program `src/perf_mutable_bitmap`. Running the program without arguments will show what arguments are required. (See also the file `src/perf_mutable_bitmap.cpp` for a list of available data structure types.)

Below we show some examples.

- The command

```
./perf_mutable_bitmap avx2_256_a rank 0.3
```

will benchmark the speed of rank queries for the b-ary Segment-Tree data structure with AVX2 instructions for the bitmap of density 30%, where the block size is 256. The bitmap is tested by varying the size from 2<sup>9</sup> to 2<sup>32</sup>. The suffix `_a` indicates the type of rank algorithms for a small bitmap (See `include/types.hpp` for the details).

- The command

```
./perf_mutable_bitmap avx512_512_b select 0.5
```

will benchmark the speed of select queries for the b-ary Segment-Tree data structure with AVX-512 instructions for the bitmap of density 50%, where the block size is 512.

- The command

```
./perf_mutable_bitmap avx512_256_c flip 0.8
```

will benchmark the speed of flip queries for the b-ary Segment-Tree data structure with AVX-512 instructions for the bitmap of density 80%, where the block size is 256.

Unit tests <a name="testing"></a>
-----------

The unit tests are written using [doctest](https://github.com/onqtam/doctest).

After compilation, it is advised
to run the unit tests with:

	make test
