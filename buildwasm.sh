#!/usr/bin/sh

mkdir -p ./.build

clang --target=wasm32 -nostdlib -Wl,--no-entry -o .build/umn.wasm ./src/wasm.c
