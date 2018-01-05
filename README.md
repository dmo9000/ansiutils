# TDFTool

A program for rendering TheDraw fonts to the terminal. 

    https://en.wikipedia.org/wiki/TheDraw

The largest collection of TDF fonts that I know of are available at:

    http://www.roysac.com/thedrawfonts-tdf.html

## Usage

After running make, and assuming you have Roy's TDF files unpacked to ../THEDRAWFONTS, run:

Run:

    ./tdftool ../THEDRAWFONTS/DUNGEON2.TDF HELLO 

There is a quick and dirty test suite that can be run with:

    make test

Currently it only outputs the fonts in the original Codepage 437 format. I'm working 
on Unicode support in the future, however for now, if you need them to display
on a UNIX/Linux terminal, rather than on DOS you'll need to convert them with iconv, eg. 

    ./tdftool ../THEDRAWFONTS/DUNGEON2.TDF HELLO | iconv -f CP437 -t UTF8 

To display them on Windows there seems to be a few solutions: 

    https://github.com/peteri/Ansi-Cat

    https://github.com/adoxa/ansicon/

And of course you should be able to load them into PabloDraw as an .ANS file. 

This utility can render over 1,000 of them, although the accuracy of the results
has not been completely verified. I'm working on resolving the issues with the ~120
fonts or so that this can't render yet. Accordingly, if you find a TDF file that it
cannot render, please provide a link to the file and let me know about it, hopefully
I can fix it.  

Current only TYPE_BLOCK and TYPE_COLOR fonts are supported. TYPE_OUTLINE will be 
hopefully be supported in a future version. Unfortunately there are not a lot of
TYPE_OUTLINE fonts in the collection (I only count four). 

This should be considered very early alpha code, do not use anywhere near production. 
There are still assert() macros all over the place to help with debugging and there
are some known issues with realloc() here and there. I will fix these as I can get
time to work on them. 

Many thanks to Roy for collecting the fonts and providing workable specifications for the
font format on his site. 
 

