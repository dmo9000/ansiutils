clear
echo -en "\x1b\x5b""15;1H"
echo "PLAIN SSR - HIT ENTER WHEN READY"
read ENTER

clear
echo -en "\x1b\x5b""10;1H"
echo -e -n "A\x1b\x5b\x64B"

echo -en "\x1b\x5b""15;1H"
echo "PARAMETERIZED SSR - HIT ENTER WHEN READY"
read ENTER

clear
echo -en "\x1b\x5b""10;1H"
echo -e -n "A\x1b\x5b\x31\x64B"
