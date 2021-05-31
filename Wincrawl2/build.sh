#!/bin/zsh
setopt null_glob
set -eo pipefail
IFS=$'\n\t'

if [ "$1" = "--watch" ]; then
	echo wincrawl | entr -npr ./wincrawl &
	echo *.cpp *.hpp | entr -c $0
else
	g++ Wincrawl2.cpp -std=c++20 -Wall -o wincrawl -g
fi