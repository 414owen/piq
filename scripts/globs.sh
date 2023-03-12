#!/bin/sh

GLOBIGNORE="src/tokenizer.c\nsrc/parser.c" 
handwritten_c_files=""
for i in src/*.c; do
  if ! echo "$GLOBIGNORE" | grep "$i" > /dev/null; then
    handwritten_c_files="${handwritten_c_files} $i"
  fi
done

GLOBIGNORE="src/parser.h" 
handwritten_h_files=""
for i in src/*.h; do
  if ! echo "$GLOBIGNORE" | grep "$i" > /dev/null; then
    handwritten_h_files="${handwritten_h_files} $i"
  fi
done
