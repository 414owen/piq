#!/usr/bin/env bash

set -euxo pipefail

if ! grep 'pragma once' parser.h; then
	echo -e "#pragma once\n$(cat parser.h)" > parser.h
fi
