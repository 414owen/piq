#!/usr/bin/env bash

set -euo pipefail

shopt -s extglob

GLOBIGNORE="src/tokenizer.c:src/parser.c" 
handwritten_c_files="$(echo src/*.c)"

GLOBIGNORE="src/parser.h" 
handwritten_h_files="$(echo src/*.h)"
