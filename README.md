Dynamic Rank/Select over Bitvectors
------

A C++ library providing rank/select queries over dynamic bitvectors.

Compiling the code <a name="compiling"></a>
------------------

The code is tested on Linux with `gcc` 7.4 and 9.2.1; on Mac 10.14 with `clang` 10.0.0 and 11.0.0.
To build the code, [`CMake`](https://cmake.org/) is required.

Clone the repository with

	git clone --recursive https://github.com/jermp/dynamic_rank_select.git

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


For the best of performance, we recommend compiling with (default configuration):

	cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_SANITIZERS=Off -DDISABLE_AVX=Off
	make -j

For a testing environment, use the following instead:

    mkdir debug_build
    cd debug_build
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DUSE_SANITIZERS=On
    make -j

<!--Benchmarks
---------

TODO-->

Unit tests <a name="testing"></a>
-----------

The unit tests are written using [doctest](https://github.com/onqtam/doctest).

After compilation, it is advised
to run the unit tests with:

	make test