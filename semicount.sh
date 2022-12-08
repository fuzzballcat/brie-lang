#!/bin/bash

echo "Scripting language implementation in $(grep -o ';' main.c ./build/*.c ./build/*.h | wc -l) semicolons, $(find . \( -name '*.c' -o -name '*.h' \) -exec wc -l {} \; | awk '{total += $1} END{print total}') lines"