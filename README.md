# UMN v026

A terminal program that converts between number notations.

## Usage & Features


```sh
# umn [...options] -- number, number, number, ...

umn -boxd -- 192, 168, 1, 1
# 192 = 0b11000000, 0300, 192, 0xc0
# 168 = 0b10101000, 0250, 168, 0xa8
# 1 = 0b1, 01, 1, 0x1
# 1 = 0b1, 01, 1, 0x1

umn -d --decimal 0x1ee7
# 0x1ee7 = 7911

umn -x --hex 7911
# 7911 = 0x1ee7

umn -d --decimal 7911
# 7911 = 7911 

umn -b --binary 7911
# 7911 = 0b1111011100111

```

## Compilation

```sh
# see ./build.sh
mkdir -p ./.build
gcc ./src/main.c -o ./.build/umn
```