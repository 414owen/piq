#!/bin/sh

export EDITLINE_LIBS=`pkg-config --libs "libeditline"`
export EDITLINE_CFLAGS=`pkg-config --cflags "libeditline"`
export LLVM_CONFIG_FLAGS=`llvm-config --cxxflags --ldflags --libs core executionengine mcjit engine analysis native bitwriter --system-libs`
