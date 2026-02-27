# Floating Point Transformations

## Theoretical Insights

### Floating Point Representation
Floating point numbers (IEEE 754) are approximations of real numbers. They have a Sign bit, an Exponent, and a Mantissa (fraction).
Because of this approximation, calculations that should be mathematically equal might result in slightly different bit patterns (e.g., `0.1 + 0.2 != 0.3` in standard FP arithmetic).

### Equality Comparisons
Comparing floating point numbers for exact equality (`==`) is considered bad practice because of rounding errors.
Instead, it is recommended to check if the difference is smaller than a small threshold (Epsilon):
`abs(a - b) < Epsilon`

### FindFCmpEq Pass
This is an **Analysis Pass** that scans the code for `fcmp` instructions with equality predicates (`oeq`, `ueq`, `one`, `une`).

### ConvertFCmpEq Pass
This is a **Transformation Pass** that uses the results of `FindFCmpEq` to rewrite the code.
It replaces `a == b` with:
1.  `diff = a - b`
2.  `abs_diff = abs(diff)` (implemented via bit manipulation)
3.  `abs_diff < Epsilon`

## Homework

### Level 1: Divide by Zero Warning
**Task**: Create a pass that warns (prints to stderr) if it finds a floating point division where the denominator is the constant `0.0`.

**Hint**:
*   Iterate over instructions.
*   Look for `Instruction::FDiv`.
*   Check the second operand: `isa<ConstantFP>(Inst.getOperand(1))` and if its value is zero.

### Level 2: Reciprocal Multiplication
**Task**: Implement a pass that replaces `x / C` (where `C` is a constant) with `x * (1/C)`.
Multiplication is generally faster than division.

**Hint**:
*   Match `Instruction::FDiv` with a constant denominator.
*   Compute `Recip = 1.0 / C` at compile time.
*   Create a new `FMul` instruction: `Builder.CreateFMul(X, ConstantFP::get(..., Recip))`.
*   Replace the division with the multiplication.
*   *Note*: This changes the result slightly due to precision, so compilers usually do this only with "fast-math" flags.

### Challenge: Approximate Sine
**Task**: Replace calls to `sin(x)` with a small Taylor series approximation (e.g., `x - x^3/6`) for small `x`.

**Hint**:
*   Look for `CallInst`.
*   Check if the called function is `sin` (intrinsic or library call).
*   Generate instructions for `x * x * x`, division by 6, and subtraction.
