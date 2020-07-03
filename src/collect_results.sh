#!/bin/bash
# $1 --> output file
./perf_prefix_sum avx2 sum 2>> $1.sum.txt
./perf_prefix_sum avx2 update 2>> $1.update.txt
./perf_prefix_sum avx2 search 2>> $1.search.txt
./perf_prefix_sum avx512 sum 2>> $1.sum.txt
./perf_prefix_sum avx512 update 2>> $1.update.txt
./perf_prefix_sum avx512 search 2>> $1.search.txt