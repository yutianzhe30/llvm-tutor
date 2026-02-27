# StaticCallCounter Pass

## Theoretical Insights

### Static vs. Dynamic Analysis
*   **Static Analysis**: Analyzes the code without executing it. It can find potential bugs and gather metrics but cannot know the runtime behavior (e.g., how many times a loop executes). `StaticCallCounter` counts how many `call` instructions appear in the source code.
*   **Dynamic Analysis**: Analyzes the program while it is running. It provides accurate runtime information but adds overhead. `DynamicCallCounter` (covered later) counts how many times functions are *actually called* during execution.

### Direct vs. Indirect Calls
*   **Direct Call**: The target function is known at compile time (e.g., `foo()`).
*   **Indirect Call**: The target function is determined at runtime via a function pointer (e.g., `(*func_ptr)()`).

`StaticCallCounter` primarily counts direct calls because `getCalledFunction()` returns `nullptr` for indirect calls.

### CallBase
In LLVM IR, `CallInst` (function call) and `InvokeInst` (function call with exception handling) both inherit from `CallBase`. It's good practice to cast to `CallBase` when you want to handle both types of calls uniformly.

## Homework

### Level 1: Filter by Name
**Task**: Modify `StaticCallCounter.cpp` to count calls *only* to a function named "foo".

**Hint**:
*   Inside the loop, check `DirectInvoc->getName()`.
*   String comparison: `DirectInvoc->getName() == "foo"`.

### Level 2: Count Indirect Calls
**Task**: Modify the pass to also count indirect calls.

**Hint**:
*   If `CB->getCalledFunction()` returns `nullptr`, it's likely an indirect call.
*   Keep a separate counter for indirect calls.
*   Update `ResultStaticCC` (or create a struct) to hold both the map of direct calls and the count of indirect calls.
*   Update the printer to display this count.

### Challenge: Call Graph (Mental Model)
**Task**: Instead of just counting calls, try to modify the pass to print a simple "Who calls Whom" graph (e.g., `main -> foo`, `foo -> bar`).

**Hint**:
*   The outer loop gives you the *caller* (`Func`).
*   The inner loop gives you the *callee* (`DirectInvoc`).
*   Store this relationship (e.g., `MapVector<Function*, Set<Function*>>`) and print it.
