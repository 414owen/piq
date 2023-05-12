#!/usr/bin/env bash

set -euo pipefail
shopt -s extglob

MY_PATH=$(dirname "$0")
source "${MY_PATH}/../globs.sh"

fail="false"


LICENCE="// Copyright 2023 The piq Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file."

LINES=$(wc -l <(echo "$LICENCE"))

num_files_checked=0
num_checks_failed=0
for i in $handwritten_c_files $handwritten_h_files; do
  num_files_checked=$((num_files_checked + 1))
  if ! cmp --silent <(cat $i | head -n 3) <(echo "$LICENCE") > /dev/null; then
    echo "Top of file ${i} doesn't match desired licence."
    echo "Please copy-paste this to the top:"
    echo "${LICENCE}"
    num_checks_failed=$((num_checks_failed + 1))
  fi
done

echo "Checked $num_files_checked files for licenses"

if [ $num_checks_failed = 0 ]; then
  echo "All checks passed"
else
  echo "$num_checks_failed/$num_files_checked files weren't formatted"
  exit 1
fi
