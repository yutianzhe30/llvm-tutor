//=============================================================================
// FILE:
//    OpcodeCounter.cpp
//
// DESCRIPTION:
//    Visits all instructions in a function and counts how many times every
//    LLVM IR opcode was used. Prints the output to stderr.
//
//    This example demonstrates:
//    1. How to create an Analysis Pass (as opposed to a Transformation Pass).
//    2. How to traverse instructions within a function.
//    3. How to register a pass to run automatically in an existing pipeline.
//    4. The separation between the Analysis pass (logic) and the Printer pass (output).
//
// USAGE:
//    1. New PM
//      opt -load-pass-plugin libOpcodeCounter.dylib `\`
//        -passes="print<opcode-counter>" `\`
//        -disable-output <input-llvm-file>
//    2. Automatically through an optimisation pipeline - new PM
//      opt -load-pass-plugin libOpcodeCounter.dylib --passes='default<O1>' `\`
//        -disable-output <input-llvm-file>
//
// License: MIT
//=============================================================================
#include "OpcodeCounter.h"

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;

// Pretty-prints the result of this analysis
// defined at the end of this file.
static void printOpcodeCounterResult(llvm::raw_ostream &,
                                     const ResultOpcodeCounter &OC);

//-----------------------------------------------------------------------------
// OpcodeCounter implementation
//-----------------------------------------------------------------------------
// The AnalysisKey is a special type used by the Analysis Manager to identify
// this specific analysis pass. It's address is used as the unique identifier.
llvm::AnalysisKey OpcodeCounter::Key;

// This method implements the core logic of the analysis.
// It iterates over all blocks and instructions to count opcode usage.
OpcodeCounter::Result OpcodeCounter::generateOpcodeMap(llvm::Function &Func) {
  OpcodeCounter::Result OpcodeMap;

  // Iterate over all Basic Blocks in the Function
  for (auto &BB : Func) {
    // Iterate over all Instructions in the Basic Block
    for (auto &Inst : BB) {
      // 'getOpcodeName()' returns the string representation of the opcode
      // (e.g., "add", "sub", "br", "call").
      StringRef Name = Inst.getOpcodeName();

      if (OpcodeMap.find(Name) == OpcodeMap.end()) {
        OpcodeMap[Inst.getOpcodeName()] = 1;
      } else {
        OpcodeMap[Inst.getOpcodeName()]++;
      }
    }
  }

  return OpcodeMap;
}

// The 'run' method for an Analysis Pass returns the Result of the analysis.
// This result is cached by the Analysis Manager and can be queried by other passes.
OpcodeCounter::Result OpcodeCounter::run(llvm::Function &Func,
                                         llvm::FunctionAnalysisManager &) {
  return generateOpcodeMap(Func);
}

//-----------------------------------------------------------------------------
// OpcodeCounterPrinter implementation
//-----------------------------------------------------------------------------
// This is a separate pass whose sole purpose is to request the results of
// OpcodeCounter and print them. This separation of concerns is standard in LLVM.
PreservedAnalyses OpcodeCounterPrinter::run(Function &Func,
                                            FunctionAnalysisManager &FAM) {
  // Request the result of the OpcodeCounter analysis for the current function.
  // If the analysis hasn't been run yet, the AM will run it.
  // If it has been run and is still valid, the AM will return the cached result.
  auto &OpcodeMap = FAM.getResult<OpcodeCounter>(Func);

  // Use the output stream (OS) provided in the constructor (usually errs())
  OS << "Printing analysis 'OpcodeCounter Pass' for function '"
     << Func.getName() << "':\n";

  printOpcodeCounterResult(OS, OpcodeMap);

  // This pass only prints output and doesn't modify the IR, so we preserve all analyses.
  return PreservedAnalyses::all();
}

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getOpcodeCounterPluginInfo() {
  return {
    LLVM_PLUGIN_API_VERSION, "OpcodeCounter", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
          // #1 REGISTRATION FOR "opt -passes=print<opcode-counter>"
          // Register OpcodeCounterPrinter so that it can be used when
          // specifying pass pipelines with `-passes=`.
          PB.registerPipelineParsingCallback(
              [&](StringRef Name, FunctionPassManager &FPM,
                  ArrayRef<PassBuilder::PipelineElement>) {
                if (Name == "print<opcode-counter>") {
                  FPM.addPass(OpcodeCounterPrinter(llvm::errs()));
                  return true;
                }
                return false;
              });
          // #2 REGISTRATION FOR "-O{1|2|3|s}"
          // Register OpcodeCounterPrinter as a step of an existing pipeline.
          // The insertion point is specified by using the
          // 'registerVectorizerStartEPCallback' callback. To be more precise,
          // using this callback means that OpcodeCounterPrinter will be called
          // whenever the vectoriser is used (i.e. when using '-O{1|2|3|s}'.
          PB.registerVectorizerStartEPCallback(
              [](llvm::FunctionPassManager &PM,
                 llvm::OptimizationLevel Level) {
                PM.addPass(OpcodeCounterPrinter(llvm::errs()));
              });
          // #3 REGISTRATION FOR "FAM.getResult<OpcodeCounter>(Func)"
          // Register OpcodeCounter as an analysis pass. This is required so that
          // OpcodeCounterPrinter (or any other pass) can request the results
          // of OpcodeCounter.
          PB.registerAnalysisRegistrationCallback(
              [](FunctionAnalysisManager &FAM) {
                FAM.registerPass([&] { return OpcodeCounter(); });
              });
          }
        };
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOpcodeCounterPluginInfo();
}

//------------------------------------------------------------------------------
// Helper functions - implementation
//------------------------------------------------------------------------------
static void printOpcodeCounterResult(raw_ostream &OutS,
                                     const ResultOpcodeCounter &OpcodeMap) {
  OutS << "================================================="
               << "\n";
  OutS << "LLVM-TUTOR: OpcodeCounter results\n";
  OutS << "=================================================\n";
  const char *str1 = "OPCODE";
  const char *str2 = "#TIMES USED";
  OutS << format("%-20s %-10s\n", str1, str2);
  OutS << "-------------------------------------------------"
               << "\n";
  for (auto &Inst : OpcodeMap) {
    // Inst.first() is the key (StringRef), Inst.second is the value (unsigned)
    OutS << format("%-20s %-10lu\n", Inst.first().str().c_str(),
                           Inst.second);
  }
  OutS << "-------------------------------------------------"
               << "\n\n";
}
