# Control Flow Graph (CFG) Transformations

## Theoretical Insights

### Basic Blocks and CFG
*   **Basic Block (BB)**: A sequence of instructions with a single entry point and a single exit point (terminator).
*   **Control Flow Graph (CFG)**: A graph where nodes are Basic Blocks and edges represent possible control flow transfers (branches).

### PHI Nodes
In SSA form, when control flow merges from multiple predecessors, we need a way to select values based on where we came from.
`%val = phi type [ %val_from_pred1, %pred1 ], [ %val_from_pred2, %pred2 ]`
This instruction selects `%val_from_pred1` if we arrived from `%pred1`, and `%val_from_pred2` if from `%pred2`.

### DuplicateBB Pass
This pass obfuscates control flow by duplicating basic blocks.
*   **Splitting**: It uses `SplitBlockAndInsertIfThenElse` to inject a conditional branch.
*   **Cloning**: It clones the original block into two branches.
*   **Opaque Predicate**: It uses a condition that is logically known (or probabilistic) but hard to determine statically, to confuse analysis tools.

### MergeBB Pass
This pass attempts to reverse the duplication (or simply optimize code) by finding identical basic blocks and merging them.
*   It identifies "sibling" blocks (predecessors of the same successor) that are identical.
*   It redirects edges and updates PHI nodes.

## Homework

### Level 1: Split Basic Block
**Task**: Create a pass that simply splits every basic block into two halves (unconditional branch).

**Hint**:
*   Use `BB.splitBasicBlock(Iterator, "split")`.
*   You can split at the middle instruction.

### Level 2: Block Fusion (Unconditional Branch Merging)
**Task**: Implement a pass that merges block A into block B if A unconditionally branches to B and A has only one successor (B), and B has only one predecessor (A).

**Hint**:
*   This is standard "SimplifyCFG".
*   Move instructions from B to A.
*   Update B's successors to point to A.
*   Remove B.
*   LLVM has utilities for this, but try to implement the logic manually to understand it.

### Challenge: Tail Merging
**Task**: Implement a pass that merges the *tails* of two basic blocks if they are identical, even if the heads are different.
*   CFG:
    ```
    BB1:          BB2:
      inst1         inst3
      inst2         inst2
      br Succ       br Succ
    ```
*   Result:
    ```
    BB1:          BB2:
      inst1         inst3
      br Merge      br Merge

    Merge:
      inst2
      br Succ
    ```

**Hint**:
*   Walk backwards from the terminators of two blocks.
*   Find the longest common suffix.
*   Split blocks before the common suffix.
*   Direct both to a new block containing the suffix.
