#!/usr/bin/env bash

MY_PATH=$(dirname "$0")
source "${MY_PATH}/globs.sh"

for i in $handwritten_c_files; do
  clang-format -i $i;
done
