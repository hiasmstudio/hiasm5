#!/bin/bash
set -euo pipefail

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

fpc "${SCRIPT_DIR}/CGTShare.pas" -Mdelphi -O1 -XX
fpc "${SCRIPT_DIR}/CodeGen.dpr" -Mdelphi -O1 -XX

# add all paths you heed here?
#cp "${SCRIPT_DIR}/libCodeGen.so" "${SCRIPT_DIR}/../web/libCodeGen.so"
