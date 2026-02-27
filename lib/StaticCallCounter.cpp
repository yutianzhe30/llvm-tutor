//==============================================================================
// FILE:
//    StaticCallCounter.cpp
//
// DESCRIPTION:
//    Counts the number of static function calls in the input module. `Static`
//    refers to the fact that the analysed functions calls are compile-time
//    calls (as opposed to `dynamic`, i.e. run-time). Only direct function
//    calls are considered. Calls via functions pointers are not taken into
//    account.
//
//    This pass is used in `static`, a tool implemented in tools/StaticMain.cpp
//    that is a wrapper around StaticCallCounter. `static` allows you to run
//    StaticCallCounter without `opt`.
//
//    This example demonstrates:
//    1. How to iterate over all instructions in a Module (across all functions).
//    2. How to identify Call instructions (`CallBase`).
//    3. How to distinguish between Direct and Indirect calls.
//    4. Using `MapVector` to store results deterministically.
//
// USAGE:
//      opt -load-pass-plugin libStaticCallCounter.dylib `\`
//        -passes="print<static-cc>" `\`
//        -disable-output <input-llvm-file>
//
// License: MIT
//==============================================================================
#include "StaticCallCounter.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

// Pretty-prints the result of this analysis
static void printStaticCCResult(llvm::raw_ostream &OutS,
                         const ResultStaticCC &DirectCalls);

//------------------------------------------------------------------------------
// StaticCallCounter Implementation
//------------------------------------------------------------------------------
// This method implements the core logic of the analysis.
// It iterates over the entire module to count direct function calls.
StaticCallCounter::Result StaticCallCounter::runOnModule(Module &M) {
  // MapVector is used instead of std::map to ensure deterministic iteration order.
  // Key: Pointer to the called function (const Function *)
  // Value: Count of calls (unsigned)
  llvm::MapVector<const llvm::Function *, unsigned> Res;

  // Iterate over all Functions in the Module
  for (auto &Func : M) {
    // Iterate over all BasicBlocks in the Function
    for (auto &BB : Func) {
      // Iterate over all Instructions in the BasicBlock
      for (auto &Ins : BB) {

        // Check if the instruction is a call instruction.
        // `CallBase` is the base class for all call-like instructions (CallInst, InvokeInst, etc.)
        // `dyn_cast` returns null if the cast fails.
        auto *CB = dyn_cast<CallBase>(&Ins);
        if (nullptr == CB) {
          continue;
        }

        // Check if it is a direct function call.
        // `getCalledFunction()` returns the Function object if it's a direct call,
        // or nullptr if it's an indirect call (e.g., via function pointer).
        auto DirectInvoc = CB->getCalledFunction();
        if (nullptr == DirectInvoc) {
          continue;
        }

        // We have a direct function call - update the count for the function
        // being called.
        auto CallCount = Res.find(DirectInvoc);
        if (Res.end() == CallCount) {
          CallCount = Res.insert(std::make_pair(DirectInvoc, 0)).first;
        }
        ++CallCount->second;
      }
    }
  }

  return Res;
}

// Printer Pass implementation.
// Fetches the analysis result and prints it.
PreservedAnalyses
StaticCallCounterPrinter::run(Module &M,
                              ModuleAnalysisManager &MAM) {

  // Request the result of the StaticCallCounter analysis.
  // This triggers the analysis if it hasn't run yet.
  auto DirectCalls = MAM.getResult<StaticCallCounter>(M);

  printStaticCCResult(OS, DirectCalls);
  return PreservedAnalyses::all();
}

// Analysis Pass implementation.
// Runs the analysis and returns the result.
StaticCallCounter::Result
StaticCallCounter::run(llvm::Module &M, llvm::ModuleAnalysisManager &) {
  return runOnModule(M);
}

//------------------------------------------------------------------------------
// New PM Registration
//------------------------------------------------------------------------------
AnalysisKey StaticCallCounter::Key;

llvm::PassPluginLibraryInfo getStaticCallCounterPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "static-cc", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // #1 REGISTRATION FOR "opt -passes=print<static-cc>"
            PB.registerPipelineParsingCallback(
                [&](StringRef Name, ModulePassManager &MPM,
                    ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "print<static-cc>") {
                    MPM.addPass(StaticCallCounterPrinter(llvm::errs()));
                    return true;
                  }
                  return false;
                });
            // #2 REGISTRATION FOR "MAM.getResult<StaticCallCounter>(Module)"
            PB.registerAnalysisRegistrationCallback(
                [](ModuleAnalysisManager &MAM) {
                  MAM.registerPass([&] { return StaticCallCounter(); });
                });
          }};
};

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getStaticCallCounterPluginInfo();
}

//------------------------------------------------------------------------------
// Helper functions
//------------------------------------------------------------------------------
static void printStaticCCResult(raw_ostream &OutS,
                                const ResultStaticCC &DirectCalls) {
  OutS << "================================================="
       << "\n";
  OutS << "LLVM-TUTOR: static analysis results\n";
  OutS << "=================================================\n";
  const char *str1 = "NAME";
  const char *str2 = "#N DIRECT CALLS";
  OutS << format("%-20s %-10s\n", str1, str2);
  OutS << "-------------------------------------------------"
       << "\n";

  for (auto &CallCount : DirectCalls) {
    OutS << format("%-20s %-10lu\n", CallCount.first->getName().str().c_str(),
                   CallCount.second);
  }

  OutS << "-------------------------------------------------"
       << "\n\n";
}
