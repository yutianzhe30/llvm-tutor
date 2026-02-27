# HelloWorld Pass

## Theoretical Insights

### What is an LLVM Pass?
An LLVM Pass is a modular piece of code that traverses, analyzes, or transforms LLVM Intermediate Representation (IR). Passes are the building blocks of the LLVM optimization pipeline.

There are two main types of passes:
1.  **Analysis Passes**: These inspect the IR to gather information (e.g., computing the complexity of a function, finding loops) but do not modify it.
2.  **Transformation Passes**: These modify the IR to optimize it (e.g., removing dead code, inlining functions).

### The New Pass Manager
LLVM has transitioned to a "New Pass Manager" (NPM) which replaces the "Legacy Pass Manager". The NPM uses a more modern C++ design, relying on templates and mixins.

Key concepts in `HelloWorld.cpp`:
*   `PassInfoMixin<HelloWorld>`: A CRTP (Curiously Recurring Template Pattern) base class that provides boilerplate code for your pass.
*   `run(Function &F, FunctionAnalysisManager &)`: The entry point of your pass. The Pass Manager calls this method for every `Function` in the module.
*   `PreservedAnalyses`: A return type that tells the Pass Manager which analyses are still valid after your pass runs. Since `HelloWorld` doesn't modify the code, it returns `PreservedAnalyses::all()`.

## Homework

### Level 1: Say Goodbye
**Task**: Modify `HelloWorld.cpp` to print "Goodbye from: [Function Name]" instead of "Hello from: ...".

**Hint**: Look at the `visitor` function where `errs()` is used.

### Level 2: Return Type
**Task**: Modify the pass to print the return type of the function as well.

**Hint**:
*   The `Function` class has a method `getReturnType()`.
*   You can print LLVM types using `errs() << *Type_Object << "\n"`.

### Challenge: Selective Greeting
**Task**: Create a new pass called `GoodbyeWorld` (in a new file or by modifying the existing one) that only prints a message if the function name is "main".

**Hint**:
*   You can check the function name using `F.getName() == "main"`.
*   You will need to register a new pass name in `getHelloWorldPluginInfo` (or a new registration function) if you want to run it as a separate pass (e.g., `-passes=goodbye-world`).
