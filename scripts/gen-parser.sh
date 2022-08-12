#!/bin/sh

set -e

lemon -c src/parser.y -q
sed -i 's/^static \(const char \*.*yyTokenName\[\].*\)$/\1/g' src/parser.c
sed -i 's/assert(/debug_assert(/g' src/parser.c
/bin/sh $(dirname "$0")/add-pragmas.sh src/parser.h
