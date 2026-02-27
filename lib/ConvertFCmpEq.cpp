//=============================================================================
// FILE:
//    ConvertFCmpEq.cpp
//
// DESCRIPTION:
//    Transformation pass which uses the results of the FindFCmpEq analysis pass
//    to convert all equality-based floating point comparison instructions in a
//    function to indirect, difference-based comparisons.
//
//    This pass replaces direct equality checks (a == b) with an epsilon-based
//    check (|a - b| < epsilon).
//
//    This example demonstrates:
//    1. How to use an Analysis Pass (FindFCmpEq) within a Transformation Pass.
//    2. How to create new instructions (FSub, BitCast, And, etc.) using IRBuilder.
//    3. How to modify existing instructions in place (setPredicate, setOperand).
//    4. Using Statistics to track pass activity.
//    5. Dealing with floating point constants and types (APInt, APFloat).
//
//    Originally developed for [1].
//
//    [1] "Writing an LLVM Optimization" by Jonathan Smith
//
// USAGE:
//      opt --load-pass-plugin libConvertFCmpEq.dylib [--stats] `\`
//        --passes='convert-fcmp-eq' --disable-output <input-llvm-file>
//
// License: MIT
//=============================================================================
#include "ConvertFCmpEq.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include <cassert>

using namespace llvm;

// Helper function to perform the conversion on a single instruction.
static FCmpInst *convertFCmpEqInstruction(FCmpInst *FCmp) noexcept {
  assert(FCmp && "The given fcmp instruction is null");

  if (!FCmp->isEquality()) {
    // We're only interested in equality-based comparisons.
    return nullptr;
  }

  Value *LHS = FCmp->getOperand(0);
  Value *RHS = FCmp->getOperand(1);

  // Determine the new floating-point comparison predicate based on the current one.
  // Equality (OEQ/UEQ) becomes Less Than (OLT/ULT) because we check if difference is smaller than epsilon.
  // Inequality (ONE/UNE) becomes Greater/Equal (OGE/UGE).
  CmpInst::Predicate CmpPred = [FCmp] {
    switch (FCmp->getPredicate()) {
    case CmpInst::Predicate::FCMP_OEQ:
      return CmpInst::Predicate::FCMP_OLT;
    case CmpInst::Predicate::FCMP_UEQ:
      return CmpInst::Predicate::FCMP_ULT;
    case CmpInst::Predicate::FCMP_ONE:
      return CmpInst::Predicate::FCMP_OGE;
    case CmpInst::Predicate::FCMP_UNE:
      return CmpInst::Predicate::FCMP_UGE;
    default:
      llvm_unreachable("Unsupported fcmp predicate");
    }
  }();

  // Create the objects and values needed to perform the equality comparison
  // conversion.
  Module *M = FCmp->getModule();
  assert(M && "The given fcmp instruction does not belong to a module");
  LLVMContext &Ctx = M->getContext();
  IntegerType *I64Ty = IntegerType::get(Ctx, 64);
  Type *DoubleTy = Type::getDoubleTy(Ctx);

  // Define the sign-mask to compute absolute value using bitwise AND.
  // (Assuming IEEE 754, the sign bit is the most significant bit).
  ConstantInt *SignMask = ConstantInt::get(I64Ty, ~(1L << 63));

  // The machine epsilon value for IEEE 754 double-precision values.
  // This serves as our threshold.
  APInt EpsilonBits(64, 0x3CB0000000000000);
  Constant *EpsilonValue =
      ConstantFP::get(DoubleTy, EpsilonBits.bitsToDouble());

  // Create an IRBuilder with an insertion point set to the given fcmp instruction.
  // This means new instructions will be inserted *before* the FCmp instruction.
  IRBuilder<> Builder(FCmp);

  // 1. Calculate the difference: %0 = fsub double %a, %b
  auto *FSubInst = Builder.CreateFSub(LHS, RHS);

  // 2. Calculate absolute value of the difference.
  //    Since we don't have a direct 'fabs' instruction in LLVM IR (it's an intrinsic),
  //    we can do it via bit manipulation for floating point numbers.
  //    %1 = bitcast double %0 to i64
  auto *CastToI64 = Builder.CreateBitCast(FSubInst, I64Ty);
  //    %2 = and i64 %1, 0x7fffffffffffffff (clear sign bit)
  auto *AbsValue = Builder.CreateAnd(CastToI64, SignMask);
  //    %3 = bitcast i64 %2 to double
  auto *CastToDouble = Builder.CreateBitCast(AbsValue, DoubleTy);

  // 3. Compare with Epsilon.
  //    %4 = fcmp <olt/ult/oge/uge> double %3, Epsilon
  // Rather than creating a new instruction and replacing usages, we reuse the existing FCmp instruction.
  // We update its predicate and operands.
  FCmp->setPredicate(CmpPred);
  FCmp->setOperand(0, CastToDouble);
  FCmp->setOperand(1, EpsilonValue);

  return FCmp;
}

static constexpr char PassArg[] = "convert-fcmp-eq";
static constexpr char PluginName[] = "ConvertFCmpEq";

#define DEBUG_TYPE ::PassArg
STATISTIC(FCmpEqConversionCount,
          "Number of direct floating-point equality comparisons converted");

//------------------------------------------------------------------------------
// ConvertFCmpEq implementation
//------------------------------------------------------------------------------
PreservedAnalyses ConvertFCmpEq::run(Function &Func,
                                     FunctionAnalysisManager &FAM) {
  // Request the results of FindFCmpEq analysis.
  auto &Comparisons = FAM.getResult<FindFCmpEq>(Func);

  // Run the transformation using the analysis results.
  bool Modified = run(Func, Comparisons);

  // If we modified the function, we (conservatively) say we preserved no analyses.
  return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool ConvertFCmpEq::run(Function &Func,
                        const FindFCmpEq::Result &Comparisons) {
  bool Modified = false;
  // Functions marked explicitly 'optnone' should be ignored (e.g. clang -O0).
  if (Func.hasFnAttribute(Attribute::OptimizeNone)) {
    LLVM_DEBUG(dbgs() << "Ignoring optnone-marked function \"" << Func.getName()
                      << "\"\n");
    Modified = false;
  } else {
    // Iterate over the list of instructions found by the analysis pass.
    for (FCmpInst *FCmp : Comparisons) {
      if (convertFCmpEqInstruction(FCmp)) {
        ++FCmpEqConversionCount;
        Modified = true;
      }
    }
  }

  return Modified;
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
PassPluginLibraryInfo getConvertFCmpEqPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, PluginName, LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [&](StringRef Name, FunctionPassManager &FPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  if (!Name.compare(PassArg)) {
                    FPM.addPass(ConvertFCmpEq());
                    return true;
                  }

                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getConvertFCmpEqPluginInfo();
}
