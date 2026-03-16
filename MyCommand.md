```
export LLVM_DIR=usr/lib/llvm-21/
```

```
$LLVM_DIR/bin/opt -load-pass-plugin ./build/lib/libHelloWorld.so -passes=hello-world -disable-output input_for_hello.ll
```

```
 $LLVM_DIR/bin/opt -load-pass-plugin build/lib/libHelloWorld.so --passes=log-return input_for_hello.bc -o instrumented.bin
```


$LLVM_DIR/bin/clang -emit-llvm -c ./inputs/input_for_cc.c -o input_for_cc.bc
$LLVM_DIR/bin/opt -load-pass-plugin ./build/lib/libOpcodeCounter.so --passes="print<opcode-counter>" -disable-output input_for_cc.bc