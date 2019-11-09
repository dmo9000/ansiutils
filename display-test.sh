#!/bin/sh

echo "" > visual-results.csv

for ANS in `ls -1 tests/pass/utf8/*.ans`; do
    echo $ANS
    cat $ANS
		ANSFILE=`basename $ANS`
    echo ""
		echo "Rendered correctly? [y/n]"
    read INPUT

		case ${INPUT} in
        [yY] | [yY][Ee][Ss] )
								echo "Y,${ANSFILE}" >> visual-results.csv
                ;;
        [nN] | [n|N][O|o] )
								echo "N,${ANSFILE}" >> visual-results.csv
                ;;
        *) echo "Invalid input"
            ;;
		esac
done


