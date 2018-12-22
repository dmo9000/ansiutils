SUMMARY_COUNT=5
SUMMARY_TAGS=`git rev-list HEAD | head -n ${SUMMARY_COUNT}` 

for COMMIT in $SUMMARY_TAGS; do 
	SHORTHASH=`echo ${COMMIT} | cut -b1-7`
	COMMENT=`git log --no-walk ${COMMIT} | grep "^ " | sed "s/^\ */\* /" | fold -s -w 90 | fmt -t | sed "s/^  /         /g"`
	echo "${SHORTHASH} ${COMMENT}" | sed "s/^\*/        \*/" 
	done


#git log --no-walk `git rev-list HEAD | head -n 10` | grep  "^ " | sed "s/^\ */\* /"
