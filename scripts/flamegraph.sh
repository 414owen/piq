#!/usr/bin/env bash

if [ $# = 0 ]; then
  echo "Usage: $0 <binary> <args>"
  exit 1
fi

dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
. ./release-env.sh

# All the optimization options are in Tupfile anyway
CFLAGS="-g -DNDEBUG"
tup

perf record -F 26000 --call-graph dwarf $@ >&2
stackpath="$(mktemp)"
perf script | stackcollapse-perf.pl > "${stackpath}"
flamegraph.pl "${stackpath}" > flamegraph.svg
rm "${stackpath}"
