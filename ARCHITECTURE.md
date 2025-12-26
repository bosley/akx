# AKX Architecture Overview

**A self-extending runtime where the language grows itself.**

## The Big Picture

```mermaid
graph TB
    subgraph Runtime[AKX Runtime Core]
        Bootstrap[Single Native Builtin<br/>cjit-load-builtin]
        Eval[Expression Evaluator]
        Cells[Cell Memory Model]
    end
    
    subgraph UserSpace[User Space]
        AKXCode[.akx Files]
        CBuiltins[C Builtins]
    end
    
    subgraph JIT[JIT Compiler Layer]
        CJIT[ak24 CJIT]
        Symbols[Runtime API Injection]
        Link[Dynamic Linking]
    end
    
    AKXCode -->|load| Bootstrap
    Bootstrap -->|compile| CBuiltins
    CBuiltins -->|through| JIT
    JIT -->|register| Eval
    Eval -->|manipulates| Cells
    Eval -->|calls| CBuiltins
    
    style Bootstrap fill:#ff6b6b
    style Eval fill:#4ecdc4
    style Cells fill:#45b7d1
    style CBuiltins fill:#96ceb4
    style JIT fill:#ffeaa7
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

## The Minimal Core Principle

```mermaid
pie title Runtime Composition
    "Native Code (Core)" : 5
    "Bootstrap Builtin" : 1
    "JIT-Loaded Builtins" : 94
```

**One native builtin. Everything else is JIT-compiled at runtime.**

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

