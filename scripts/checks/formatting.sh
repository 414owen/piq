#!/usr/bin/env bash

set -euxo pipefail
shopt -s extglob

GLOBIGNORE="tokenizer.c:parser.c:parser.h" 
for i in *.c; do clang-format -n --Werror $i; done
