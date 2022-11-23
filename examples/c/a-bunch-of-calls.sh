#!/usr/bin/env bash

if [ "$1" = "" ]; then
  echo "Please supply a depth"
  exit 1
fi

echo "int id(int a) { return a; }"

echo "int main (void) {"
echo " int a0 = 2;"
for i in $(seq $1); do
  pred=$((i - 1))
  echo " int a${i} = id(a${i});"
done
echo " return a${1};"
echo "}"
