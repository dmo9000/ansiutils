#!/bin/sh

# fruit test, without compression 
echo -n "fruit.ans test, without compression ... "
TEST=`./ansiread -c ansifiles/fruit.ans 2>/dev/null | md5sum - | awk '{ print $1; }'`
if [ $TEST == "5c75c39faec8702f154d6f711af226ec" ]; then 
		echo "OK"
	else
	 	echo "FAIL -> ${TEST} " 
	exit 1  
	fi

# fruit test, with compression
echo -n "fruit.ans test, with compression ... "
TEST=`./ansiread -z -c ansifiles/fruit.ans 2>/dev/null | md5sum - | awk '{ print $1; }'`
if [ $TEST == "13f407c7449784c5674808abd7193bdd" ]; then 
		echo "OK"
	else
	 	echo "FAIL -> ${TEST} " 
	exit 1  
	fi

