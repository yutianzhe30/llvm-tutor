```
export LLVM_DIR=usr/lib/llvm-21/
```

```
$LLVM_DIR/bin/opt -load-pass-plugin ./build/lib/libHelloWorld.so -passes=hello-world -disable-output input_for_hello.ll
```

```
 $LLVM_DIR/bin/opt -load-pass-plugin build/lib/libHelloWorld.so --passes=log-return input_for_hello.bc -o instrumented.bin
```