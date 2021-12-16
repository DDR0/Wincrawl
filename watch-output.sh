echo Run this script, then run ./wincrawl 2\>wincrawlerr.log to get debug output.
mkfifo wincrawlerr.log 2> /dev/null || true
tail -f wincrawlerr.log | nl -w3 -s': ' -