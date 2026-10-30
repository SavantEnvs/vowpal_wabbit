#!/usr/bin/env bash
#
# mayhem/build.sh — build VW's fuzz harnesses (sanitized, DWARF 3) AND its gtest suite (normal flags).
#
# Runs inside the commit image (mayhem/Dockerfile) as `mayhem` in /mayhem. Every dependency except the
# in-tree string-view-lite header is an apt -dev package baked into the image, so a re-run is air-gapped.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# CC/CXX come from the environment (exported above): a -DCMAKE_C_COMPILER=clang flag would not match the
# cache's resolved /usr/bin/clang on a re-run, and CMake would then wipe the cache and the -D dep flags.

# Upstream's dependencies, all resolved from system packages. string-view-lite is vendored in-tree.
VW_DEPS=(-DFMT_SYS_DEP=ON -DSPDLOG_SYS_DEP=ON -DRAPIDJSON_SYS_DEP=ON -DVW_EIGEN_SYS_DEP=ON
         -DVW_BOOST_MATH_SYS_DEP=ON -DVW_ZLIB_SYS_DEP=ON -DVW_GTEST_SYS_DEP=ON)

# 1) Sanitized project + harnesses. -fsanitize=fuzzer-no-link gives the *library* coverage feedback;
#    only the harness links the engine. The build tree lives under build/ (gitignored upstream).
rm -f build/fuzz/standalone_main.o
mkdir -p build/fuzz
# The standalone driver is C: compile it as C so the harness's extern "C" LLVMFuzzerTestOneInput matches.
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o build/fuzz/standalone_main.o

FUZZ_FLAGS="$SANITIZER_FLAGS $DEBUG_FLAGS -fsanitize=fuzzer-no-link"
cmake -S . -B build/fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS="$FUZZ_FLAGS" -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_C_FLAGS_RELEASE="-O1 -DNDEBUG" -DCMAKE_CXX_FLAGS_RELEASE="-O1 -DNDEBUG" \
  -DCMAKE_EXE_LINKER_FLAGS="$SANITIZER_FLAGS" \
  -DBUILD_TESTING=OFF -DVW_BUILD_EXECUTABLES=OFF -DVW_BUILD_VW_C_WRAPPER=OFF \
  "${VW_DEPS[@]}" \
  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$SRC/mayhem/harness.cmake" \
  -DMAYHEM_DIR="$SRC/mayhem" -DMAYHEM_OUT_DIR="$SRC" \
  -DMAYHEM_LSAN_OFF_SRC="$SRC/mayhem/lsan_off.cc" \
  -DMAYHEM_LIB_FUZZING_ENGINE="$LIB_FUZZING_ENGINE" \
  -DMAYHEM_STANDALONE_MAIN_OBJ="$SRC/build/fuzz/standalone_main.o"
cmake --build build/fuzz -j"$MAYHEM_JOBS"

# 2) The project's own gtest suite with NORMAL flags (independent of the sanitized build) so
#    mayhem/test.sh only RUNS it.
cmake -S . -B build/test -G Ninja \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
  -DBUILD_TESTING=ON -DVW_BUILD_EXECUTABLES=OFF -DVW_BUILD_VW_C_WRAPPER=ON \
  "${VW_DEPS[@]}"
cmake --build build/test -j"$MAYHEM_JOBS" --target \
  vw_core_test vw_json_parser_test vw_config_test vw_io_test vw_cache_parser_test vw_common_test vw_explore_test
