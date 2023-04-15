#!/bin/sh

# We stage files in tmp directories, because one of our build systems
# seems to start the next stage as soon as the output files have been
# created

infile="$1"
outdir="$2"
outcfile="${outdir}/parser.c"
outhfile="${outdir}/parser.h"

set -xe

TMPDIR="${TMPDIR:=/tmp}"
tmpdir="$(mktemp -d -p "$TMPDIR")"

tmpcfile="${tmpdir}/parser.c"
tmpcfile2="${tmpdir}/parser.c.2"
tmphfile="${tmpdir}/parser.h"

lemon -c "$infile" -d"$tmpdir" -q

echo "#include <hedley.h>" > "$tmpcfile2"
echo "HEDLEY_DIAGNOSTIC_PUSH" >> "$tmpcfile2"
echo '#pragma GCC diagnostic ignored "-Wunused-variable"' >> "$tmpcfile2"
echo '#pragma GCC diagnostic ignored "-Wunused-parameter"' >> "$tmpcfile2"
echo '#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"' >> "$tmpcfile2"

LINES_ADDED=5
# Add to every self-reference line
cat "$tmpcfile" \
  | sed 's/^static \(const char \*.*yyTokenName\)/\1/
    ; /defined(YYCOVERAGE) || !defined(NDEBUG)/d
    ; s/assert(/debug_assert(/g' \
  | awk "{if (/#line [0-9]+ \"src\/parser.c\"/) {print \$1,(\$2 + ${LINES_ADDED}),\$3} else {print \$0}}" \
  >> "$tmpcfile2"

echo "HEDLEY_DIAGNOSTIC_POP" >> "$tmpcfile2"

cat "$tmpcfile2"

/bin/sh $(dirname "$0")/add-pragmas.sh "$tmphfile"

mv "$tmphfile" "${outhfile}"
mv "$tmpcfile2" "${outcfile}"

rm -r $tmpdir
