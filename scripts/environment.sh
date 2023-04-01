#!/bin/sh

# This is to be run for non-nix builds
set +x

getBest() {
  var="$1"
  old="$(eval "echo \$${var}")"
  if [ ! -z "${old}" ]; then
    echo "Already have ${var} : ${old}"
    return 0
  fi
  echo "Finding a value for ${var}"
  shift
  for c in $@; do
    if [ -x "$(command -v $c)" ]; then
      export "${var}"="${c}"
      echo "Found ${c}"
      return 0
    fi
  done
  echo "Error: None of $@ installed"
  exit 1
}

getBest CC cc gcc clang
getBest CXX c++ g++ clang++
getBest LD mold lld gold ld
export READLINE_LIBS=`pkg-config --libs "readline"`
export READLINE_CFLAGS=`pkg-config --cflags "readline"`
export LLVM_CFLAGS=`llvm-config --cflags --libs core executionengine mcjit engine analysis native bitwriter --system-libs`
export LLVM_LDFLAGS=`llvm-config --ldflags --libs core executionengine mcjit engine analysis native bitwriter --system-libs`
