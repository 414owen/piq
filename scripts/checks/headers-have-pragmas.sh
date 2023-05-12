#!/usr/bin/env bash

set -euo pipefail

fail="false"

num_files_checked=0
num_checks_failed=0
for i in src/*.h; do
  num_files_checked=$((num_files_checked + 1))
  if ! cat $i | grep 'pragma once' > /dev/null; then
    echo "Header file '$i' doesn't have a pragma once."
    num_checks_failed=$((num_checks_failed + 1))
  fi
done

echo "Checked pragmas of $num_files_checked files"

if [ $num_checks_failed = 0 ]; then
  echo "All checks passed"
else
  echo "$num_checks_failed/$num_files_checked headers didn't have '#pragma once'"
  exit 1
fi
