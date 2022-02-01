#!/usr/bin/env bash

set -euxo pipefail

shopt -s extglob

GLOBIGNORE="tokenizer.c:parser.c" 
handwritten_c_files="$(echo *.c)"

GLOBIGNORE="parser.h" 
handwritten_h_files="$(echo *.h)"
