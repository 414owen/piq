#!/usr/bin/env bash

if [ $# = 0 ]; then
  echo "Usage: $0 <binary> <args>"
  exit 1
fi

dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"

outdir=build-flamegraph

# All the optimization options are in Tupfile anyway
export CFLAGS="-g"
cmake -B "$outdir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$outdir"

perf record -F 26000 --call-graph dwarf "${outdir}/$@" >&2
stackpath="$(mktemp)"
perf script | stackcollapse-perf.pl > "${stackpath}"
flamegraph.pl "${stackpath}" > flamegraph.svg
rm "${stackpath}"
