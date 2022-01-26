#!/usr/bin/env bash

OUTPUT_PATH="$HOME/workspace/pmem_bench/data/$(date --iso-8601=seconds)"
BIN_PATH="$HOME/workspace/pmem_bench/build/pmem_bench"
DAX_PATH="/dev/dax0.0"
WR_SIZE="1073741824" # 1GiB
#DEBUG_ENV="PMEM2_LOG_LEVEL=4 LD_PRELOAD=/home/kinoshita/spack/opt/spack/linux-ubuntu20.04-cascadelake/gcc-9.3.0/pmdk-1.11.0-vmtyuozbowpuxi37t22ontyvdnz4li2b/lib/pmdk_debug/libpmem2.so"
DEBUG_ENV=""

flush_func_list=(
  "HOGE=0" # use clwb
  "PMEM_NO_CLFLUSHOPT=1 PMEM_NO_CLWB=1" # use clflush
  "PMEM_NO_CLWB=1" # use clflushopt
)
declare -A flush
flush["HOGE=0"]="clwb"
flush["PMEM_NO_CLFLUSHOPT=1 PMEM_NO_CLWB=1"]="clflush"
flush["PMEM_NO_CLWB=1"]="clflushopt"

declare -A write
write_func_list=(
  "HOGE=0" # use avx512f
  "PMEM_AVX512F=0" # use avx
  "PMEM_AVX512F=0 PMEM_AVX=0" # use sse2
  "PMEM_NO_MOVNT=1 PMEM_NO_GENERIC_MEMCPY=1" # use libc memmove
  "PMEM_NO_MOVNT=1 PMEM_NO_GENERIC_MEMCPY=0" # use generic memmove
)
  
write["HOGE=0"]="avx512f"
write["PMEM_AVX512F=0"]="avx"
write["PMEM_AVX512F=0 PMEM_AVX=0"]="sse2"
write["PMEM_NO_MOVNT=1 PMEM_NO_GENERIC_MEMCPY=1"]="libcmemmove"
write["PMEM_NO_MOVNT=1 PMEM_NO_GENERIC_MEMCPY=0"]="genericmemmove"

mkdir -p $OUTPUT_PATH
echo $OUTPUT_PATH

for flush_func in "${flush_func_list[@]}"; do
  for write_func in "${write_func_list[@]}"; do
    RESULT_FILE="${HOSTNAME}_${flush[$flush_func]}_${write[$write_func]}.data"
    for is_random in 0 1; do
      for operation in 0 1 2 3; do
	echo benchmarking $RESULT_FILE
        for thread in 1 2 4 8 16; do
          for blocksize_base in `seq 4 21`; do
          for kurikaeshi in `seq 5`; do
            echo env $DEBUG_ENV $write_func $flush_func $BIN_PATH $DAX_PATH $is_random $operation $thread $WR_SIZE $(echo "print 2 ** $blocksize_base" | perl)
            sudo env $DEBUG_ENV $write_func $flush_func $BIN_PATH $DAX_PATH $is_random $operation $thread $WR_SIZE $(echo "print 2 ** $blocksize_base" | perl) >> "$OUTPUT_PATH/$RESULT_FILE"
          done
          done
        done
      done
    done
  done
done

