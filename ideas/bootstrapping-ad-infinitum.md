# ABOUT

I had a neat idea that I wanted to make sure I could come back to.
I decided to work out parts of the idea with an AI and had it generate this document to overview some of the stuff
i was ideaing.

I might branch off and play with this later, idk.

# Bootstrapping Ad Infinitum: AOT Compilation for AKX

## The Core Idea

Create a `compile` or `program` nucleus function that takes AKX code and compiles it to a standalone native executable. This would transform AKX from a dynamic runtime into a true ahead-of-time (AOT) compiler.

## Why This Could Be Powerful

### 1. The AST is Already There
When you call `(compile '(program-body))`, the entire program is already parsed into the cell AST structure. We have:
- All expressions as `akx_cell_t` structures
- Type information for literals
- Symbol references
- Function calls with arguments

### 2. We Know All Dependencies
By walking the AST, we can identify:
- Which nucleus functions are called
- Which variables are defined
- What the control flow looks like
- What external resources are needed

### 3. Nucleus Functions Are Already C
Every nucleus function is already a C function with a known signature:
```c
akx_cell_t *function_name(akx_runtime_ctx_t *rt, akx_cell_t *args);
```

The manifest (`nucleus/manifest.txt`) maps symbols to C function names, so we know exactly what to link.

## Implementation Approaches

### Approach 1: Runtime-Embedded Compilation (Easiest)

Generate C code that embeds the runtime and calls nucleus functions:

```c
#include "akx_rt.h"
#include "builtin_registry.h"

int main(int argc, char **argv) {
    akx_runtime_ctx_t *rt = akx_runtime_init();
    akx_rt_register_compiled_nuclei(rt);
    
    // (let x 10)
    akx_cell_t *x_val = akx_rt_alloc_cell(rt, AKX_TYPE_INTEGER_LITERAL);
    akx_rt_set_int(rt, x_val, 10);
    akx_rt_scope_set(rt, "x", x_val);
    
    // (io/putf "x = %d\n" x)
    akx_cell_t *fmt = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, fmt, "x = %d\n");
    akx_cell_t *x_lookup = akx_rt_scope_get(rt, "x");
    akx_cell_t *args = /* build arg list */;
    putf_impl(rt, args);
    
    akx_runtime_deinit(rt);
    return 0;
}
```

**Pros:**
- Straightforward translation
- Can reuse all existing nucleus code
- Still benefits from native compilation

**Cons:**
- Still requires runtime library
- Not truly standalone
- Performance limited by runtime overhead

### Approach 2: Optimizing Compiler (Medium Difficulty)

Add optimization passes before code generation:

1. **Constant Folding**: `(+ 1 2)` → `3`
2. **Dead Code Elimination**: Remove unreachable branches
3. **Type Inference**: Know types at compile time
4. **Inline Simple Functions**: Eliminate function call overhead

```c
// Before optimization:
akx_cell_t *result = add(rt, args);

// After optimization:
int result = 3;  // Computed at compile time
```

**Pros:**
- Significant performance gains
- Smaller binaries
- Can eliminate runtime for pure code

**Cons:**
- Complex to implement
- Need type inference system
- Some features (eval, dynamic loading) impossible to optimize

### Approach 3: Full Static Compilation (Hardest)

Generate pure C code with no runtime dependency:

```c
#include <stdio.h>

int main() {
    int x = 10;
    int y = 20;
    printf("Sum: %d\n", x + y);
    return 0;
}
```

**Pros:**
- Maximum performance
- Tiny binaries
- Can cross-compile easily
- No runtime dependencies

**Cons:**
- Can't support dynamic features (eval, cjit-load-builtin)
- Complex type system needed
- Essentially building a new language

## Proposed API

```lisp
;; Basic compilation
(compile 
  :program '(begin
              (let x 10)
              (io/putf "x = %d\n" x))
  :output "myapp"
  :mode 'embedded)  ; 'embedded, 'optimized, or 'static

;; Advanced options
(compile
  :program (quote (import "myprogram.akx"))
  :output "production-app"
  :optimize 2
  :static-link true
  :strip-symbols true
  :target "x86_64-linux-gnu")

;; Returns: path to executable
;; Can now run: ./myapp
```

## The Compilation Pipeline

```
AKX Source Code
      ↓
   Parse to AST (akx_cell_t)
      ↓
   Analyze Dependencies
      ↓
   [Optional] Optimization Passes
      ↓
   Generate C Code
      ↓
   Compile with GCC/Clang/TCC
      ↓
   Link Nucleus Functions
      ↓
   Native Executable
```

## Use Cases

### 1. Distribution
```lisp
;; Compile your app for distribution
(compile :program (import "web_server.akx") 
         :output "server"
         :static-link true)

;; Ship: ./server (no dependencies!)
```

### 2. Performance-Critical Code
```lisp
;; JIT for development, AOT for production
(if (env/get "PRODUCTION")
  (compile :program my-app :optimize 3)
  (eval my-app))
```

### 3. Embedded Systems
```lisp
;; Cross-compile for ARM
(compile :program sensor-reader
         :target "arm-linux-gnueabihf"
         :optimize-size true)
```

### 4. Plugin Systems
```lisp
;; Compile plugins as shared libraries
(compile :program my-plugin
         :output "plugin.so"
         :shared-library true)
```

## Technical Challenges

### 1. Runtime vs Compile-Time
Some features fundamentally require a runtime:
- `eval` - Evaluating arbitrary code
- `cjit-load-builtin` - Dynamic code loading
- Reflection/introspection

**Solution:** Hybrid approach - compile what you can, keep minimal runtime for dynamic features.

### 2. Type System
AKX is dynamically typed, but compiled code benefits from static types.

**Solution:** 
- Type inference where possible
- Runtime type checks where needed
- Optional type annotations: `(let x:int 10)`

### 3. Garbage Collection
The runtime uses AK24's GC. Compiled code needs memory management.

**Solution:**
- Link GC library
- Or use reference counting for compiled code
- Or manual memory management with escape analysis

### 4. Cross-Compilation
Compiling for different architectures/OSes.

**Solution:**
- Use LLVM backend instead of direct C generation
- Or generate C and use cross-compiler toolchains
- Platform-specific nucleus implementations

## Why We're NOT Doing This (Yet)

1. **Scope Creep** - We're building a shell, not a compiler
2. **Premature Optimization** - Don't need native performance yet
3. **Complexity** - Significant engineering effort
4. **Maintenance Burden** - Two execution paths to maintain
5. **Missing Foundation** - Need more nucleus functions first

## When This WOULD Make Sense

- **After** we have a solid set of shell nucleus functions
- **After** we've used AKX for real automation tasks
- **When** we hit performance bottlenecks
- **When** we need to distribute binaries to users
- **When** we want to target embedded systems

## The Right Time to Revisit

This idea becomes valuable when:
1. AKX is feature-complete for its primary use case (shell scripting)
2. We have real-world programs that would benefit from compilation
3. We need to ship binaries without the runtime
4. Performance becomes a limiting factor

## Implementation Sketch

If we were to do this, here's the nucleus function signature:

```c
// nucleus/core/compile.c
akx_cell_t *compile_impl(akx_runtime_ctx_t *rt, akx_cell_t *args) {
    // 1. Extract program AST from quoted expression
    akx_cell_t *program = /* get :program argument */;
    
    // 2. Analyze AST
    //    - Collect all referenced nucleus functions
    //    - Build symbol table
    //    - Identify constants vs variables
    
    // 3. Generate C code
    ak_buffer_t *c_code = generate_c_code(program);
    
    // 4. Write to temp file
    char *temp_path = write_temp_file(c_code);
    
    // 5. Invoke compiler
    //    gcc -O2 temp.c -o output -lakx_nucleus -lak24
    
    // 6. Return path to executable
    akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, result, output_path);
    return result;
}
```

## Related Ideas

### JIT Compilation (Easier Alternative)
Instead of AOT, improve the JIT:
- Cache compiled functions
- Profile-guided optimization
- Inline hot paths

### Bytecode VM (Middle Ground)
Compile to bytecode instead of native:
- Faster than interpretation
- Portable across platforms
- Easier than native compilation

### Partial Evaluation
Compile frequently-used functions, interpret the rest:
```lisp
(jit-compile my-hot-function)  ; Compile this one function
```