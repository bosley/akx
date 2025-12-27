# AKX Nucleus System

**A unified builtin architecture where language features are standalone C files that can be compiled-in or loaded at runtime.**

## The Big Picture

```mermaid
flowchart TB
    subgraph "Build Time"
        M[manifest.txt<br/>SYMBOL FUNCTION PATH COMPILE_IN]
        S[nucleus/**/*.c<br/>Standalone builtin implementations]
        
        M --> CMake[CMake Parser]
        S --> CMake
        
        CMake --> |Reads manifest| Filter{COMPILE_IN?}
        Filter --> |1| Gen[Generate Registry Code]
        Filter --> |0| Skip[Skip - Runtime Only]
        
        Gen --> RegC[builtin_registry.c<br/>Auto-generated]
        Gen --> RegH[builtin_registry.h<br/>Auto-generated]
        
        RegC --> |Includes source inline| Compile[Compile libakx_nucleus.a]
        S --> |For COMPILE_IN=1| Compile
    end
    
    subgraph "Runtime Initialization"
        Init[akx_runtime_init]
        Init --> Boot[Register Bootstrap<br/>cjit-load-builtin]
        Boot --> Nuclei[Register Compiled Nuclei<br/>akx_rt_register_compiled_nuclei]
        Nuclei --> Map[Builtin Hash Map<br/>name → function pointer]
    end
    
    subgraph "Usage"
        Code[.akx code:<br/>if, lambda, let, set]
        Code --> |O1 lookup| Map
        Map --> Execute[Call C function]
    end
    
    Compile --> |Links into runtime| Init
    
    style M fill:#ff6b6b
    style CMake fill:#ffeaa7
    style RegC fill:#4ecdc4
    style Map fill:#96ceb4
```

## The Manifest: Single Source of Truth

```
nucleus/manifest.txt
────────────────────────────────────────────────
SYMBOL      FUNCTION        PATH              COMPILE_IN
if          if_impl         core/if.c         1
lambda      lambda_impl     core/lambda.c     1
+           add             math/add.c        0
println     println         io/println.c      0
```

**COMPILE_IN values:**
- `1` = Compile into binary (always available)
- `0` = Load at runtime via `cjit-load-builtin`

## CMake Code Generation Flow

```mermaid
sequenceDiagram
    participant M as manifest.txt
    participant C as CMakeLists.txt
    participant G as Generated Files
    participant B as Build System
    
    Note over C: Parse manifest at configure time
    C->>M: Read line by line
    M-->>C: if if_impl core/if.c 1
    M-->>C: lambda lambda_impl core/lambda.c 1
    M-->>C: + add math/add.c 0
    
    Note over C: Filter COMPILE_IN=1
    C->>C: if=1, lambda=1, +=0
    
    Note over C: Generate registration code
    C->>G: Write builtin_registry.h
    C->>G: Write builtin_registry.c
    
    Note over G: builtin_registry.c contains:
    Note over G: 1. #includes
    Note over G: 2. Inline nucleus sources
    Note over G: 3. Registration function
    
    G->>B: Compile libakx_nucleus.a
    B->>B: Link into akx_rt
```

## Generated Registry Structure

```mermaid
graph LR
    subgraph "builtin_registry.c"
        H[Headers & Includes]
        H --> S1[nucleus/core/if.c source]
        S1 --> S2[nucleus/core/lambda.c source]
        S2 --> S3[nucleus/core/let.c source]
        S3 --> Sn[... all COMPILE_IN=1 sources]
        Sn --> F[Registration Function]
    end
    
    F --> R1[Register if_impl as 'if']
    F --> R2[Register lambda_impl as 'lambda']
    F --> R3[Register let_impl as 'let']
    F --> Rn[... all compiled nuclei]
    
    style F fill:#4ecdc4
    style R1 fill:#96ceb4
    style R2 fill:#96ceb4
    style R3 fill:#96ceb4
```

## Runtime Registration

```mermaid
sequenceDiagram
    participant Main as main()
    participant RT as Runtime
    participant Boot as Bootstrap
    participant Reg as Registry
    participant Map as Builtin Map
    
    Main->>RT: akx_runtime_init()
    RT->>RT: Create builtin hash map
    
    RT->>Boot: akx_rt_register_bootstrap_builtins()
    Boot->>Map: Add "cjit-load-builtin" → builtin_cjit_load_builtin
    
    RT->>Reg: akx_rt_register_compiled_nuclei()
    Note over Reg: Auto-generated function
    
    Reg->>Map: Add "if" → if_impl
    Reg->>Map: Add "lambda" → lambda_impl
    Reg->>Map: Add "let" → let_impl
    Reg->>Map: Add "set" → set_impl
    Reg->>Map: Add "assert-true" → assert_true_impl
    Reg->>Map: Add "assert-false" → assert_false_impl
    Reg->>Map: Add "assert-eq" → assert_eq_impl
    Reg->>Map: Add "assert-ne" → assert_ne_impl
    
    RT-->>Main: Runtime ready (9 builtins)
```

## Execution Flow

```mermaid
flowchart LR
    subgraph "AKX Code"
        Code["(if 1 'yes' 'no')"]
    end
    
    subgraph "Evaluator"
        Parse[Parse: LIST with 'if' symbol]
        Lookup[Hash map lookup: 'if']
        Info[Get builtin_info_t*]
        Call[Call info→function]
    end
    
    subgraph "Compiled Nucleus"
        Func[if_impl C function]
        Eval[Evaluate arguments]
        Return[Return result cell]
    end
    
    Code --> Parse
    Parse --> Lookup
    Lookup --> Info
    Info --> Call
    Call --> Func
    Func --> Eval
    Eval --> Return
    Return --> Code
    
    style Lookup fill:#ffeaa7
    style Func fill:#4ecdc4
```

## The Two Deployment Modes

### Mode 1: Compiled-In (COMPILE_IN=1)

```mermaid
graph LR
    M[manifest.txt<br/>COMPILE_IN=1] --> CMake
    S[nucleus/core/if.c] --> CMake
    CMake --> Gen[Generate & Include]
    Gen --> Binary[akx binary]
    Binary --> Ready[Immediately Available]
    
    style Ready fill:#96ceb4
```

**Pros:** Instant availability, faster (no JIT)  
**Cons:** Slightly larger binary

### Mode 2: Runtime-Loadable (COMPILE_IN=0)

```mermaid
graph LR
    M[manifest.txt<br/>COMPILE_IN=0] --> Skip[Skip at build]
    Code["(cjit-load-builtin + :root 'nucleus/math/add.c')"] --> CJIT[JIT Compile]
    S[nucleus/math/add.c] --> CJIT
    CJIT --> Register[Register in map]
    Register --> Available[Now Available]
    
    style Available fill:#96ceb4
```

**Pros:** Smaller binary, load on demand, hot-reloadable  
**Cons:** Requires explicit loading

## Key Files

```
nucleus/
├── manifest.txt              ← Define all nuclei here
├── CMakeLists.txt           ← Parses manifest, generates code
├── core/if.c                ← Standalone nucleus implementation
├── core/lambda.c
└── ...

build/generated/
├── builtin_registry.h       ← Auto-generated header
└── builtin_registry.c       ← Auto-generated registration code

pkg/rt/
├── akx_rt.c                 ← Calls registration functions
└── akx_rt_builtins.c        ← Only bootstrap loader
```

## Adding a New Nucleus

```mermaid
flowchart LR
    Create[1. Create nucleus/math/pow.c] --> Add[2. Add to manifest.txt]
    Add --> Build[3. cmake .. && make]
    Build --> Done[4. Available!]
    
    style Done fill:#96ceb4
```

That's it! The system handles everything else automatically.

---

## Runtime Loading: `cjit-load-builtin` Options

For nuclei with `COMPILE_IN=0` (or any custom C code), use `cjit-load-builtin` to load at runtime:

### Basic Usage

```lisp
(cjit-load-builtin SYMBOL :root "path/to/file.c")
```

**Example:**
```lisp
(cjit-load-builtin + :root "nucleus/math/add.c" :as "add")
(+ 1 2 3)  ; Now available
```

### All Keyword Arguments

| Keyword | Type | Required | Description |
|---------|------|----------|-------------|
| `:root` | string | ✅ Yes | Path to main C file containing the builtin function |
| `:as` | string | ❌ No | Override C function name (default: same as SYMBOL) |
| `:include-paths` | list | ❌ No | Additional header search directories |
| `:implementation-files` | list | ❌ No | Extra C files to compile (relative to :root) |
| `:linker` | map | ❌ No | Linker configuration (libraries, defines, etc.) |

### Advanced Example: Multi-File Builtin with Dependencies

```lisp
(cjit-load-builtin http-client
    :root "builtins/http/client.c"
    
    :include-paths {
        "/usr/local/include"
        "/opt/custom/headers"
    }
    
    :implementation-files {
        "parser.c"
        "connection.c"
        "utils.c"
    }
    
    :linker {
        :library-paths { "/usr/local/lib" }
        :libraries { "curl" "ssl" "crypto" }
        :defines {
            ["DEBUG" "1"]
            ["VERSION" "\"1.0.0\""]
            ["ENABLE_LOGGING" ""]
        }
    })
```

### Linker Sub-Options

Within `:linker { ... }`:

| Sub-Keyword | Type | Description | Example |
|-------------|------|-------------|---------|
| `:library-paths` | list | Directories to search for libraries | `{ "/usr/local/lib" }` |
| `:libraries` | list | Library names (without `lib` prefix/extension) | `{ "m" "pthread" }` |
| `:defines` | list of pairs | Preprocessor defines as `[NAME VALUE]` | `{["DEBUG" "1"] ["FLAG" ""]}` |

### Common Patterns

**Load math operator:**
```lisp
(cjit-load-builtin + :root "nucleus/math/add.c" :as "add")
```

**Load with math library:**
```lisp
(cjit-load-builtin sqrt 
    :root "nucleus/math/sqrt.c"
    :linker { :libraries { "m" } })
```

**Load with custom defines:**
```lisp
(cjit-load-builtin logger
    :root "builtins/logger.c"
    :linker {
        :defines {
            ["LOG_LEVEL" "3"]
            ["ENABLE_COLORS" "1"]
        }
    })
```

**Multi-file builtin:**
```lisp
(cjit-load-builtin json
    :root "builtins/json/main.c"
    :implementation-files {
        "parser.c"
        "encoder.c"
    })
```

### How It Works

```mermaid
sequenceDiagram
    participant AKX as AKX Code
    participant RT as Runtime
    participant CJIT as JIT Compiler
    participant Map as Builtin Map
    
    AKX->>RT: (cjit-load-builtin + :root "add.c")
    RT->>RT: Parse keyword arguments
    RT->>CJIT: Create compilation unit
    RT->>CJIT: Add include paths
    RT->>CJIT: Add libraries & defines
    RT->>CJIT: Read & inject runtime API
    RT->>CJIT: Compile to machine code
    CJIT-->>RT: Function pointer
    RT->>Map: Register "+" → add function
    RT-->>AKX: Return 't' (success)
    
    AKX->>RT: (+ 1 2 3)
    RT->>Map: Lookup "+"
    Map-->>RT: Call add function
    RT-->>AKX: Return 6
```

### Hot Reloading

Even compiled-in nuclei can be replaced:

```lisp
(cjit-load-builtin if :root "nucleus/core/if_v2.c" :as "if_impl")
```

The old version (compiled or JIT) is immediately replaced with the new one!

