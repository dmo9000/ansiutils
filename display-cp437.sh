#!/bin/sh

for ANS in `ls -1 tests/pass/utf8/*.ans`; do
    echo $ANS
    cat $ANS
    echo ""
    read INPUT
    done

