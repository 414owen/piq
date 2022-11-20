#!/bin/sh

export READLINE_LIBS=`pkg-config --libs "readline"`
export READLINE_CFLAGS=`pkg-config --cflags "readline"`
export LLVM_CONFIG_FLAGS=`llvm-config --cxxflags --ldflags --libs core executionengine mcjit engine analysis native bitwriter --system-libs`
