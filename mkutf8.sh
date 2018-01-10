
for ANS in `ls -1 *.ans`; do 
		cat $ANS | iconv -f CP437 -t UTF8 > utf8/$ANS
		done
