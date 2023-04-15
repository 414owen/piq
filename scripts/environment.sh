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

get_llvm_config() {
  if [ -x "$(command -v llvm-config)" ]; then
    LLVM_CONFIG=llvm-config
    return 0
  fi
  version=9;
  while true; do
    if [ $version -gt 30 ]; then
      return 1
    fi
    found=false
    if [ -x "$(command -v llvm-config-$version)" ]; then
      LLVM_CONFIG=llvm-config-$version
      found=true
    elif [ $found = "true" ]; then
      return 0
    fi
    version=$((version + 1))
  done
}

if [ -z "$USER_LLVM_CONFIG" ]; then
  get_llvm_config
else
  LLVM_CONFIG="$USER_LLVM_CONFIG"
fi

export READLINE_LIBS=`pkg-config --libs "readline"`
export READLINE_CFLAGS=`pkg-config --cflags "readline"`
export LLVM_CFLAGS=`$LLVM_CONFIG --cflags core executionengine mcjit engine analysis native bitwriter --system-libs`
export LLVM_LDFLAGS=`$LLVM_CONFIG --ldflags --libs core executionengine mcjit engine analysis native bitwriter --system-libs`
