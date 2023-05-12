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

for i in $handwritten_c_files $handwritten_h_files; do
  if ! cmp --silent <(cat $i | head -n 3) <(echo "$LICENCE") > /dev/null; then
    echo "Top of file ${i} doesn't match desired licence."
    echo "Please copy-paste this to the top:"
    echo "${LICENCE}"
    fail=true
  fi
done 

if [ "$fail" = "true" ]; then
  exit 1
fi
