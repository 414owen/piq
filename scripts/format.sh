#!/usr/bin/env bash

GLOBIGNORE="tokenizer.c:parser.c:parser.h" 
for i in *.c; do clang-format -i $i; done
