mkdir -p .tmp
NAME=$1
shift 1


cc --std=c99 -g -S -O0 -pedantic -Werror -o "./.tmp/$NAME.s" ./src/$NAME.c
cc --std=c99 -O0 -pedantic -Werror -o "./.tmp/$NAME" ./src/$NAME.c

if [ -z ${DIFF+x} ]; then
    ./.tmp/"$NAME" "$@"
else
    mv "./.tmp/$NAME.new" "./.tmp/$NAME.old" 
    ./.tmp/"$NAME" "$@" > "./.tmp/$NAME.new"
    
    cat "./.tmp/$NAME.new" 
    echo "\nDIFF REPORT\n"
    diff "./.tmp/$NAME.new" "./.tmp/$NAME.old" 
fi

# rm "./.tmp/$NAME"