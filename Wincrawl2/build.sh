#!/bin/zsh
setopt null_glob
set -eo pipefail
IFS=$'\n\t'

if [ "$1" = "--watch" ]; then
	ls wincrawl | entr -npr ./wincrawl &
	ls *.c* *.h* | entr -cn $0
else
	g++ *.c* -std=c++20 -Wall -o wincrawl -g -lpthread
fi