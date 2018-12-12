
runtest () {
    rm -rf tests/pass/*.ans
    rm -rf tests/fail/*.ans
    mkdir -p tests/pass/cp437
    mkdir -p tests/pass/utf8
    mkdir -p tests/fail/cp437
    mkdir -p tests/fail/utf8
    let testcount=1;
    let passed=0;
    let failed=0;
    TESTTOTAL=`ls -1 ../THEDRAWFONTS/*.TDF | wc -l`

    for TDFFILE in ../THEDRAWFONTS/*.TDF; do 
        BASENAME=`basename ${TDFFILE}`
        OUTNAME=`echo ${BASENAME} | tr "[:upper:]" "[:lower:]"`
        rm -f output.ans
        #printf "-> [%04u/%04u] %s" $testcount $TESTTOTAL $TDFFILE
        #echo "-> [$testcount/$TESTTOTAL] $TDFFILE" 
        echo "-> UTF8 [$testcount/$TESTTOTAL] $TDFFILE" > output.ans 
        ./tdftool ../THEDRAWFONTS/${TDFFILE} "TDFTool" 1>output.ans 2>output.ans
        STATUS=$?
        if [ ${STATUS} == 0 ]; then
            mv output.ans tests/pass/utf8/${OUTNAME}.ans ;
            let passed=passed+1
            else 
            mv output.ans tests/fail/utf8/${OUTNAME}.ans ;
            let failed=failed+1
            fi

       echo "-> CP437 [$testcount/$TESTTOTAL] $TDFFILE" > output.ans
        ./tdftool -c ../THEDRAWFONTS/${TDFFILE} "ANSIUTILS" 1>output.ans 2>output.ans
        STATUS=$?
        if [ ${STATUS} == 0 ]; then
            mv output.ans tests/pass/cp437/${OUTNAME}.ans ;
            let passed=passed+1
            else 
            mv output.ans tests/fail/cp437/${OUTNAME}.ans ;
            let failed=failed+1
            fi
        
        echo "-> [$testcount/$TESTTOTAL] $TDFFILE: $STATUS"  
        let testcount=testcount+1
        done

    echo ""
    echo "PASSED: ${passed}"
    echo "FAILED: ${failed}"
    echo ""
}

# Disable noisy malloc check - we'll deal with heap/malloc corruption issues on a case-by-case basis, mkay?
# that's why we are collecting these stats in the first place

if [ ! -r ./tdftool ]; then
    echo "Please run 'make' first."
    exit 1
    fi


export MALLOC_CHECK_=0 

echo "Running tests ..."
runtest 2>/dev/null
[ -r testresults.log ] && mv testresults.log testresults.log.last
cat tests/fail/*.ans | sort | uniq -c | sort -n -r | tee testresults.log
