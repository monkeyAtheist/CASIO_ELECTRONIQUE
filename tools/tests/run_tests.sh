#!/bin/sh
# Verifie fiches et ecrans (check_all), puis le noyau numerique (test_num).
# Usage : tools/tests/run_tests.sh [-png DIR]
set -e
cd "$(dirname "$0")/../.."
SRC=$(find src -name '*.cpp' ! -name main.cpp)
FLAGS="-std=c++17 -O1 -g -Wall -Wextra -Wno-missing-field-initializers -fsanitize=address,undefined -fno-exceptions -fno-rtti -Itools/host_sim"
g++ $FLAGS -o tools/tests/check_all tools/tests/check_all.cpp $SRC
g++ -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -o tools/tests/test_num tools/tests/test_num.cpp src/num/FMath.cpp src/num/Num.cpp -lm
tools/tests/test_num
tools/tests/check_all "$@"
