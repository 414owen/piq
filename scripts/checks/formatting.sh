#!/usr/bin/env bash

set -uo pipefail
shopt -s extglob

MY_PATH=$(dirname "$0")
source "${MY_PATH}/../globs.sh"

num_files_checked=0
num_checks_failed=0
for i in $handwritten_c_files; do
  num_files_checked=$((num_files_checked + 1))
  clang-format -n --Werror $i;
  if [ $? != 0 ]; then
    num_checks_failed=$((num_checks_failed + 1))
  fi
done

echo "Checked formatting of $num_files_checked files"

if [ $num_checks_failed = 0 ]; then
  echo "All checks passed"
else
  echo "$num_checks_failed/$num_files_checked files weren't formatted"
  exit 1
fi
