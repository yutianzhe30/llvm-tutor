//=============================================================================
// FILE:
//    HelloWorld.cpp
//
// DESCRIPTION:
//    Visits all functions in a module, prints their names and the number of
//    arguments via stderr. Strictly speaking, this is an analysis pass (i.e.
//    the functions are not modified). However, in order to keep things simple
//    there's no 'print' method here (every analysis pass should implement it).
//
//    This pass demonstrates:
//    1. The basic structure of an LLVM Pass using the New Pass Manager.
//    2. How to traverse functions in a module (implicitly handled by the Pass Manager).
//    3. Accessing basic function properties like name and argument count.
//
// USAGE:
//    New PM
//      opt -load-pass-plugin=libHelloWorld.dylib -passes="hello-world" `\`
//        -disable-output <input-llvm-file>
//
//
// License: MIT
//=============================================================================

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

//-----------------------------------------------------------------------------
// HelloWorld implementation
//-----------------------------------------------------------------------------
// No need to expose the internals of the pass to the outside world - keep
// everything in an anonymous namespace.
namespace {

// This method implements what the pass does
void visitor(Function &F) {
    // 'errs()' is an LLVM output stream corresponding to stderr.
    // 'F.getName()' returns the name of the function as a string.
    errs() << "(llvm-tutor) Hello from: "<< F.getName() << "\n";

    // 'F.arg_size()' returns the number of arguments the function takes.
    errs() << "(llvm-tutor)   number of arguments: " << F.arg_size() << "\n";
}

// New PM implementation
// LLVM passes are normally classes/structs.
// 'PassInfoMixin' is a CRTP (Curiously Recurring Template Pattern) mixin that
// provides boilerplate code for pass identification.
struct HelloWorld : PassInfoMixin<HelloWorld> {
  // Main entry point, takes IR unit to run the pass on (&F) and the
  // corresponding pass manager (to be queried if need be).
  // The 'run' method is required for all new PM passes.
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &) {
    visitor(F);

    // 'PreservedAnalyses::all()' means that this pass preserved all previous
    // analyses. This is true because this pass does not modify the IR.
    // If we modified the IR, we would need to specify which analyses might
    // be invalidated.
    return PreservedAnalyses::all();
  }

  // Without isRequired returning true, this pass will be skipped for functions
  // decorated with the optnone LLVM attribute. Note that clang -O0 decorates
  // all functions with optnone.
  static bool isRequired() { return true; }
};
} // namespace

//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
// This function provides the plugin information to the Pass Manager.
llvm::PassPluginLibraryInfo getHelloWorldPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "HelloWorld", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            // Register a callback to parse the pass pipeline.
            // When the user specifies a pass name in '-passes=...', this callback
            // is invoked.
            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "hello-world") {
                    // Add our pass to the Function Pass Manager
                    FPM.addPass(HelloWorld());
                    return true;
                  }
                  return false;
                });
          }};
}

// This is the core interface for pass plugins. It guarantees that 'opt' will
// be able to recognize HelloWorld when added to the pass pipeline on the
// command line, i.e. via '-passes=hello-world'
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getHelloWorldPluginInfo();
}
