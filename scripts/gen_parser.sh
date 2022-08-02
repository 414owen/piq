lemon -c src/parser.y -q
sed -i 's/^static \(const char \*.*yyTokenName\[\].*\)$/\1/g' src/parser.c
sed -i 's/assert(/debug_assert(/g' src/parser.c
./scripts/add-pragmas.sh src/parser.h
