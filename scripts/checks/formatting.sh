#!/usr/bin/env bash

set -euxo pipefail

for i in *.c; do clang-format -n --Werror $i; done
