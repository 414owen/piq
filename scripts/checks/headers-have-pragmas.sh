#!/usr/bin/env bash

set -euxo pipefail

fail="false"

for i in *.h; do
  if ! cat $i | grep 'pragma once' > /dev/null; then
    echo "Header file '$i' doesn't have a pragma once."
    fail=true
  fi
done 

if [ "$fail" = "true" ]; then
  exit 1
fi