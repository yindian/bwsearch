#!/bin/sh
setllb() {
    echo "L=$1 LB=$2"
    sed -i "/^#define CSA_L /s/[0-9]\+/$1/;/^#define CSA_LB /s/[0-9]\+/$2/" csacompat.h
}
check() {
    echo -n $1 > $1
    ../mkbws $1
    test $1 = `../chkbws $1`
}
docheck() {
    make $MAKE_ARGS
    cd bin/test
    # C(1, 0) = 1
    check a
    # C(2, 0) = 1
    check aa
    # C(2, 1) / S(1, 1) = 1
    check ab
    # C(3, 0) = 1
    check aaa
    # C(3, 1) / S(2, 1) = 3
    check aab
    check aba
    check baa
    # C(3, 1) / S(1, 1, 1) * C(2, 1) / S(1, 1) = 1
    check abc
    # C(4, 0) = 1
    check aaaa
    # C(4, 1) / S(3, 1) = 4
    check aaab
    check aaba
    check abaa
    check baaa
    # C(4, 2) / S(2, 2) = 3
    check aabb
    check abab
    check baab
    # C(4, 1) / S(2, 1, 1) * C(3, 1) / S(2, 1) = 6
    check aabc
    check abac
    check baac
    check abca
    check baca
    check bcaa
    # C(4, 1) / S(1, 1, 1, 1) * C(3, 1) / S(1, 1, 1) * C(2, 1) / S(1, 1) = 1
    check abcd
    cd ../..
}
set -e
if [ -n "$NUMBER_OF_PROCESSORS" ]; then
    MAKE_ARGS=-j$NUMBER_OF_PROCESSORS
else
    MAKE_ARGS=-j`nproc`
fi
rm -rf bin/test
mkdir -p bin/test
cp -p chkbws.c chkbws.c.bak
cp -p csacompat.h csacompat.h.bak
sed -i '/^#if 0$/s/0/1/' chkbws.c
setllb 1 1 && docheck
setllb 1 2 && docheck
setllb 2 2 && docheck
setllb 1 4 && docheck
setllb 2 4 && docheck
setllb 4 4 && docheck
mv chkbws.c.bak chkbws.c
mv csacompat.h.bak csacompat.h
touch chkbws.c
touch csacompat.h
make $MAKE_ARGS
