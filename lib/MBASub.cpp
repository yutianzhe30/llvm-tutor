//==============================================================================
// FILE:
//    MBASub.cpp
//
// DESCRIPTION:
//    Obfuscation for integer sub instructions through Mixed Boolean Arithmetic
//    (MBA). This pass performs an instruction substitution based on this
//    equality:
//      a - b == (a + ~b) + 1
//    See formula 2.2 (j) in [1].
//
//    This example demonstrates:
//    1. How to iterate over instructions in a Basic Block while modifying it.
//    2. How to identify specific instructions (BinaryOperator, Sub).
//    3. How to replace one instruction with a sequence of new instructions.
//    4. Using `ReplaceInstWithInst` utility.
//
// USAGE:
//      $ opt -load-pass-plugin <BUILD_DIR>/lib/libMBASub.so `\`
//        -passes=-"mba-sub" <bitcode-file>
//
//  [1] "Hacker's Delight" by Henry S. Warren, Jr.
//
// License: MIT
//==============================================================================
#include "MBASub.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"



using namespace llvm;

#define DEBUG_TYPE "mba-sub"

// Statistics are useful for debugging and measuring the impact of the pass.
// They are printed when running opt with -stats.
STATISTIC(SubstCount, "The # of substituted instructions");

//-----------------------------------------------------------------------------
// MBASub Implementaion
//-----------------------------------------------------------------------------
bool MBASub::runOnBasicBlock(BasicBlock &BB) {
  bool Changed = false;

  // Loop over all instructions in the block.
  // IMPORTANT: Replacing instructions requires careful iteration.
  // `ReplaceInstWithInst` takes a reference to the iterator and updates it
  // to point to the newly inserted instruction. This ensures that `++Inst`
  // in the loop header correctly moves to the next instruction in the block.
  for (auto Inst = BB.begin(), IE = BB.end(); Inst != IE; ++Inst) {

    // Skip non-binary (e.g. unary or compare) instruction.
    // dyn_cast checks if the instruction is a BinaryOperator.
    auto *BinOp = dyn_cast<BinaryOperator>(Inst);
    if (!BinOp)
      continue;

    /// Skip instructions other than integer sub.
    unsigned Opcode = BinOp->getOpcode();
    if (Opcode != Instruction::Sub || !BinOp->getType()->isIntegerTy())
      continue;

    // A uniform API for creating instructions and inserting
    // them into basic blocks.
    // passing `BinOp` to the constructor sets the insertion point *before* BinOp.
    IRBuilder<> Builder(BinOp);

    // Create an instruction representing (a + ~b) + 1
    // 1. CreateNot(b) -> ~b
    // 2. CreateAdd(a, ~b) -> a + ~b
    // 3. CreateAdd(..., 1) -> (a + ~b) + 1
    Instruction *NewValue = BinaryOperator::CreateAdd(
        Builder.CreateAdd(BinOp->getOperand(0),
                          Builder.CreateNot(BinOp->getOperand(1))),
        ConstantInt::get(BinOp->getType(), 1));

    // The following is visible only if you pass -debug on the command line
    // *and* you have an assert build.
    LLVM_DEBUG(dbgs() << *BinOp << " -> " << *NewValue << "\n");

    // Replace `(a - b)` (original instructions) with `(a + ~b) + 1`
    // (the new instruction).
    ReplaceInstWithInst(&BB, Inst, NewValue);
    Changed = true;

    // Update the statistics
    ++SubstCount;
  }
  return Changed;
}

PreservedAnalyses MBASub::run(llvm::Function &F,
                              llvm::FunctionAnalysisManager &) {
  bool Changed = false;

  for (auto &BB : F) {
    Changed |= runOnBasicBlock(BB);
  }
  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getMBASubPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "mba-sub", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "mba-sub") {
                    FPM.addPass(MBASub());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getMBASubPluginInfo();
}
