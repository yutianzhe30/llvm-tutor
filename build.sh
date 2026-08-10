#!/bin/bash

export LLVM_DIR=/usr/lib/llvm-21/
mkdir -p build
cd build
cmake -DLT_LLVM_INSTALL_DIR=$LLVM_DIR ..
make

export LLVM_DIR=/usr/lib/llvm-21/

$LLVM_DIR/bin/clang -O1 -S -emit-llvm ./inputs/input_for_hello.c -o input_for_hello.ll
$LLVM_DIR/bin/opt -load-pass-plugin ./build/lib/libHelloWorld.so -passes=hello-world -disable-output input_for_hello.ll

export SOURCE_DIR=$(pwd)
export BUILD_DIR=$SOURCE_DIR/build 
$LLVM_DIR/bin/clang -emit-llvm -c $SOURCE_DIR/inputs/input_for_cc.c -o input_for_cc.bc
# Run the pass through opt
$LLVM_DIR/bin/opt -load-pass-plugin $BUILD_DIR/lib/libOpcodeCounter.so --passes="print<opcode-counter>" -disable-output input_for_cc.bc

#Func InjectionCall
export LLVM_DIR=<installation/dir/of/llvm/21>
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -O0 -emit-llvm -c $SOURCE_DIR/inputs/input_for_hello.c -o input_for_hello.bc
# Run the pass through opt
$LLVM_DIR/bin/opt -load-pass-plugin $BUILD_DIR/lib/libInjectFuncCall.so --passes="inject-func-call" input_for_hello.bc -o instrumented.bin

$LLVM_DIR/bin/lli instrumented.bin

# Static function calls
# Generate an LLVM file to analyze
$LLVM_DIR/bin/clang -emit-llvm -c $SOURCE_DIR/inputs/input_for_cc.c -o input_for_cc.bc
# Run the pass through opt
$LLVM_DIR/bin/opt -load-pass-plugin $BUILD_DIR/lib/libStaticCallCounter.so -passes="print<static-cc>" -disable-output input_for_cc.bc