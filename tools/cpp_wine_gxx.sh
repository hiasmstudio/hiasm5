#!/bin/sh
set -eu
exec sh "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)/cpp_wine_tool.sh" compiler "$@"
