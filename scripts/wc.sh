#!/usr/bin/env bash

MY_PATH=$(dirname "$0")
source "${MY_PATH}/globs.sh"

echo ".c lines: $(cat ${handwritten_c_files} | wc -l)"
echo ".h lines: $(cat ${handwritten_h_files} | wc -l)"
echo "All lines: $(cat ${handwritten_h_files} ${handwritten_c_files} | wc -l)"
