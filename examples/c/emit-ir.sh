#!/usr/bin/env bash

clang -S -O0 -emit-llvm -o /dev/stdout -x c -
