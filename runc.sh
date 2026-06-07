#!/usr/bin/bash 

mkdir -p .tmp
NAME=$1
shift 1

CC=clang
CFLAGS="--std=c99 -pedantic -Werror -Wextra -Wall -O0"
CFLAGS="--std=c99 -pedantic -Werror -O0"

$CC $CFLAGS -ggdb -S -o "./.tmp/$NAME.s" ./src/$NAME.c
$CC $CFLAGS -ggdb -o "./.tmp/$NAME" ./src/$NAME.c

if [ -z ${DIFF+x} ]; then
    ./.tmp/"$NAME" "$@"
else
    mv "./.tmp/$NAME.new" "./.tmp/$NAME.old" 
    ./.tmp/"$NAME" "$@" > "./.tmp/$NAME.new"
    
    cat "./.tmp/$NAME.new" 
    echo "\nDIFF REPORT\n"
    diff "./.tmp/$NAME.new" "./.tmp/$NAME.old" 
fi

if [ $? -eq 0 ]; then
    rm "./.tmp/$NAME"
fi