#!/bin/sh

MD5SUM=`which gmd5sum 2>/dev/null | sed -e "s/^no.*$//g" | tr -d "\n"`
if [ -z "${MD5SUM}" ] ; then
	MD5SUM=`which md5sum 2>/dev/null`
	fi




echo ""

MSG_PASS=`./ansitext FG_GREEN BOLD "[PASS]" FG_NONE NONE NEWLINE`
MSG_FAIL=`./ansitext FG_RED BOLD "[FAIL]" FG_NONE NONE NEWLINE`

printf "%-50s" "TDFTool UTF8 rendering test (BOARDX) ..."
TEST=`./tdftool -f 1 THEDRAWFONTS/BOARDX.TDF ANSIUTILS | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "573727da4dd42a6edeff1f8bf251805a" ]; then 
    printf "%s\n" "$MSG_PASS -> ${TEST}"
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST}" 
  fi

printf "%-50s" "TDFTool CP437 rendering test (BOARDX) ..."
TEST=`./tdftool -f 1 -c THEDRAWFONTS/BOARDX.TDF ANSIUTILS | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "8059e156bc874feff6b61a12e0eb6e88" ]; then 
    printf "%s\n" "$MSG_PASS -> ${TEST}"
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST}" 
  fi


echo ""

printf "%-50s" "fruit.ans should have MD5 signature  ...  "
TEST=`${MD5SUM} ansifiles/fruit.ans 2>/dev/null | awk '{ print $1; }'`
if [ $TEST == "29d0681a96d72e364222958b36b36ca5" ]; then 
    printf "%s\n" "$MSG_PASS -> ${TEST}"
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST}" 
  fi


printf "%-50s" "fruit.ans should always have 24 lines ...  "
TEST=`./ansiread -c ansifiles/fruit.ans 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | wc -l`
if [ $TEST == "24" ]; then 
    printf "%s\n" "$MSG_PASS -> ${TEST}"
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} " 
  fi


# fruit test, without compression 
printf "%-50s" "fruit.ans test, without compression ... "
TEST=`./ansiread -c ansifiles/fruit.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "923c984036d37cdd3208e9fb8fa55e79" ]; then 
    printf "%s\n" "$MSG_PASS -> ${TEST}"
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} " 
	fi

# fruit test, with compression
printf "%-50s" "fruit.ans test, with compression ... "
TEST=`./ansiread -z -c ansifiles/fruit.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "f6da036e8e5266b8a92473916ace99df" ]; then 
    printf "%s\n" "$MSG_PASS -> ${TEST}"
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} " 
	fi

# fruit PNG render, direct
printf "%-50s" "fruit.ans PNG, direct render test ... "
./ansiread -o fruit.png ansifiles/fruit.ans 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" fruit.png | md5sum - | cut -f1 -d ' '`
if [ $TEST == "0a2bedc1fd419f92dd73773a0f75006c" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST}"
		cp fruit.png tests/ansiread/pass/fruit.1.png
		rm -f fruit.png
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} " 
  fi

# fruit PNG re-render, stdin, uncompressed 
printf "%-50s" "fruit.ans PNG, stdin/uncompressed test ... "
./ansiread -c ansifiles/fruit.ans 2>/dev/null | ./ansiread -o fruit.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" fruit.png | md5sum - | cut -f1 -d ' '`
if [ $TEST == "0a2bedc1fd419f92dd73773a0f75006c" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST}"
    cp fruit.png tests/ansiread/pass/fruit.2.png
    rm -f fruit.png
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} " 
  fi

# fruit PNG re-render, stdin, compressed 
printf "%-50s" "fruit.ans PNG, stdin/compressed test ... "
./ansiread -z -c ansifiles/fruit.ans 2>/dev/null | ./ansiread -o fruit.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" fruit.png | md5sum - | cut -f1 -d ' '`
if [ $TEST == "0a2bedc1fd419f92dd73773a0f75006c" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST}"
    cp fruit.png tests/ansiread/pass/fruit.3.png
    rm -f fruit.png
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} " 
  fi

echo ""

printf "%-50s" "timebend.ans should have MD5 signature  ...  "
TEST=`${MD5SUM} ansifiles/timebend.ans 2>/dev/null | awk '{ print $1; }'`
if [ $TEST == "b098be5f4529d33d835f126d704f7eb5" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST}"
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST}"
  fi

printf "%-50s" "timebend.ans should always have 26 lines ...  "
TEST=`./ansiread -c ansifiles/timebend.ans 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | ./ansiread -c - 2>/dev/null | wc -l`
if [ $TEST == "26" ]; then
		printf "%s\n" "$MSG_PASS -> ${TEST} "
  else
		printf "%s\n" "$MSG_FAIL -> ${TEST} "
  fi


# timebend test, without compression 
printf "%-50s" "timebend.ans test, without compression ... "
TEST=`./ansiread -c ansifiles/timebend.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "430c8b0ebf958ee34d671d6d27f31778" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST} "
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} "
  fi

# timebend test, with compression
printf "%-50s" "timebend.ans test, with compression ... "
TEST=`./ansiread -z -c ansifiles/timebend.ans 2>/dev/null | ${MD5SUM} - | awk '{ print $1; }'`
if [ $TEST == "6105c3b4b077f73872df86e3744ceab9" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST} "
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} "
  fi

# timebend PNG render, direct
printf "%-50s" "timebend.ans PNG, direct render test ... "
./ansiread -o timebend.png ansifiles/timebend.ans 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" timebend.png | md5sum - | cut -f1 -d ' '`
if [ $TEST == "67618531f8e50536008a05930803f0ec" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST} "
    cp timebend.png tests/ansiread/pass/timebend.1.png
		rm -f timebend.png
  else
		printf "%s\n" "$MSG_FAIL -> ${TEST} "
  fi

# timebend PNG re-render, stdin, uncompressed 
printf "%-50s" "timebend.ans PNG, stdin/uncompressed test ... "
./ansiread -c ansifiles/timebend.ans 2>/dev/null | ./ansiread -o timebend.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" timebend.png | md5sum - | cut -f1 -d ' '`
if [ $TEST == "67618531f8e50536008a05930803f0ec" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST} "
    cp timebend.png tests/ansiread/pass/timebend.2.png
    rm -f timebend.png
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} "
  fi

# timebend PNG re-render, stdin, compressed 
printf "%-50s" "timebend.ans PNG, stdin/compressed test ... "
./ansiread -z -c ansifiles/timebend.ans 2>/dev/null | ./ansiread -o timebend.png - 1>/dev/null 2>&1
TEST=`identify -quiet -format "%#" timebend.png | md5sum - | cut -f1 -d ' '`
if [ $TEST == "67618531f8e50536008a05930803f0ec" ]; then
    printf "%s\n" "$MSG_PASS -> ${TEST} "
    cp timebend.png tests/ansiread/pass/timebend.3.png
    rm -f timebend.png
  else
    printf "%s\n" "$MSG_FAIL -> ${TEST} "
  fi


# for later ... image comparison of contents (ignore metadata)
# identify -quiet -format "%# " timebend1.png timebend2.png


echo ""
