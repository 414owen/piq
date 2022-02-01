#!/usr/bin/env bash

set -euxo pipefail
shopt -s extglob

MY_PATH=$(dirname "$0")
source "${MY_PATH}/../globs.sh"

for i in $handwritten_c_files;  do clang-format -n --Werror $i; done
