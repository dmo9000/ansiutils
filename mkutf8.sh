mkdir -p tests/pass/utf8

for ANS in `ls -1 tests/pass/*.ans`; do 
		BASENAME=`basename $ANS`
		cat $ANS | iconv -f CP437 -t UTF-8 > tests/pass/utf8/$BASENAME
		done