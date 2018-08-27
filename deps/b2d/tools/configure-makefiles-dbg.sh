#!/bin/sh

CURRENT_DIR=`pwd`
BUILD_DIR="build_dbg"

if [ -z "${ASMJIT_DIR}" ]; then
  ASMJIT_DIR="../../asmjit"
fi

mkdir -p ../${BUILD_DIR}
cd ../${BUILD_DIR}

cmake .. -G"Unix Makefiles" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DASMJIT_DIR="${ASMJIT_DIR}" \
  -DB2D_BUILD_TEST=1 \
  -DB2D_BUILD_SANITIZE=1 \
  -DB2D_BUILD_BENCH=1

cd ${CURRENT_DIR}
