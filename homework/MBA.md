# Mixed Boolean Arithmetic (MBA) Obfuscation

## Theoretical Insights

### Code Obfuscation
The goal of obfuscation is to make the code difficult for humans and tools to reverse engineer, while preserving its semantics (behavior).

### Mixed Boolean Arithmetic (MBA)
MBA identities mix arithmetic operations (like `+`, `-`, `*`) with bitwise boolean operations (like `&`, `|`, `^`, `~`). These identities are often complex and counter-intuitive.

Example:
*   Standard: `a + b`
*   MBA: `(a ^ b) + 2 * (a & b)`

This works because `a ^ b` represents "addition without carry" and `a & b` represents the "carry".

### Instruction Substitution
The `MBASub` and `MBAAdd` passes implement **Instruction Substitution**. They find a specific instruction pattern (e.g., `a - b`) and replace it with a more complex, equivalent sequence of instructions.

### Pattern Matching in LLVM
To perform substitution, we first need to **match** the target instructions.
*   Check opcode: `Inst.getOpcode() == Instruction::Sub`
*   Check types: `Inst.getType()->isIntegerTy()`
*   Check operands (if needed).

## Homework

### Level 1: Simple Substitution
**Task**: Implement a new pass (or modify `MBASub`) to replace integer addition `a + b` with `a - (-b)`.

**Hint**:
*   Match `Instruction::Add`.
*   Create negation: `Builder.CreateNeg(OperandB)`.
*   Create subtraction: `Builder.CreateSub(OperandA, NegatedOperandB)`.

### Level 2: XOR Obfuscation
**Task**: Obfuscate `XOR` instructions (`a ^ b`) using the identity `(a | b) - (a & b)`.

**Hint**:
*   Match `Instruction::Xor`.
*   Create OR: `Builder.CreateOr(A, B)`.
*   Create AND: `Builder.CreateAnd(A, B)`.
*   Create SUB: `Builder.CreateSub(OrResult, AndResult)`.

### Challenge: Linear MBA
**Task**: Implement the following linear MBA identity for addition:
`x + y = (x ^ y) + 2 * (x & y)`

**Hint**:
*   This is actually used inside `MBAAdd` but as part of a larger polynomial.
*   Implement it as a standalone substitution for *any* integer width (not just 8-bit).
*   Be careful with the constant `2`. Use `ConstantInt::get(Type, 2)` to ensure it has the same bit width as the operands.
