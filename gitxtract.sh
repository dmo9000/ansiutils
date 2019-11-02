SUMMARY_COUNT=5

# get SUMMARY_COUNT last commits

SUMMARY_TAGS=`git rev-list HEAD | head -n ${SUMMARY_COUNT}` 

FMT=`which gfmt 2>/dev/null`
# On FreeBSD, all GNU utils from coreutils have a 'g' prefix
if [ -z "${FMT}" ]; then
	FMT=`which fmt`
	fi

for COMMIT in $SUMMARY_TAGS; do 
	SHORTHASH=`echo ${COMMIT} | cut -b1-7`
	#COMMENT=`git log --no-walk ${COMMIT} | grep "^ " | sed "s/^\ */\* /" | fold -s -w 90 | ${FMT} -t | sed "s/^  /         /g"`
	COMMENT=`git log --no-walk ${COMMIT} | grep "^ " | paste -s -d " " | tr -s " " | sed -e "s/^\ *//" | fmt -w 90 | fmt -w 90 -t | sed '/^*\ /! s/\(.*\)/\^\1/'`

	ANSI_FG_RED=`./ansitext FG_RED`
	ANSI_FG_NONE=`./ansitext FG_NONE`
	echo "${ANSI_FG_RED}${SHORTHASH}${ANSI_FG_NONE} ${COMMENT}" | sed "s/^\^/          /" | sed "s/\^/\* /g" 
	done


#git log --no-walk `git rev-list HEAD | head -n 10` | grep  "^ " | sed "s/^\ */\* /"
