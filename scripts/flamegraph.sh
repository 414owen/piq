#!/usr/bin/env bash

if [ $# = 0 ]; then
  echo "Usage: $0 <binary> <args>"
  exit 1
fi

perf record -F 26000 --call-graph dwarf $@ >&2
stackpath="$(mktemp)"
perf script | stackcollapse-perf.pl > "${stackpath}"
flamegraph.pl "${stackpath}" > flamegraph.svg
rm "${stackpath}"
