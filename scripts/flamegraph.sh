#!/usr/bin/env bash

perf record --call-graph dwarf $@ >&2
stackpath="$(mktemp)"
perf script | stackcollapse-perf.pl > "${stackpath}"
flamegraph.pl "${stackpath}" > flamegraph.svg
rm "${stackpath}"
