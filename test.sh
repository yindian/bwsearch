#!/bin/sh
#GDB_PREFIX="gdb --batch-silent -ex r -ex bt --args"
setllb() {
    echo
    echo "L=$1 LB=$2"
    sed -i "/^#define CSA_L /s/[0-9]\+/$1/;/^#define CSA_LB /s/[0-9]\+/$2/" csacompat.h
}
check() {
    echo -n $1 > $1
    ../mkbws $1
    $GDB_PREFIX ../chkbws $1 > $1.out
    test $1 = `cat $1.out`
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
docheck2() {
    cd bin/test
    # C(5, 0) = 1
    check aaaaa
    # C(5, 1) / S(4, 1) = 5
    check aaaab
    check aaaba
    check aabaa
    check abaaa
    check baaaa
    # C(5, 2) / S(3, 2) = 10
    check aaabb
    check aabab
    check abaab
    check baaab
    check aabba
    check ababa
    check baaba
    check abbaa
    check babaa
    check bbaaa
    # C(5, 1) / S(3, 1, 1) * C(4, 1) / S(3, 1) = 10
    check aaabc
    check aabac
    check abaac
    check baaac
    check aabca
    check abaca
    check baaca
    check abcaa
    check bacaa
    check bcaaa
    # C(5, 1) / S(2, 2, 1) * C(4, 2) / S(2, 2) = 15
    check aabbc
    check ababc
    check baabc
    check aabcb
    check abacb
    check baacb
    check aacbb
    check abcab
    check bacab
    check acabb
    check acbab
    check bcaab
    check caabb
    check cabab
    check cbaab
    # C(5, 1) / S(2, 1, 1, 1) * C(4, 1) / S(2, 1, 1) * C(3, 1) / S(2, 1) = 10
    check aabcd
    check abacd
    check baacd
    check abcad
    check bacad
    check bcaad
    check abcda
    check bacda
    check bcada
    check bcdaa
    # C(5, 1) / S(1, 1, 1, 1, 1) * C(4, 1) / S(1, 1, 1, 1) * C(3, 1) / S(1, 1, 1) * C(2, 1) / S(1, 1) = 1
    check abcde
    # C(6, 0) = 1
    check aaaaaa
    # C(6, 1) / S(5, 1) = 6
    check aaaaab
    check aaaaba
    check aaabaa
    check aabaaa
    check abaaaa
    check baaaaa
    # C(6, 2) / S(4, 2) = 15
    check aaaabb
    check aaabab
    check aabaab
    check abaaab
    check baaaab
    check aaabba
    check aababa
    check abaaba
    check baaaba
    check aabbaa
    check ababaa
    check baabaa
    check abbaaa
    check babaaa
    check bbaaaa
    # C(6, 3) / S(3, 3) = 10
    check aaabbb
    check aababb
    check abaabb
    check baaabb
    check aabbab
    check ababab
    check baabab
    check abbaab
    check babaab
    check bbaaab
    # C(6, 1) / S(4, 1, 1) * C(5, 1) / S(4, 1) = 15
    check aaaabc
    check aaabac
    check aabaac
    check abaaac
    check baaaac
    check aaabca
    check aabaca
    check abaaca
    check baaaca
    check aabcaa
    check abacaa
    check baacaa
    check abcaaa
    check bacaaa
    check bcaaaa
    # C(6, 1) / S(3, 2, 1) * C(5, 2) / S(3, 2) = 60
    check aaabbc
    check aababc
    check abaabc
    check baaabc
    check aabbac
    check ababac
    check baabac
    check abbaac
    check babaac
    check bbaaac
    check aaabcb
    check aabacb
    check abaacb
    check baaacb
    check aabbca
    check ababca
    check baabca
    check abbaca
    check babaca
    check bbaaca
    check aaacbb
    check aabcab
    check abacab
    check baacab
    check aabcba
    check abacba
    check baacba
    check abbcaa
    check babcaa
    check bbacaa
    check aacabb
    check aacbab
    check abcaab
    check bacaab
    check aacbba
    check abcaba
    check bacaba
    check abcbaa
    check bacbaa
    check bbcaaa
    check acaabb
    check acabab
    check acbaab
    check bcaaab
    check acabba
    check acbaba
    check bcaaba
    check acbbaa
    check bcabaa
    check bcbaaa
    check caaabb
    check caabab
    check cabaab
    check cbaaab
    check caabba
    check cababa
    check cbaaba
    check cabbaa
    check cbabaa
    check cbbaaa
    # C(6, 2) / S(2, 2, 2) * C(4, 2) / S(2, 2) = 15
    check aabbcc
    check ababcc
    check baabcc
    check aabcbc
    check abacbc
    check baacbc
    check aacbbc
    check abcabc
    check bacabc
    check acabbc
    check acbabc
    check bcaabc
    check caabbc
    check cababc
    check cbaabc
    # C(6, 1) / S(3, 1, 1, 1) * C(5, 1) / S(3, 1, 1) * C(4, 1) / S(3, 1) = 20
    check aaabcd
    check aabacd
    check abaacd
    check baaacd
    check aabcad
    check abacad
    check baacad
    check abcaad
    check bacaad
    check bcaaad
    check aabcda
    check abacda
    check baacda
    check abcada
    check bacada
    check bcaada
    check abcdaa
    check bacdaa
    check bcadaa
    check bcdaaa
    # C(6, 1) * S(2, 2, 1, 1) * C(5, 1) / S(2, 2, 1) * C(4, 2) / S(2, 2) = 45
    check aabbcd
    check ababcd
    check baabcd
    check aabcbd
    check abacbd
    check baacbd
    check aacbbd
    check abcabd
    check bacabd
    check acabbd
    check acbabd
    check bcaabd
    check caabbd
    check cababd
    check cbaabd
    check aabcdb
    check abacdb
    check baacdb
    check aacbdb
    check abcadb
    check bacadb
    check acabdb
    check acbadb
    check bcaadb
    check caabdb
    check cabadb
    check cbaadb
    check aacdbb
    check abcdab
    check bacdab
    check acadbb
    check acbdab
    check bcadab
    check caadbb
    check cabdab
    check cbadab
    check acdabb
    check acdbab
    check bcdaab
    check cadabb
    check cadbab
    check cbdaab
    check cdaabb
    check cdabab
    check cdbaab
    # C(6, 1) / S(2, 1, 1, 1, 1) * C(5, 1) / S(2, 1, 1, 1) * C(4, 1) / S(2, 1, 1) * C(3, 1) / S(2, 1) = 15
    check aabcde
    check abacde
    check baacde
    check abcade
    check bacade
    check bcaade
    check abcdae
    check bacdae
    check bcadae
    check bcdaae
    check abcdea
    check bacdea
    check bcadea
    check bcdaea
    check bcdeaa
    # C(6, 1) / S(1, 1, 1, 1, 1, 1) * C(5, 1) / S(1, 1, 1, 1, 1) * C(4, 1) / S(1, 1, 1, 1) * C(3, 1) / S(1, 1, 1) * C(2, 1) / S(1, 1) = 1
    check abcdef
    # auto gen
    bellnum 7 | while read s; do
        check $s
    done
    bellnum 8 | while read s; do
        check $s
    done
    cd ../..
}
docheck3() {
    cd bin/test
    for i in `seq 9 16`; do
        bellnum $i
    done | while read s; do check $s; done
    cd ../..
}
stirling2() {
    local n=$1
    local k=$2
    local c=$3
    if [ -n "$(eval "echo \$stlg_${n}_${k}_$c")" ]; then
        eval "echo \$stlg_${n}_${k}_$c"
        return 0
    fi
    local i
    local ret
    local line
    #printf "(%d %d %d)" $n $k $c >&2
    if [ $k -eq 1 ]; then
        for i in `seq $n`; do
            printf %b \\`expr 140 + $c`
        done
        echo
    elif [ $n -lt $k ]; then
        return 1
    else
        local n_1
        n_1=`expr $n - 1`
        k_1=`expr $k - 1`
        ret="$(stirling2 $n_1 $k_1 $c)"
        if [ -z "$(eval "echo \$stlg_${n_1}_${k_1}_$c")" ]; then
            eval "stlg_${n_1}_${k_1}_$c=\"$ret\""
            #echo "stlg_${n_1}_${k_1}_$c=\"$ret\"" >&2
        fi
        for line in $ret; do
            printf %s%b\\n $line \\`expr 140 + $c + $k - 1`
        done
        if [ $n -gt $k ]; then
            for i in `seq $k`; do
                ret="$(stirling2 $n_1 $k $c)"
                if [ -z "$(eval "echo \$stlg_${n_1}_${k}_$c")" ]; then
                    eval "stlg_${n_1}_${k}_$c=\"$ret\""
                    #echo "stlg_${n_1}_${k}_$c=\"$ret\"" >&2
                fi
                for line in $ret; do
                    printf %s%b\\n $line \\`expr 140 + $c + $i - 1`
                done
            done
        fi
    fi
    return 0
}
bellnum() {
    local n=$1
    local i
    for i in `seq $n`; do
        stirling2 $n $i 1
    done
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
setllb 1 4 && docheck && docheck2
setllb 2 4 && docheck && docheck2
setllb 4 4 && docheck && docheck2
setllb 1 8 && docheck && docheck2 #&& docheck3
setllb 2 8 && docheck && docheck2 #&& docheck3
setllb 4 8 && docheck && docheck2 #&& docheck3
setllb 8 8 && docheck && docheck2 #&& docheck3
mv chkbws.c.bak chkbws.c
mv csacompat.h.bak csacompat.h
touch chkbws.c
touch csacompat.h
make $MAKE_ARGS
