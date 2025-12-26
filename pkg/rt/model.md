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
(cjit-load-builtin add :root "builtins/add.c")
(cjit-load-builtin println :root "builtins/println.c")

(println "Hello from JIT!")
(add 1 2 3)
```

When `cjit-load-builtin` executes:
1. Parses keyword arguments (`:root`, `:include-paths`, `:implementation-files`, `:linker`)
2. Creates a CJIT compilation unit with specified compiler options
3. Adds include paths, library paths, libraries, and preprocessor defines
4. Reads the root C source file
5. Injects runtime API declarations
6. Reads and adds any additional implementation files
7. Compiles all sources using TCC
8. Resolves all runtime API symbols
9. Extracts the builtin function pointer and lifecycle hooks
10. Registers it in the runtime's builtin map

Future calls to that builtin name invoke the JIT-compiled function directly.

## Advanced Usage

### Multi-File Builtins with Custom Compilation

```akx
(cjit-load-builtin complex-builtin
    :root "builtins/complex/main.c"
    :include-paths {
        "/usr/local/include"
        "/opt/custom/headers"
    }
    :implementation-files {
        "helper.c"
        "utils.c"
        "algorithms.c"
    }
    :linker {
        :library-paths { "/usr/local/lib" }
        :libraries { "m" "pthread" }
        :defines {
            ["DEBUG" "1"]
            ["VERSION" "\"2.0.0\""]
            ["ENABLE_LOGGING" ""]
        }
    })
```

### Keyword Arguments

- **`:root`** (required) - Path to the main C file containing the builtin function and optional lifecycle hooks
- **`:include-paths`** (optional) - Curly-brace list of directory paths to search for header files
- **`:implementation-files`** (optional) - Curly-brace list of additional C files to compile (paths relative to root file's directory)
- **`:linker`** (optional) - Curly-brace list containing linker configuration:
  - **`:library-paths`** - Directories to search for libraries
  - **`:libraries`** - Library names to link (without `lib` prefix or extension)
  - **`:defines`** - Preprocessor defines as `[name value]` pairs (empty string for flag-only defines)

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

## Runtime API Functions

| Function | Purpose |
|----------|---------|
| `akx_runtime_init` | Initialize the runtime context and register the bootstrap builtin |
| `akx_runtime_deinit` | Clean up runtime resources, free CJIT units and builtins |
| `akx_runtime_start` | Execute a list of top-level expressions |
| `akx_runtime_get_current_scope` | Get the current runtime scope context |
| `akx_runtime_get_errors` | Retrieve the linked list of runtime errors |
| `akx_runtime_load_builtin_ex` | Load and compile a C builtin with custom compilation options |
| `akx_rt_alloc_cell` | Allocate a new cell of the specified type |
| `akx_rt_free_cell` | Free a cell and its resources |
| `akx_rt_set_symbol` | Set a cell's value to an interned symbol |
| `akx_rt_set_int` | Set a cell's value to an integer |
| `akx_rt_set_real` | Set a cell's value to a floating-point number |
| `akx_rt_set_string` | Set a cell's value to a string |
| `akx_rt_set_list` | Set a cell's value to a list head |
| `akx_rt_cell_is_type` | Check if a cell matches a specific type |
| `akx_rt_cell_as_symbol` | Extract symbol value from a cell |
| `akx_rt_cell_as_int` | Extract integer value from a cell |
| `akx_rt_cell_as_real` | Extract floating-point value from a cell |
| `akx_rt_cell_as_string` | Extract string value from a cell |
| `akx_rt_cell_as_list` | Extract list head from a cell |
| `akx_rt_cell_next` | Get the next sibling cell in a list |
| `akx_rt_list_length` | Count the number of elements in a list |
| `akx_rt_list_nth` | Get the nth element of a list |
| `akx_rt_list_append` | Append an item to the end of a list |
| `akx_rt_get_scope` | Get the runtime's scope context |
| `akx_rt_scope_set` | Set a variable in the current scope |
| `akx_rt_scope_get` | Get a variable from the current scope |
| `akx_rt_error` | Report a runtime error with a message |
| `akx_rt_error_at` | Report a runtime error with source location from a cell |
| `akx_rt_error_fmt` | Report a formatted runtime error |
| `akx_rt_eval` | Evaluate an expression and return the result |
| `akx_rt_eval_list` | Evaluate all elements in a list and return results |
| `akx_rt_eval_and_assert` | Evaluate and assert the result is of expected type |

## Hot Reloading

Calling `cjit-load-builtin` on an already-loaded builtin replaces it. The old CJIT unit is freed and the new one takes its place immediately.

## Error Reporting

Runtime errors capture source locations from cells and display them using the source view (sv) module, showing the exact line and column where the error occurred with visual context.
