# AKX Runtime Model

## Overview

The AKX runtime is a dynamic evaluation engine that can expand its own capabilities at runtime through C JIT compilation.

## Bootstrap Architecture

The runtime ships with **exactly one native builtin**: `cjit-load-builtin`.

This single function is the bootstrap mechanism for the entire runtime. It:
1. Reads C source files containing builtin implementations
2. Compiles them at runtime using TCC (Tiny C Compiler)
3. Links them into the running process
4. Registers them as callable functions from AKX code

All other builtins (arithmetic, I/O, data structures, etc.) are loaded dynamically via this mechanism.

## How It Works

```akx
cjit-load-builtin add "builtins/add.c"
cjit-load-builtin println "builtins/println.c"

println "Hello from JIT!"
add 1 2 3
```

When `cjit-load-builtin` executes:
1. Reads the C source file
2. Injects runtime API declarations (see below)
3. Compiles the source using TCC
4. Resolves all runtime API symbols
5. Extracts the builtin function pointer
6. Registers it in the runtime's builtin map

Future calls to that builtin name invoke the JIT-compiled function directly.

## Runtime API

Builtins are C functions with this signature:
```c
akx_cell_t* builtin_name(akx_runtime_ctx_t *rt, akx_cell_t *args);
```

They receive **unevaluated arguments** and must explicitly evaluate them using `akx_rt_eval()`.

The runtime provides a complete API for:
- Cell allocation and manipulation
- Type checking and conversion
- List traversal
- Evaluation
- Error reporting with source location

For the full API specification and implementation details, see `akx_rt.c` and `akx_rt.h`.

## Hot Reloading

Calling `cjit-load-builtin` on an already-loaded builtin replaces it. The old CJIT unit is freed and the new one takes its place immediately.

## Error Reporting

Runtime errors capture source locations from cells and display them using the source view (sv) module, showing the exact line and column where the error occurred with visual context.

