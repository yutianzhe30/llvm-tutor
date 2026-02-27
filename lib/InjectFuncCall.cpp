//========================================================================
// FILE:
//    InjectFuncCall.cpp
//
// DESCRIPTION:
//    For each function defined in the input IR module, InjectFuncCall inserts
//    a call to printf (from the C standard I/O library). The injected IR code
//    corresponds to the following function call in ANSI C:
//    ```C
//      printf("(llvm-tutor) Hello from: %s\n(llvm-tutor)   number of arguments: %d\n",
//             FuncName, FuncNumArgs);
//    ```
//    This code is inserted at the beginning of each function, i.e. before any
//    other instruction is executed.
//
//    To illustrate, for `void foo(int a, int b, int c)`, the code added by InjectFuncCall
//    will generated the following output at runtime:
//    ```
//    (llvm-tutor) Hello World from: foo
//    (llvm-tutor)   number of arguments: 3
//    ```
//
//    This example demonstrates:
//    1. How to modify the IR (Transformation Pass).
//    2. How to use IRBuilder to create instructions.
//    3. How to inject function calls into existing code.
//    4. How to handle Global Variables (like format strings).
//
// USAGE:
//      $ opt -load-pass-plugin <BUILD_DIR>/lib/libInjectFunctCall.so `\`
//        -passes=-"inject-func-call" <bitcode-file>
//
// License: MIT
//========================================================================
#include "InjectFuncCall.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Passes/PassBuilder.h"

using namespace llvm;

#define DEBUG_TYPE "inject-func-call"

//-----------------------------------------------------------------------------
// InjectFuncCall implementation
//-----------------------------------------------------------------------------
bool InjectFuncCall::runOnModule(Module &M) {
  bool InsertedAtLeastOnePrintf = false;

  auto &CTX = M.getContext();
  // Get the pointer type for the current context.
  // In opaque pointer mode (default in recent LLVM), all pointers are the same type.
  PointerType *PrintfArgTy = PointerType::getUnqual(CTX);

  // STEP 1: Inject the declaration of printf
  // ----------------------------------------
  // We need to tell LLVM about the 'printf' function so we can call it.
  // It corresponds to the C declaration: int printf(char *, ...)
  FunctionType *PrintfTy = FunctionType::get(
      IntegerType::getInt32Ty(CTX), // Return type: i32
      PrintfArgTy,                  // First argument: char* (pointer)
      /*IsVarArgs=*/true);          // Variadic arguments: ...

  // 'getOrInsertFunction' will either return the existing function if it was
  // already declared, or create a new declaration.
  FunctionCallee Printf = M.getOrInsertFunction("printf", PrintfTy);

  // Set attributes for the printf function.
  // 'dyn_cast' checks if the callee is indeed a Function (it could be something else if name collision).
  Function *PrintfF = dyn_cast<Function>(Printf.getCallee());
  if (PrintfF) {
      PrintfF->setDoesNotThrow(); // printf doesn't throw C++ exceptions
      PrintfF->addParamAttr(0, llvm::Attribute::getWithCaptureInfo(
                                   M.getContext(), llvm::CaptureInfo::none()));
      PrintfF->addParamAttr(0, Attribute::ReadOnly); // First arg is read-only
  }


  // STEP 2: Inject a global variable that will hold the printf format string
  // ------------------------------------------------------------------------
  // We create a constant string literal for the format string.
  llvm::Constant *PrintfFormatStr = llvm::ConstantDataArray::getString(
      CTX, "(llvm-tutor) Hello from: %s\n(llvm-tutor)   number of arguments: %d\n");

  // Create a global variable in the module to hold this string.
  Constant *PrintfFormatStrVar =
      M.getOrInsertGlobal("PrintfFormatStr", PrintfFormatStr->getType());

  // Initialize the global variable with the string data.
  if (auto *GV = dyn_cast<GlobalVariable>(PrintfFormatStrVar)) {
      GV->setInitializer(PrintfFormatStr);
  }

  // STEP 3: For each function in the module, inject a call to printf
  // ----------------------------------------------------------------
  for (auto &F : M) {
    // Skip function declarations (functions without a body, e.g. external libraries)
    if (F.isDeclaration())
      continue;

    // Get an IR builder. Sets the insertion point to the top of the function
    // (the first instruction of the entry block).
    IRBuilder<> Builder(&*F.getEntryBlock().getFirstInsertionPt());

    // Create a global string variable for the function name.
    // IRBuilder handles the details of creating the global variable and getting a pointer to it.
    auto FuncName = Builder.CreateGlobalString(F.getName());

    // Prepare arguments for printf.
    // The format string is a global variable, so we need a pointer to it.
    // In opaque pointers, we just need the pointer.
    llvm::Value *FormatStrPtr = PrintfFormatStrVar;

    // The following is visible only if you pass -debug on the command line
    // *and* you have an assert build.
    LLVM_DEBUG(dbgs() << " Injecting call to printf inside " << F.getName()
                      << "\n");

    // Finally, inject a call to printf.
    // Args: {Format String, Function Name (string), Number of Arguments (int)}
    Builder.CreateCall(
        Printf, {FormatStrPtr, FuncName, Builder.getInt32(F.arg_size())});

    InsertedAtLeastOnePrintf = true;
  }

  return InsertedAtLeastOnePrintf;
}

PreservedAnalyses InjectFuncCall::run(llvm::Module &M,
                                       llvm::ModuleAnalysisManager &) {
  // Delegate the work to a helper method.
  bool Changed =  runOnModule(M);

  // If we modified the module, we must indicate that analyses are not preserved.
  // (Or carefully specify which ones are preserved).
  // 'PreservedAnalyses::none()' is the safest default for transformation passes.
  return (Changed ? llvm::PreservedAnalyses::none()
                  : llvm::PreservedAnalyses::all());
}


//-----------------------------------------------------------------------------
// New PM Registration
//-----------------------------------------------------------------------------
llvm::PassPluginLibraryInfo getInjectFuncCallPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "inject-func-call", LLVM_VERSION_STRING,
          [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "inject-func-call") {
                    MPM.addPass(InjectFuncCall());
                    return true;
                  }
                  return false;
                });
          }};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getInjectFuncCallPluginInfo();
}
