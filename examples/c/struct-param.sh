#!/usr/bin/env bash

if [ "$1" = "" ]; then
  echo "Please supply a depth"
  exit 1
fi

echo "struct s { int a; int b; };"
echo "int getb(struct s a) { return a.b; }"

echo "int main (void) {"
echo " int a0 = 2;"
for i in $(seq $1); do
  pred=$((i - 1))
  echo " struct s p${i} = {.a = 1, .b = a${pred}};"
  echo " int a${i} = getb(p${i});"
done
echo " return a${1};"
echo "}"
