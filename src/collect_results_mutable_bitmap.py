import sys, os


output_filename = sys.argv[1]

types = [
"avx2_256_a",
"avx512_256_a",
"avx2_256_b",
"avx512_256_b",
"avx2_512_a",
"avx512_512_a",
"avx2_512_b",
"avx512_512_b",
"avx2_256_c",
"avx512_256_c"]

for t in types:
    os.system("./perf_mutable_bitmap " + t + " flip 0.3 2>> " + output_filename + ".flip.txt")
    os.system("./perf_mutable_bitmap " + t + " rank 0.3 2>> " + output_filename + ".rank.txt")
    os.system("./perf_mutable_bitmap " + t + " select 0.3 2>> " + output_filename + ".select.txt")
