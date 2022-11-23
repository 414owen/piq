#!/usr/bin/env bash

if [ "$1" = "" ]; then
  echo "Please supply a depth"
  exit 1
fi

echo "(sig snd (Fn (I32, I32) I32))"
echo "(fun snd (a, b) b)"

echo "(sig test (Fn () I32))"
echo "(fun test () "

for i in `seq $1`; do
  echo " (snd (1, "
done
echo -n "  2"
for i in `seq $1`; do
  echo -n "))"
done
echo ")"
