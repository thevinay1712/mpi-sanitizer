#include "llvm/IR/PassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

namespace {
struct MPISanPass : public PassInfoMixin<MPISanPass> {
    PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
        bool Modified = false;
        
        for (Function &F : M) {
            if (F.getName().starts_with("__mpisan")) continue;

            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    if (auto *Call = dyn_cast<CallInst>(&I)) {
                        Function *CalledF = Call->getCalledFunction();
                        if (CalledF && CalledF->hasName()) {
                            StringRef Name = CalledF->getName();
                            
                            // Check for functions we intercept (supports standard MPI_ and PMPI_ profiling wrappers)
                            if (Name.starts_with("MPI_") || Name.starts_with("PMPI_")) {
                                StringRef BaseName = Name;
                                if (Name.starts_with("PMPI_")) {
                                    BaseName = Name.substr(1); // Strip the 'P' prefix -> "MPI_..."
                                }
                                
                                bool injectAfter = false;
                                StringRef HookName = "";
                                
                                if (BaseName == "MPI_Init") { HookName = "__mpisan_init"; injectAfter = true; }
                                else if (BaseName == "MPI_Finalize") { HookName = "__mpisan_finalize"; }
                                else if (BaseName == "MPI_Send") { HookName = "__mpisan_check_send"; }
                                else if (BaseName == "MPI_Recv") { HookName = "__mpisan_check_recv"; }
                                else if (BaseName == "MPI_Bcast") { HookName = "__mpisan_check_bcast"; }
                                else if (BaseName == "MPI_Reduce") { HookName = "__mpisan_check_reduce"; }
                                else if (BaseName == "MPI_Isend") { HookName = "__mpisan_check_isend"; injectAfter = true; }
                                else if (BaseName == "MPI_Irecv") { HookName = "__mpisan_check_irecv"; injectAfter = true; }
                                else if (BaseName == "MPI_Wait") { HookName = "__mpisan_check_wait"; injectAfter = true; }
                                
                                if (!HookName.empty()) {
                                    IRBuilder<> Builder(Call);
                                    if (injectAfter) {
                                        Builder.SetInsertPoint(Call->getNextNode());
                                    }
                                    
                                    std::vector<Type*> ArgTypes;
                                    std::vector<Value*> Args;
                                    for (unsigned i = 0; i < Call->arg_size(); ++i) {
                                        ArgTypes.push_back(Call->getArgOperand(i)->getType());
                                        Args.push_back(Call->getArgOperand(i));
                                    }
                                    
                                    FunctionCallee Hook = M.getOrInsertFunction(
                                        HookName,
                                        FunctionType::get(Builder.getVoidTy(), ArgTypes, false)
                                    );
                                    
                                    Builder.CreateCall(Hook, Args);
                                    Modified = true;
                                }
                            }
                        }
                    }
                }
            }
        }
        return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};
}

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return { LLVM_PLUGIN_API_VERSION, "MPISanPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            PB.registerPipelineParsingCallback(
                [](StringRef Name, ModulePassManager &MPM, ArrayRef<PassBuilder::PipelineElement>) {
                    if (Name == "mpisan") { MPM.addPass(MPISanPass()); return true; }
                    return false;
                });
            // Automatically run the pass during normal compilation!
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(MPISanPass());
                });
        }
    };
}
