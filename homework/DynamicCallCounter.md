# DynamicCallCounter Pass

## Theoretical Insights

### Runtime Instrumentation
Instrumentation adds code to a program to collect data at runtime. `DynamicCallCounter` inserts a global counter for each function and increments it whenever the function is entered. This is a common technique for profiling tools.

### Global Constructors and Destructors
*   **Constructors (`.ctors`)**: Functions that run before `main()`.
*   **Destructors (`.dtors`)**: Functions that run after `main()` returns (or when `exit()` is called).

LLVM provides helper functions like `appendToGlobalDtors` to easily register functions to run at program exit. This is where we print our report.

### Atomic Operations
In a multi-threaded program, simply doing `Load -> Add -> Store` on a global variable is **not thread-safe**. Two threads might load the same value, increment it, and store it back, missing a count. Real-world profilers use atomic instructions (e.g., `atomicrmw add`) or thread-local storage to avoid this.

## Homework

### Level 1: Basic Block Execution Count
**Task**: Modify the pass to count how many times a specific **Basic Block** is executed (e.g., the first block of "main").

**Hint**:
*   Instead of iterating just functions, iterate over Basic Blocks.
*   Identify the block you want (e.g., `if (F.getName() == "main" && &BB == &F.getEntryBlock())`).
*   Create a counter for it and increment it inside that block.

### Challenge: Execution Profiler
**Task**: Implement a full Basic Block execution profiler.
1.  Assign a unique ID to every Basic Block in the module.
2.  Create an array of counters (one for each block).
3.  Instrument every block to increment its corresponding counter.
4.  Print the counts for all blocks at the end.

**Hint**:
*   This generates a lot of data!
*   You might need to name blocks to make the output readable (`BB.setName(...)`).
*   Be careful with the `printf` wrapper loop; it needs to iterate over all blocks.
