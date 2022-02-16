#!/usr/bin/env bash

set -euxo pipefail

for i in *.h; do
	if ! grep 'pragma once' $i; then
		echo -e "#pragma once\n\n$(cat $i)" > $i
	fi
done
