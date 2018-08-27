@echo off

set CURRENT_DIR=%CD%
set BUILD_DIR="build_vs2017_x86"
set ASMJIT_DIR="../../asmjit"

mkdir ..\%BUILD_DIR%
cd ..\%BUILD_DIR%
cmake .. -G"Visual Studio 15" -DASMJIT_DIR="%ASMJIT_DIR%" -DB2D_BUILD_TEST=1 -DB2D_BUILD_BENCH=1
cd %CURRENT_DIR%
