#!/usr/bin/env bash

set -euxo pipefail

function fixup() {
	if ! grep 'pragma once' $1; then
		echo -e "#pragma once\n\n$(cat $1)" > $1
	fi
}

if [[ $# -gt 0 ]]; then
	for ((i=1; i<=$#; i++)); do
	  fixup "${!i}"
	done
else
	for i in src/*.h; do
		fixup $i
	done
fi
