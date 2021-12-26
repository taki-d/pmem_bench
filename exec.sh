#!/usr/bin/bash

OUTPUT_PATH="$HOME/result/$(date)"
BIN_PATH="$HOME/workspace/pmem_bench/build/pmem_bench"
DAX_PATH="/dev/dax0.0"
WR_SIZE="1073741824" # 1GiB

flush_func_list=(
  "", # use clwb
  "PMEM_NO_CLFLUSHOPT=1 PMEM_NO_CLWB=1", # use clflush
  "PMEM_NO_CLWB=1" # use clflushopt
)

write_func_list=(
  "", # use avx512f
  "PMEM_AVX512F=0", # use avx
  "PMEM_AVX512F=0 PMEM_AVX=0" # use sse2
  "PMEM_NO_MOVNT=1 PMEM_NO_GENELIC_MEMCPY=1" # use libc memmove
  "PMEM_NO_MOVNT=1 PMEM_NO_GENELIC_MEMCPY=0" # use generic memmove
)

for flush_func in flushfunclist; do
  for write_func in write_func_list; do
    for is_random in 0,1; do
      for operation in 0,1,2; do
        RESULT_FILE="$(date --iso-8601=seconds)-$is_random-$operation-$flush_func-$write_func.data"

        for thread in 1,2,4,8,16; do
          for blocksize_base in ${seq 2 21}; do
            env $write_func $flush_func $BIN_PATH $DAX_PATH $is_random $operation $thread $WR_SIZE $blocksize >> $RESULT_FILE
          done
        done
      done
    done
  done
done