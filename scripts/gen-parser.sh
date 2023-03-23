#!/bin/sh

set -e

lemon -c src/parser.y -q
sed -i 's/^static \(const char \*.*yyTokenName\[\].*\)$/\1/g
  ; /defined(YYCOVERAGE) || !defined(NDEBUG)/d
  ; s/assert(/debug_assert(/g' src/parser.c
tmp="$(mktemp)"
mv src/parser.c "$tmp"
echo "#include <hedley.h>" > src/parser.c
echo "HEDLEY_DIAGNOSTIC_PUSH" >> src/parser.c
echo '#pragma GCC diagnostic ignored "-Wunused-variable"' >> src/parser.c
echo '#pragma GCC diagnostic ignored "-Wunused-parameter"' >> src/parser.c
echo '#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"' >> src/parser.c

LINES_ADDED=4
# Add to every self-reference line
cat "$tmp" \
  | awk "{if (/#line [0-9]+ \"src\/parser.c\"/) {print \$1,(\$2 + ${LINES_ADDED}),\$3} else {print \$0}}" \
  >> src/parser.c

echo "HEDLEY_DIAGNOSTIC_POP" >> src/parser.c
rm $tmp

/bin/sh $(dirname "$0")/add-pragmas.sh src/parser.h
