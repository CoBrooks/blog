#!/usr/bin/env bash

set -eu pipefail

if [ -f out.lua ]; then
    rm out.lua
fi

for url in $(cat list.txt); do
    curl $url | ./generate.c | sort -r | head -n3 >> out.lua
done

sort -ro out.lua out.lua
