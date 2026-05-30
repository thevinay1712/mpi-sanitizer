# MPISan Implementation Details

This document covers the low-level implementation details of the LLVM instrumentation pass and the runtime safety validation library.

---

## 1. LLVM IR Instrumentation Pass (`src/pass/MPISanPass.cpp`)

The LLVM pass runs inside the compiler optimization pipeline. It operates on the **Intermediate Representation (IR)** level to inject tracking hooks:

### A. Pass Registration & Injection
The pass registers itself using the new LLVM Pass Manager API:
```cpp
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
    return { LLVM_PLUGIN_API_VERSION, "MPISanPass", LLVM_VERSION_STRING,
        [](PassBuilder &PB) {
            // Automatically registers the pass to execute at the start of compilation
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                    MPM.addPass(MPISanPass());
                });
        }
    };
}
```

### B. IR Traversal & CallSite Detection
1. **Module & Function Loops:** It loops over all `Function`s in the compilation `Module`, skipping our sanitizer hook declarations (`__mpisan_...`).
2. **Instruction Scanning:** Within each basic block, it searches for `CallInst` (Call Instructions).
3. **Function Matching:** If the call targets an MPI function prefix (`MPI_`), it begins preparing the instrumentation hook.

### C. Hook Parameter Passing
For every detected MPI call site, the compiler generates a shadow hook call and copies the arguments:
```cpp
IRBuilder<> Builder(Call);
if (injectAfter) {
    Builder.SetInsertPoint(Call->getNextNode()); // Insert after for Isend/Irecv/Wait to capture requests
}

std::vector<Type*> ArgTypes;
std::vector<Value*> Args;
for (unsigned i = 0; i < Call->arg_size(); ++i) {
    ArgTypes.push_back(Call->getArgOperand(i)->getType());
    Args.push_back(Call->getArgOperand(i));
}

FunctionCallee Hook = M.getOrInsertFunction(HookName, FunctionType::get(Builder.getVoidTy(), ArgTypes, false));
Builder.CreateCall(Hook, Args);
```

---

## 2. Runtime Validation Logic (`src/runtime/mpisan_rt.cpp`)

The runtime is compiled as a thread-safe shared library (`libmpisan_rt.so`). It implements the following state tracking:

### A. Structures
```cpp
struct CommEvent {
    int type;      // 0 = Send, 1 = Recv
    int peer;      // dest or source
    int tag;
    int count;
    int type_size;
};

struct ActiveBuffer {
    const void* ptr;
    size_t size;
    bool is_write;
};
```

### B. Safety Check Specifications
* **Immediate Null-Pointer Checking:**
  For all intercepted operations, if `buf == nullptr` while `count > 0` (and `datatype` is not `MPI_DATATYPE_NULL`), an error is immediately printed and the program aborts.
* **Immediate Deadlock Detection:**
  If an `MPI_Send` dest matches the calling rank itself (`dest == g_my_rank`), a blocking send deadlock error is thrown immediately.
* **Buffer Aliasing Checks:**
  Asynchronous operations are registered in `g_active_requests` (keyed by `MPI_Request`). When a new `Isend`/`Irecv` is registered, its memory bounds `[ptr, ptr + size]` are compared against all currently active requests:
  ```cpp
  bool check_overlap(const void* ptr1, size_t size1, const void* ptr2, size_t size2) {
      size_t start1 = (size_t)ptr1;
      size_t end1 = start1 + size1;
      size_t start2 = (size_t)ptr2;
      size_t end2 = start2 + size2;
      return (start1 < end2 && start2 < end1);
  }
  ```
  If an overlap is found and at least one operation is a write (`is_write`), an immediate buffer aliasing error is flagged. When `MPI_Wait` is executed, the request is erased from `g_active_requests`.

* **FIFO Message Matching at Finalize:**
  Rank 0 mimics a standard MPI message queue. It matches sends to recvs based on peer ranks (supporting `MPI_ANY_SOURCE`) and tags (supporting `MPI_ANY_TAG`).
  
  ```cpp
  auto rcv_it = std::find_if(all_recvs.begin(), all_recvs.end(), [&](const GlobalCommEvent& rcv) {
      if (rcv.matched) return false;
      bool rank_matches = (rcv.origin_rank == snd.peer) && 
                          (rcv.peer == snd.origin_rank || rcv.peer == MPI_ANY_SOURCE);
      bool tag_matches = (rcv.tag == snd.tag) || (rcv.tag == MPI_ANY_TAG);
      return rank_matches && tag_matches;
  });
  ```
  Once matched, base type sizes (`type_size`) and total payload bounds are checked to guarantee no type signature or buffer overflow bugs exist in the communication pipeline.
