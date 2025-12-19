# /usr/bin/sh

mkdir -p ./.build
clang -pedantic -Werror -s -O3 ./src/main.c -o ./.build/umn
