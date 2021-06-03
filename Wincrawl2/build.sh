#!/bin/zsh
setopt null_glob
set -eo pipefail
IFS=$'\n\t'

if [ "$1" = "--watch" ]; then
	ls wincrawl | entr -npr ./wincrawl &
	ls *.cpp *.hpp | entr -cn $0
else
	g++ Wincrawl2.cpp -std=c++20 -Wall -o wincrawl -g -lpthread
fi