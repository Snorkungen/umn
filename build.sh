# /usr/bin/sh

mkdir -p ./.build
clang -pedantic -s -O3 ./src/main.c -o ./.build/umn
