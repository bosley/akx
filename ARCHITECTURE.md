# AKX Architecture Overview

**A self-extending runtime where the language grows itself.**

## The Big Picture

```mermaid
graph TB
    subgraph Build[Build Time]
        Manifest[nucleus/manifest.txt]
        NucleusSrc[nucleus/**/*.c]
        CMakeGen[CMake Generator]
        
        Manifest --> CMakeGen
        NucleusSrc --> CMakeGen
        CMakeGen --> CompiledNuclei[Compiled Nuclei<br/>if, lambda, let, set, assert-*]
    end
    
    subgraph Runtime[AKX Runtime Core]
        Bootstrap[Bootstrap Builtin<br/>cjit-load-builtin]
        CompiledNuclei --> Eval[Expression Evaluator]
        Bootstrap --> Eval
        Eval --> Cells[Cell Memory Model]
    end
    
    subgraph UserSpace[User Space]
        AKXCode[.akx Files]
        RuntimeNuclei[Runtime Nuclei<br/>+, -, *, /, print, println]
    end
    
    subgraph JIT[JIT Compiler Layer]
        CJIT[ak24 CJIT]
        Symbols[Runtime API Injection]
        Link[Dynamic Linking]
    end
    
    AKXCode -->|load| Bootstrap
    Bootstrap -->|compile| RuntimeNuclei
    RuntimeNuclei -->|through| JIT
    JIT -->|register| Eval
    Eval -->|manipulates| Cells
    Eval -->|calls| RuntimeNuclei
    
    style Bootstrap fill:#ff6b6b
    style Eval fill:#4ecdc4
    style Cells fill:#45b7d1
    style CompiledNuclei fill:#96ceb4
    style RuntimeNuclei fill:#96ceb4
    style JIT fill:#ffeaa7
    style CMakeGen fill:#ffeaa7
```

## Bootstrap Flow

```mermaid
sequenceDiagram
    participant User as .akx File
    participant RT as Runtime
    participant CJIT as JIT Compiler
    participant Builtin as New Builtin
    
    User->>RT: (cjit-load-builtin add :root "add.c")
    RT->>CJIT: Read add.c + inject API
    CJIT->>CJIT: Compile to machine code
    CJIT->>RT: Return function pointer
    RT->>RT: Register "add" in builtin map
    
    User->>RT: (add 1 2 3)
    RT->>Builtin: Call with unevaluated args
    Builtin->>RT: Use runtime API
    Builtin->>RT: Return result cell
    RT->>User: 6
```

## Cell Memory Model

```mermaid
graph LR
    subgraph Expression[Expression: add 1 2 3]
        List[LIST Cell]
        Add[SYMBOL: add]
        One[INTEGER: 1]
        Two[INTEGER: 2]
        Three[INTEGER: 3]
    end
    
    List -->|list_head| Add
    Add -->|next| One
    One -->|next| Two
    Two -->|next| Three
    
    style List fill:#ff6b6b
    style Add fill:#4ecdc4
    style One fill:#96ceb4
    style Two fill:#96ceb4
    style Three fill:#96ceb4
```

## The Power Move: Hot Reloading

```mermaid
stateDiagram-v2
    [*] --> LoadV1: cjit-load-builtin counter v1.c
    LoadV1 --> Running: counter() = 1, 2, 3...
    Running --> Reload: cjit-load-builtin counter v2.c
    Reload --> PreserveState: reload_hook(old_state)
    PreserveState --> RunningV2: counter() = 4, 5, 6...
    RunningV2 --> [*]: Runtime shutdown
    
    note right of PreserveState
        State preserved across versions
        No restart needed
    end note
```

## Compilation Pipeline

```mermaid
flowchart LR
    subgraph Input
        Root[Root C File]
        Impl[Implementation Files]
        Headers[Include Paths]
    end
    
    subgraph Compile[JIT Compilation]
        API[Inject Runtime API]
        Combine[Combine Sources]
        Defines[Apply Defines]
        Compile2[Compile to Machine Code]
    end
    
    subgraph Link[Dynamic Linking]
        Symbols[Link Runtime Symbols]
        Libs[Link Libraries]
        Relocate[Relocate Code]
    end
    
    subgraph Register[Registration]
        Extract[Extract Function Pointer]
        Hooks[Find Lifecycle Hooks]
        Map[Add to Builtin Map]
    end
    
    Input --> Compile
    Compile --> Link
    Link --> Register
    Register --> Callable[Callable from AKX]
    
    style Compile fill:#ffeaa7
    style Link fill:#ff6b6b
    style Register fill:#4ecdc4
    style Callable fill:#96ceb4
```

## Keyword Argument System

```mermaid
graph TD
    Call[cjit-load-builtin name ...]
    
    Call --> Root[:root path]
    Call --> Includes[:include-paths]
    Call --> Impls[:implementation-files]
    Call --> Linker[:linker]
    
    Linker --> LibPaths[:library-paths]
    Linker --> Libs[:libraries]
    Linker --> Defines[:defines]
    
    Root --> Compiler[Compiler]
    Includes --> Compiler
    Impls --> Compiler
    LibPaths --> Compiler
    Libs --> Compiler
    Defines --> Compiler
    
    Compiler --> Binary[Executable Function]
    
    style Root fill:#ff6b6b
    style Linker fill:#4ecdc4
    style Compiler fill:#ffeaa7
    style Binary fill:#96ceb4
```

## The Nucleus System: Unified Builtin Architecture

AKX uses a **nucleus architecture** where all builtins are standalone C files that can be either compiled into the binary or loaded at runtime via CJIT.

### Nucleus Directory Structure

```
nucleus/
├── manifest.txt          # Single source of truth for all nuclei
├── CMakeLists.txt       # Auto-generates registration code
├── core/                # Core language primitives
│   ├── if.c
│   ├── lambda.c
│   ├── let.c
│   └── set.c
├── assert/              # Assertion functions
│   ├── assert_true.c
│   ├── assert_false.c
│   ├── assert_eq.c
│   └── assert_ne.c
├── math/                # Mathematical operators
│   ├── add.c
│   ├── sub.c
│   ├── mul.c
│   ├── div.c
│   ├── mod.c
│   └── eq.c
└── io/                  # I/O operations
    ├── print.c
    └── println.c
```

### The Manifest

`nucleus/manifest.txt` defines all available nuclei:

```
# Format: SYMBOL C_FUNCTION_NAME RELATIVE_PATH COMPILE_IN
# COMPILE_IN: 1=compile into binary, 0=load at runtime

# Core language (always compiled in)
if          if_impl         core/if.c           1
lambda      lambda_impl     core/lambda.c       1
let         let_impl        core/let.c          1
set         set_impl        core/set.c          1

# Math operators (runtime loadable)
+           add             math/add.c          0
-           sub             math/sub.c          0
```

### Build-Time Code Generation

```mermaid
flowchart LR
    Manifest[nucleus/manifest.txt] --> CMake[CMake Parser]
    Sources[nucleus/**/*.c] --> CMake
    
    CMake --> GenH[builtin_registry.h]
    CMake --> GenC[builtin_registry.c]
    
    GenC --> Compile[Compile nucleus]
    Sources --> Compile
    
    Compile --> Library[libakx_nucleus.a]
    Library --> Runtime[Link into runtime]
    
    style Manifest fill:#ff6b6b
    style CMake fill:#ffeaa7
    style Library fill:#4ecdc4
```

At build time, CMake:
1. Parses `manifest.txt`
2. Identifies which nuclei have `COMPILE_IN=1`
3. Generates `builtin_registry.c` with registration code
4. Includes nucleus source files inline
5. Creates `libakx_nucleus.a` library

### Runtime Initialization

```mermaid
sequenceDiagram
    participant Main as main()
    participant RT as Runtime Init
    participant Bootstrap as Bootstrap Loader
    participant Nuclei as Compiled Nuclei
    participant Map as Builtin Map
    
    Main->>RT: akx_runtime_init()
    RT->>Bootstrap: Register cjit-load-builtin
    Bootstrap->>Map: Add bootstrap loader
    RT->>Nuclei: akx_rt_register_compiled_nuclei()
    Nuclei->>Map: Add if, lambda, let, set, assert-*
    RT->>Main: Runtime ready
    
    Note over Map: 9 builtins available<br/>(1 bootstrap + 8 nuclei)
```

### Deployment Flexibility

The same nucleus can be used in three ways:

#### 1. Compiled-In (Fast, Static)
```c
// Set COMPILE_IN=1 in manifest.txt
// Nucleus compiled into binary at build time
(if 1 "yes" "no")  // Immediate, no loading needed
```

#### 2. Runtime-Loaded (Flexible, Dynamic)
```lisp
// Set COMPILE_IN=0 in manifest.txt
// Load nucleus at runtime via CJIT
(cjit-load-builtin + :root "nucleus/math/add.c" :as "add")
(+ 1 2 3)  // Now available
```

#### 3. Hot-Reloaded (Development, Live Updates)
```lisp
// Even compiled-in nuclei can be replaced
(cjit-load-builtin if :root "nucleus/core/if_v2.c" :as "if_impl")
// Old compiled version replaced with new JIT version
```

### Current Configuration

```mermaid
pie title Builtin Distribution
    "Compiled-In Nuclei" : 8
    "Bootstrap Loader" : 1
    "Runtime-Loadable Nuclei" : 7
```

**Compiled-In (8):** if, lambda, let, set, assert-true, assert-false, assert-eq, assert-ne  
**Runtime-Loadable (7):** +, -, *, /, %, =, print, println  
**Bootstrap (1):** cjit-load-builtin

### Benefits

1. **Single Source of Truth** - Each builtin is one `.c` file
2. **Flexible Deployment** - Manifest controls compilation strategy
3. **Automatic Registration** - CMake generates all registration code
4. **Hot Reload Everything** - Even compiled nuclei can be replaced
5. **No Duplication** - Same code works compiled or JIT-loaded

## Module Lifecycle

```mermaid
stateDiagram-v2
    [*] --> Load: cjit-load-builtin
    Load --> Init: name_init()
    Init --> Active: Ready to call
    Active --> Active: Function calls
    Active --> Reload: Hot reload
    Reload --> ReloadHook: name_reload(old_state)
    ReloadHook --> Active: Continue with new code
    Active --> Deinit: Runtime shutdown
    Deinit --> [*]: name_deinit()
    
    note right of ReloadHook
        Preserves state across versions
    end note
```

## What Makes This Special

### Traditional Language
```mermaid
graph LR
    Source[Source Code] --> Compiler
    Compiler --> Binary
    Binary --> Run[Execute]
    
    style Compiler fill:#ddd
    style Binary fill:#ddd
```

### AKX
```mermaid
graph LR
    Runtime[Tiny Runtime] --> Load[Load Language Features]
    Load --> Compile[JIT Compile]
    Compile --> Execute[Execute]
    Execute --> Reload[Reload Features]
    Reload --> Compile
    
    style Runtime fill:#ff6b6b
    style Compile fill:#ffeaa7
    style Execute fill:#4ecdc4
    style Reload fill:#96ceb4
```

**The language defines itself. At runtime. Repeatedly.**

## Tail Call Optimization

AKX implements a trampoline-based tail call optimization system that eliminates recursion depth limits:

```mermaid
flowchart TD
    Start[Lambda Invocation] --> EvalBody[Evaluate Lambda Body]
    EvalBody --> CheckTail{Last Expression?}
    CheckTail -->|Yes| TailEval[Use Tail Evaluation]
    CheckTail -->|No| RegEval[Regular Evaluation]
    
    TailEval --> CheckResult{Result is Lambda Call?}
    CheckResult -->|Yes| CreateCont[Create Continuation]
    CheckResult -->|No| Return[Return Value]
    
    CreateCont --> Trampoline[Trampoline Loop]
    Trampoline --> PopScope[Pop Current Scope]
    PopScope --> PushScope[Push New Scope]
    PushScope --> BindArgs[Bind Arguments]
    BindArgs --> EvalBody
    
    RegEval --> Return
    
    style TailEval fill:#ff6b6b
    style CreateCont fill:#4ecdc4
    style Trampoline fill:#ffeaa7
    style Return fill:#96ceb4
```

### How It Works

1. **Continuation Detection**: When evaluating the last expression in a lambda body, the runtime uses `akx_rt_eval_tail()` instead of regular evaluation
2. **Continuation Creation**: If the tail expression is a lambda call, a continuation cell is created instead of invoking the lambda
3. **Trampoline Loop**: The `akx_rt_invoke_lambda()` function runs an iterative loop that:
   - Invokes the lambda and gets the result
   - If the result is a continuation, extracts the next lambda and arguments
   - Pops the old scope and pushes a new scope
   - Repeats until a non-continuation result is obtained
4. **No C Stack Recursion**: All tail calls are handled iteratively, eliminating stack overflow

## Creating a New Nucleus

Adding a new builtin to AKX is simple:

### 1. Create the Nucleus File

```c
// nucleus/math/pow.c
akx_cell_t *pow_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
    akx_cell_t *base_cell = akx_rt_list_nth(args, 0);
    akx_cell_t *exp_cell = akx_rt_list_nth(args, 1);
    
    akx_cell_t *base = akx_rt_eval_and_assert(rt, base_cell, 
        AKX_TYPE_INTEGER_LITERAL, "pow: base must be integer");
    if (!base) return NULL;
    
    akx_cell_t *exp = akx_rt_eval_and_assert(rt, exp_cell,
        AKX_TYPE_INTEGER_LITERAL, "pow: exponent must be integer");
    if (!exp) {
        akx_rt_free_cell(rt, base);
        return NULL;
    }
    
    int result = 1;
    int b = akx_rt_cell_as_int(base);
    int e = akx_rt_cell_as_int(exp);
    
    for (int i = 0; i < e; i++) {
        result *= b;
    }
    
    akx_rt_free_cell(rt, base);
    akx_rt_free_cell(rt, exp);
    
    akx_cell_t *ret = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
    akx_rt_set_int(rt, ret, result);
    return ret;
}
```

### 2. Add to Manifest

```txt
# nucleus/manifest.txt
pow         pow_impl        math/pow.c          0
```

### 3. Rebuild

```bash
cd build && cmake .. && make
```

### 4. Use It

```lisp
# Load at runtime
(cjit-load-builtin pow :root "nucleus/math/pow.c" :as "pow_impl")
(let result (pow 2 8))  # 256

# Or set COMPILE_IN=1 to have it always available
```

That's it! The nucleus is now part of the language.

## Data Flow

```mermaid
flowchart TB
    subgraph Parse[Parser Layer]
        File[.akx File]
        Cells1[Cell AST]
    end
    
    subgraph Runtime[Runtime Layer]
        Eval[Evaluator]
        Builtins[Builtin Map]
    end
    
    subgraph Execution[Execution Layer]
        Native[Native Builtin]
        JIT[JIT Builtins]
    end
    
    File --> Cells1
    Cells1 --> Eval
    Eval --> Builtins
    Builtins --> Native
    Builtins --> JIT
    JIT --> Eval
    Native --> Eval
    
    style Parse fill:#e8f4f8
    style Runtime fill:#fff4e6
    style Execution fill:#f0f8e8
```

