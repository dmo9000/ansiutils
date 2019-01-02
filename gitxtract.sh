SUMMARY_COUNT=5
SUMMARY_TAGS=`git rev-list HEAD | head -n ${SUMMARY_COUNT}` 

FMT=`which gfmt`
# On FreeBSD, all GNU utils from coreutils have a 'g' prefix
if [ -z "${FMT}" ]; then
	FMT=`which fmt`
	fi

for COMMIT in $SUMMARY_TAGS; do 
	SHORTHASH=`echo ${COMMIT} | cut -b1-7`
	COMMENT=`git log --no-walk ${COMMIT} | grep "^ " | sed "s/^\ */\* /" | fold -s -w 90 | ${FMT} -t | sed "s/^  /         /g"`
	echo "${SHORTHASH} ${COMMENT}" | sed "s/^\*/        \*/" 
	done


#git log --no-walk `git rev-list HEAD | head -n 10` | grep  "^ " | sed "s/^\ */\* /"
