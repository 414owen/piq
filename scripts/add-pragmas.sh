#!/bin/sh

. $(dirname $0)/globs.sh

nl='
'

fixup() {
	if ! grep 'pragma once' $1; then
		echo "#pragma once${nl}${nl}$(cat $1)" > $1
	fi
}

if [ $# -gt 0 ]; then
	for i in $@; do
	  fixup "${i}"
	done
else
	for i in $handwritten_h_files; do
		fixup $i
	done
fi
