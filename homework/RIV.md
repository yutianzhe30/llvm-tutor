# Reachable Integer Values (RIV) Pass

## Theoretical Insights

### Dominator Tree
In a Control Flow Graph (CFG), a node (Basic Block) **A** dominates node **B** if every path from the entry node to **B** goes through **A**.
This relationship forms a tree called the **Dominator Tree**.

If A dominates B, then any definition in A is guaranteed to be available (reachable) in B. This is crucial for optimizations like Common Subexpression Elimination (CSE) and Loop Invariant Code Motion (LICM).

### Reachability Analysis
`RIV` computes the set of integer values that are available for use in each basic block.
*   **Definitions**: Instructions that produce a value (e.g., `add`, `load`).
*   **Reachability**: A definition `d` in block `D` is reachable in block `U` if `D` dominates `U`.

### SSA Form (Static Single Assignment)
LLVM IR uses SSA form, which means every variable is assigned exactly once. This simplifies data flow analysis significantly because "definition" and "assignment" are synonymous.

## Homework

### Level 1: Floating Point Reachability
**Task**: Modify the pass to also track reachable **floating-point** values.

**Hint**:
*   Update the check `Inst.getType()->isIntegerTy()` to also allow `isFloatingPointTy()`.
*   Update `DefinedValuesMap` population logic.
*   Update global variable and argument checks.

### Level 2: Immediate Dominator
**Task**: Modify the printer to also print the **Immediate Dominator** of each basic block.

**Hint**:
*   The `DominatorTree` class has a method `getNode(BB)->getIDom()`.
*   You will need to pass the `DominatorTree` to the printer pass or store this info in the result.

### Challenge: Liveness Analysis
**Task**: Implement a simplified Liveness Analysis.
*   Instead of "what is defined before me" (Reachability), compute "what is used after me" (Liveness).
*   A variable is **live** at a point if it holds a value that may be needed in the future.

**Hint**:
*   Liveness is a **backward** analysis (starts from the end of the function).
*   Start with `Use` operands of instructions.
*   Propagate liveness up the CFG (predecessors).
