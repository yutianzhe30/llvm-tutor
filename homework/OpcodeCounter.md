# OpcodeCounter Pass

## Theoretical Insights

### Analysis vs. Transformation Passes
In LLVM, passes are broadly categorized into:
*   **Transformation Passes**: Modify the IR (e.g., optimization).
*   **Analysis Passes**: Compute information about the IR without modifying it (e.g., finding loops, counting instructions).

`OpcodeCounter` is an **Analysis Pass**. It computes a frequency map of opcodes for each function.

### The Analysis Manager
The Analysis Manager (AM) caches the results of analysis passes. If a transformation pass invalidates the IR (e.g., deletes instructions), the AM knows that the cached analysis results might be stale and re-runs the analysis when needed.

This is why `OpcodeCounter` is split into:
1.  `OpcodeCounter` (Analysis): Does the work and returns the result.
2.  `OpcodeCounterPrinter` (Pass): A pass that asks the AM for the result and prints it.

### Iterating over Instructions
LLVM IR has a hierarchy: `Module` -> `Function` -> `BasicBlock` -> `Instruction`.
To visit every instruction, we use nested loops:
```cpp
for (auto &BB : Func) {      // Iterate over BasicBlocks in Function
  for (auto &Inst : BB) {    // Iterate over Instructions in BasicBlock
     // Process Inst
  }
}
```

## Homework

### Level 1: Binary Operators Only
**Task**: Modify `OpcodeCounter.cpp` to count *only* binary operators (e.g., `add`, `sub`, `mul`, `fadd`, etc.).

**Hint**:
*   The `Instruction` class has a method `isBinaryOp()`.
*   Check `if (Inst.isBinaryOp())` inside the loop.

### Level 2: Total Instruction Count
**Task**: Modify the pass to print the **total** number of instructions in the function, in addition to the per-opcode breakdown.

**Hint**:
*   You can sum the values in the map, or keep a separate counter in the loop.
*   Update `printOpcodeCounterResult` to display this total.

### Challenge: Memory Instruction Ratio
**Task**: Extend the pass to calculate and print the ratio of memory instructions (`load`, `store`, `alloca`) to the total number of instructions.

**Hint**:
*   `Inst.getOpcode()` returns an enum. You can check for `Instruction::Load`, `Instruction::Store`, etc.
*   Alternatively, `Inst.mayReadOrWriteMemory()` might be useful, but be careful as function calls also read/write memory.
