#!/bin/sh

PATH=$PATH:$HOME/.local/bin:$HOME/bin
PATH=~/.npm-global/bin:~/.cargo/bin:$PATH
export PATH
export GOPATH=/home/dan/git-remote
#cat $1 | \
./tdftool $@ | \
#    go run /home/dan/git-remote/src/ansiart2utf8/ansiart2utf8.go -w 78 
		iconv -f CP437 -t UTF8 


