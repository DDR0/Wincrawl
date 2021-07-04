#!/bin/zsh
setopt null_glob
set -eo pipefail
IFS=$'\n\t'

if [ "$1" = "--watch" ]; then
	ls wincrawl | entr -npr ./wincrawl &
	ls *.c* *.h* | entr -cn $0
elif [ "$1" = "--debug" ]; then
	$0 && gdb -ex run -tui wincrawl
else
	g++ *.c* -std=c++20 -Wall -o wincrawl -g -lpthread
fi