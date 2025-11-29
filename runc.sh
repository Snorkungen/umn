mkdir -p .tmp
NAME=$1
shift 1

cc --std=c99 -o "./.tmp/$NAME" ./src/$NAME.c
./.tmp/"$NAME" "$@"
# rm "./.tmp/$NAME"