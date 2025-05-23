clang -fsanitize=undefined -fsanitize=unreachable -fsanitize-trap -fsanitize=fuzzer -fsanitize=address \
    -O2 -g3 -I ./include/ -I ./deps/libaven/include/ \
    fuzz/fmt.c -o fuzz_fmt
./fuzz_fmt -max_len=4096 -jobs=4 -dict=fuzz/cdict.txt -timeout=1
