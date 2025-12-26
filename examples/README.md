# AKX Examples

This directory contains examples demonstrating how to define and load runtime builtins using the AKX JIT compilation system.

## Overview

AKX has a unique runtime model: it ships with **only one native builtin** (`cjit-load-builtin`), which is the bootstrap mechanism for the entire runtime. All other functionality is loaded dynamically by compiling C source files at runtime using JIT compilation.

This means you can extend the language with new functions written in C, compiled and linked on-the-fly, without restarting the runtime.

## How It Works

### The Bootstrap Builtin

The runtime provides a single native function:

```akx
(cjit-load-builtin <name> :root <path> [options...])
```

This function:
1. Reads C source files
2. Injects the AKX runtime API declarations
3. Compiles the code using JIT compilation
4. Links it into the running process
5. Registers the function as callable from AKX code

### Basic Example

```akx
(cjit-load-builtin println :root "examples/builtins/println.c")
(cjit-load-builtin add :root "examples/builtins/add.c")

(println "Hello from JIT!")
(add 1 2 3 4 5)
```

After loading, `println` and `add` become first-class functions in the runtime.

## Builtin API

### Function Signature

Every builtin is a C function with this signature:

```c
akx_cell_t* function_name(akx_runtime_ctx_t *rt, akx_cell_t *args);
```

- **`rt`** - Runtime context for API calls
- **`args`** - Linked list of **unevaluated** arguments
- **Returns** - A new cell (the result), or NULL on error

### Simple Builtin Example

From `builtins/add.c`:

```c
akx_cell_t *add(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  if (akx_rt_list_length(args) < 2) {
    akx_rt_error(rt, "add requires at least 2 arguments");
    return NULL;
  }

  int sum = 0;
  akx_cell_t *current = args;

  while (current) {
    akx_cell_t *evaled = akx_rt_eval_and_assert(
        rt, current, AKX_TYPE_INTEGER_LITERAL,
        "add requires integer arguments");
    if (!evaled)
      return NULL;

    sum += akx_rt_cell_as_int(evaled);
    akx_rt_free_cell(rt, evaled);
    current = akx_rt_cell_next(current);
  }

  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, sum);
  return result;
}
```

Key points:
- Arguments arrive **unevaluated** - you must call `akx_rt_eval()` explicitly
- Use `akx_rt_eval_and_assert()` to evaluate and type-check in one call
- Always free evaluated cells when done
- Allocate a new cell for the return value
- Return NULL and call `akx_rt_error()` on failure

## Lifecycle Hooks

Builtins can define optional lifecycle hooks for initialization, cleanup, and hot-reloading:

```c
void counter_init(akx_runtime_ctx_t *rt) {
  counter_state_t *state = malloc(sizeof(counter_state_t));
  state->count = 0;
  akx_rt_module_set_data(rt, state);
  printf("[counter] Initialized with count=0\n");
}

void counter_deinit(akx_runtime_ctx_t *rt) {
  counter_state_t *state = akx_rt_module_get_data(rt);
  if (state) {
    printf("[counter] Deinitializing with final count=%d\n", state->count);
    free(state);
  }
}

void counter_reload(akx_runtime_ctx_t *rt, void *old_state) {
  counter_state_t *old = (counter_state_t *)old_state;
  counter_state_t *new_state = malloc(sizeof(counter_state_t));
  new_state->count = old->count;
  akx_rt_module_set_data(rt, new_state);
  printf("[counter] Reloaded, preserving count=%d\n", new_state->count);
  free(old);
}

akx_cell_t *counter(akx_runtime_ctx_t *rt, akx_cell_t *args) {
  counter_state_t *state = akx_rt_module_get_data(rt);
  state->count++;
  
  akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
  akx_rt_set_int(rt, result, state->count);
  return result;
}
```

Hooks are discovered by naming convention:
- `<name>_init` - Called when first loaded
- `<name>_deinit` - Called on runtime shutdown
- `<name>_reload` - Called when hot-reloading (receives old state)

## Advanced Compilation Options

### Multi-File Builtins

Compile multiple C files together:

```akx
(cjit-load-builtin advanced_math
    :root "examples/builtins/advanced_math.c"
    :include-paths {
        "examples/builtins"
    }
    :implementation-files {
        "math_helper.c"
    })
```

The `:root` file contains the main builtin function. Implementation files are compiled and linked together. Include paths let you use custom headers.

### Preprocessor Defines

Pass preprocessor definitions:

```akx
(cjit-load-builtin config_demo
    :root "examples/builtins/config_demo.c"
    :linker {
        :defines {
            ["DEBUG_MODE" ""]
            ["VERSION" "\"2.0.0\""]
        }
    })
```

Defines are `[name value]` pairs. Empty string for flag-only defines.

### Linking Libraries

Link against system libraries:

```akx
(cjit-load-builtin math_ops
    :root "examples/builtins/math_ops.c"
    :linker {
        :library-paths { "/usr/local/lib" }
        :libraries { "m" "pthread" }
    })
```

## Hot Reloading

Calling `cjit-load-builtin` on an already-loaded builtin replaces it immediately:

```akx
(cjit-load-builtin counter :root "examples/builtins/counter.c")
(println "Count:" (counter))

(cjit-load-builtin counter :root "examples/builtins/counter.c")
(println "Count after reload:" (counter))
```

If a `<name>_reload` hook exists, it receives the old module state. Otherwise, `<name>_deinit` is called followed by `<name>_init`.

## Runtime API Reference

### Cell Management

```c
akx_cell_t* akx_rt_alloc_cell(akx_runtime_ctx_t *rt, akx_type_t type);
void akx_rt_free_cell(akx_runtime_ctx_t *rt, akx_cell_t *cell);
```

### Setting Cell Values

```c
void akx_rt_set_symbol(akx_runtime_ctx_t *rt, akx_cell_t *cell, const char *sym);
void akx_rt_set_int(akx_runtime_ctx_t *rt, akx_cell_t *cell, int value);
void akx_rt_set_real(akx_runtime_ctx_t *rt, akx_cell_t *cell, double value);
void akx_rt_set_string(akx_runtime_ctx_t *rt, akx_cell_t *cell, const char *str);
```

### Getting Cell Values

```c
const char* akx_rt_cell_as_symbol(akx_cell_t *cell);
int akx_rt_cell_as_int(akx_cell_t *cell);
double akx_rt_cell_as_real(akx_cell_t *cell);
const char* akx_rt_cell_as_string(akx_cell_t *cell);
```

### Type Checking

```c
int akx_rt_cell_is_type(akx_cell_t *cell, akx_type_t type);
```

Types: `AKX_TYPE_SYMBOL`, `AKX_TYPE_INTEGER_LITERAL`, `AKX_TYPE_REAL_LITERAL`, `AKX_TYPE_STRING_LITERAL`, `AKX_TYPE_LIST`

### List Operations

```c
size_t akx_rt_list_length(akx_cell_t *list);
akx_cell_t* akx_rt_list_nth(akx_cell_t *list, size_t n);
akx_cell_t* akx_rt_cell_next(akx_cell_t *cell);
```

### Evaluation

```c
akx_cell_t* akx_rt_eval(akx_runtime_ctx_t *rt, akx_cell_t *expr);
akx_cell_t* akx_rt_eval_and_assert(akx_runtime_ctx_t *rt, 
                                   akx_cell_t *expr,
                                   akx_type_t expected_type,
                                   const char *error_msg);
```

### Error Reporting

```c
void akx_rt_error(akx_runtime_ctx_t *rt, const char *message);
void akx_rt_error_fmt(akx_runtime_ctx_t *rt, const char *fmt, ...);
```

### Module State

```c
void akx_rt_module_set_data(akx_runtime_ctx_t *rt, void *data);
void* akx_rt_module_get_data(akx_runtime_ctx_t *rt);
```

## Example Files

- **`load_builtins.akx`** - Basic builtin loading
- **`test_counter.akx`** - Lifecycle hooks and hot reloading
- **`test_mixed.akx`** - Multiple builtins with and without hooks
- **`test_advanced.akx`** - Multi-file compilation with includes
- **`test_defines.akx`** - Preprocessor defines

## Running Examples

From the project root:

```bash
./build/bin/akx examples/load_builtins.akx
./build/bin/akx examples/test_counter.akx
./build/bin/akx examples/test_advanced.akx
```

## Writing Your Own Builtins

1. Create a C file in `examples/builtins/`
2. Implement your function with signature: `akx_cell_t* name(akx_runtime_ctx_t*, akx_cell_t*)`
3. Optionally add `name_init`, `name_deinit`, `name_reload` hooks
4. Load it with `cjit-load-builtin`
5. Call it like any other function

No header files needed - the runtime API is automatically injected during compilation.

