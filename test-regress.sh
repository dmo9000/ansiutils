#!/usr/bin/bash

#MD5SUM=`which gmd5sum 2>/dev/null`
MD5SUM=`which gmd5sum | sed -e "s/^no.*$//g" | tr -d "\n"`
if [ -z "${MD5SUM}" ] ; then
	MD5SUM=`which md5sum 2>/dev/null`
	fi


echo ""

MSG_PASS=`./ansitext FG_GREEN BOLD "[PASS]" FG_NONE NONE NEWLINE`
MSG_FAIL=`./ansitext FG_RED BOLD "[FAIL]" FG_NONE NONE NEWLINE`

printf "%-50s" "fruit.ans should always have 24 lines ...  "
TEST=`./ansiread -c ansifiles/fruit.ans 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | wc -l`
if [ $TEST == "24" ]; then 
    echo -e $MSG_PASS
  else
    echo -e "$MSG_FAIL -> ${TEST} " 
  fi


# fruit test, without compression 
printf "%-50s" "fruit.ans test, without compression ... "
TEST=`./ansiread -c ansifiles/fruit.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "e5b885845167d58db0fd747a363ad1ac" ]; then 
    echo -e $MSG_PASS
  else
    echo -e "$MSG_FAIL -> ${TEST} " 
	fi

# fruit test, with compression
printf "%-50s" "fruit.ans test, with compression ... "
TEST=`./ansiread -z -c ansifiles/fruit.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "cf7c91b714fde086a748911c34598f94" ]; then 
    echo -e $MSG_PASS
  else
    echo -e "$MSG_FAIL -> ${TEST} " 
	fi

# fruit PNG render, direct
printf "%-50s" "fruit.ans PNG, direct render test ... "
./ansiread -o fruit.png ansifiles/fruit.ans 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" fruit.png`
if [ $TEST == "7a056e3280526815af436cd16294d5c7b915def074635752a7cf68416bf16a31" ]; then
		echo -e $MSG_PASS
		cp fruit.png tests/ansiread/pass/fruit.1.png
		rm -f fruit.png
  else
    echo -e "$MSG_FAIL -> ${TEST} " 
  fi

# fruit PNG re-render, stdin, uncompressed 
printf "%-50s" "fruit.ans PNG, stdin/uncompressed test ... "
./ansiread -c ansifiles/fruit.ans 2>/dev/null | ./ansiread -o fruit.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" fruit.png`
if [ $TEST == "7a056e3280526815af436cd16294d5c7b915def074635752a7cf68416bf16a31" ]; then
    echo -e $MSG_PASS
    cp fruit.png tests/ansiread/pass/fruit.2.png
    rm -f fruit.png
  else
    echo -e "$MSG_FAIL -> ${TEST} " 
  fi

# fruit PNG re-render, stdin, compressed 
printf "%-50s" "fruit.ans PNG, stdin/compressed test ... "
./ansiread -z -c ansifiles/fruit.ans 2>/dev/null | ./ansiread -o fruit.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" fruit.png`
if [ $TEST == "7a056e3280526815af436cd16294d5c7b915def074635752a7cf68416bf16a31" ]; then
    echo -e $MSG_PASS
    cp fruit.png tests/ansiread/pass/fruit.3.png
    rm -f fruit.png
  else
    echo -e "$MSG_FAIL -> ${TEST} " 
  fi

echo ""

printf "%-50s" "timebend.ans should always have 26 lines ...  "
TEST=`./ansiread -c ansifiles/timebend.ans 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | wc -l`
if [ $TEST == "26" ]; then
    echo -e $MSG_PASS
  else
		echo -e "$MSG_FAIL -> ${TEST} "
  fi


# timebend test, without compression 
printf "%-50s" "timebend.ans test, without compression ... "
TEST=`./ansiread -c ansifiles/timebend.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "58dae962396588abc901c37591bf84a6" ]; then
    echo -e $MSG_PASS
  else
    echo -e "$MSG_FAIL -> ${TEST} "
  fi

# timebend test, with compression
printf "%-50s" "timebend.ans test, with compression ... "
TEST=`./ansiread -z -c ansifiles/timebend.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "18a5c85c274aabd497701f3e370a2da1" ]; then
    echo -e $MSG_PASS
  else
    echo -e "$MSG_FAIL -> ${TEST} "
  fi

# timebend PNG render, direct
printf "%-50s" "timebend.ans PNG, direct render test ... "
./ansiread -o timebend.png ansifiles/timebend.ans 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" timebend.png`
if [ $TEST == "8f23c862c4b21a2fc7196e32e60c93d6a811ed51b8dc7cab00108155bf1ab372" ]; then
    echo -e $MSG_PASS
    cp timebend.png tests/ansiread/pass/timebend.1.png
		rm -f timebend.png
  else
		echo -e "$MSG_FAIL -> ${TEST} "
  fi

# timebend PNG re-render, stdin, uncompressed 
printf "%-50s" "timebend.ans PNG, stdin/uncompressed test ... "
./ansiread -c ansifiles/timebend.ans 2>/dev/null | ./ansiread -o timebend.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" timebend.png`
if [ $TEST == "8f23c862c4b21a2fc7196e32e60c93d6a811ed51b8dc7cab00108155bf1ab372" ]; then
    echo -e $MSG_PASS
    cp timebend.png tests/ansiread/pass/timebend.2.png
    rm -f timebend.png
  else
    echo -e "$MSG_FAIL -> ${TEST} "
  fi

# timebend PNG re-render, stdin, compressed 
printf "%-50s" "timebend.ans PNG, stdin/compressed test ... "
./ansiread -z -c ansifiles/timebend.ans 2>/dev/null | ./ansiread -o timebend.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" timebend.png`
if [ $TEST == "8f23c862c4b21a2fc7196e32e60c93d6a811ed51b8dc7cab00108155bf1ab372" ]; then
    echo -e $MSG_PASS
    cp timebend.png tests/ansiread/pass/timebend.3.png
    rm -f timebend.png
  else
    echo -e "$MSG_FAIL -> ${TEST} "
  fi


# for later ... image comparison of contents (ignore metadata)
# identify -quiet -format "%# " timebend1.png timebend2.png


echo ""
