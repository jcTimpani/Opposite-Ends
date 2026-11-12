#!/bin/sh
# From the Opposite Ends repository root: sh tests/run_portable_tests.sh
set -eu
CXX=${CXX:-clang++}
OUT="${TMPDIR:-/tmp}/opposite_ends_stage4_tests"
VALIDATOR="${TMPDIR:-/tmp}/opposite_ends_validate_levels"
"$CXX" -std=c++14 -O2 -Wall -Wextra -pedantic \
    tests/ObstacleSystemTests.cpp OppositeEnds/ObstacleSystem.cpp \
    OppositeEnds/PuzzleSolver.cpp -o "$OUT"
"$OUT"
"$CXX" -std=c++14 -O2 -Wall -Wextra -pedantic \
    tools/ValidateLevels.cpp OppositeEnds/ObstacleSystem.cpp \
    OppositeEnds/PuzzleSolver.cpp -o "$VALIDATOR"
"$VALIDATOR" Levels/level01.txt Levels/level02.txt Levels/level05.txt
