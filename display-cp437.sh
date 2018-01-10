#!/bin/sh

for ANS in `ls -1 tests/pass//*.ans`; do
    echo $ANS
    cat $ANS
    echo ""
    read INPUT
    done

