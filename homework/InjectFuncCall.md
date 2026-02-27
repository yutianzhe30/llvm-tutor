# InjectFuncCall Pass

## Theoretical Insights

### Code Instrumentation
Instrumentation involves adding extra code to a program to monitor its execution (e.g., for profiling, debugging, or coverage analysis). This pass demonstrates static instrumentation: we modify the source code (at the IR level) before compilation is finished.

### The IRBuilder
`IRBuilder` is a helper class in LLVM that simplifies the creation of LLVM IR instructions. It keeps track of the "insertion point" (where new instructions should go) and provides methods like `CreateAdd`, `CreateCall`, `CreateRet`, etc.

### Module vs. Function Pass
`InjectFuncCall` is a **Module Pass** because it adds global variables (the format string) and function declarations (`printf`) which belong to the Module scope, not just a single Function.

### Opaque Pointers
Modern LLVM uses "opaque pointers" (`ptr`), meaning pointer types don't carry information about the type they point to (e.g., just `ptr` instead of `i8*` or `i32*`). This simplifies the IR but requires you to be explicit about types when loading/storing.

## Homework

### Level 1: Customize the Message
**Task**: Change the injected `printf` call to print "Entering function: [Function Name]" instead of the default message.

**Hint**:
*   Locate where `PrintfFormatStr` is defined.
*   Modify the string literal passed to `ConstantDataArray::getString`.

### Level 2: Exit Instrumentation
**Task**: Inject a `printf` call at the *end* of the function (before every `ret` instruction) saying "Exiting function: [Function Name]".

**Hint**:
*   Iterate over all instructions in the function.
*   Check if an instruction is a return instruction (`isa<ReturnInst>(Inst)`).
*   If it is, set the `IRBuilder` insertion point to *before* that instruction and insert the call.

### Challenge: Argument Printing
**Task**: If the function takes integer arguments, modify the pass to print the value of the *first* argument.

**Hint**:
*   Check `F.arg_size() > 0`.
*   Get the first argument: `Value *FirstArg = F.getArg(0)`.
*   Check its type: `FirstArg->getType()->isIntegerTy()`.
*   Update the `printf` format string to include an integer format specifier (`%d`).
*   Pass `FirstArg` to the `CreateCall` method.
