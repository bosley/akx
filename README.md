<div align="center">

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/logo-dark.svg">
  <img src="assets/logo-light.svg" alt="AKX Logo" width="200" height="200">
</picture>

# AKX

**A self-extending runtime where the language grows itself.**

</div>

## What is AKX?

AKX is a dynamic runtime that extends itself through JIT compilation. The philosophy is simple: **you provide the function, we provide the environment**. Write your logic in C, and AKX handles compilation, linking, memory management, and integration—all at runtime.

By default, AKX ships with exactly **one native builtin**: `cjit-load-builtin`. This single function bootstraps the entire system by JIT-compiling C source files on-the-fly. Everything else—arithmetic, I/O, control flow—can be loaded dynamically or compiled in via the nucleus system.

### The Nucleus System

AKX includes a **nucleus** directory containing core language features as standalone C files. At build time, you choose which nuclei to compile into the binary (for speed) and which to load at runtime (for flexibility). The same C file works both ways—no code duplication.

This means:

- **Minimal core**: The runtime is tiny, containing only the evaluation engine and bootstrap loader
- **Maximum extensibility**: Add new language features without recompiling the runtime
- **Hot reloading**: Update builtins while running, preserving state across reloads
- **Your code, our runtime**: Focus on logic; we handle the environment

## Architecture Overview

AKX follows a unique bootstrap architecture where the runtime grows itself:

1. **Bootstrap**: The single native `cjit-load-builtin` function reads C source files
2. **JIT Compilation**: Uses TCC (Tiny C Compiler) to compile code at runtime
3. **Dynamic Linking**: Links compiled functions into the running process
4. **Registration**: Registers new functions as callable builtins from AKX code

<summary><h3>The Nucleus System</h3></summary>

<details>

At build time, `nucleus/manifest.txt` defines which language features to compile in versus load at runtime. The core runtime is truly minimal - check the manifest to see which primitives are enabled by default.

**Current default compilation includes:**
- **Core language**: `if`, `lambda`, `let`, `set`, `import`, `begin`, `loop`, `?defined`, `cons`, `car`, `cdr`
- **Math operations**: `+`, `-`, `*`, `/`, `%`, `=`
- **Comparison**: `eq`, `neq`, `gt`, `gte`, `lt`, `lte`
- **Logic**: `and`, `or`, `not`
- **Type predicates**: `?nil`, `?int`, `?real`, `?lambda`, `?str`, `?quote`, `?sym`
- **I/O**: `io/putf`, `io/scanf`
- **Filesystem**: `fs/read-file`, `fs/write-file`, `fs/open`, `fs/close`, and more
- **OS integration**: `os/args`, `os/cwd`, `os/chdir`, `os/env`
- **Runtime**: `akx/exec`
- **Testing**: `assert/true`, `assert/false`, `assert/eq`, `assert/ne`

The same C file works both ways. Change the manifest to rebalance speed vs. flexibility without touching any code. **Disable primitives you don't need to keep the runtime lightweight.**
</details>

<summary><h3>Cell Memory Model</h3></summary>

<details>

The runtime uses a **cell memory model** where expressions are represented as linked cells. Each cell can be a symbol, integer, real number, string, or list. Lists are formed by linking cells horizontally (`next`) and vertically (`list_head`).

For detailed architecture diagrams, data flows, and implementation details, see [ARCHITECTURE.md](ARCHITECTURE.md).
</details>

## Quick Start

### Installation

```bash
# Build and install to ~/.akx
make
make install

# Add to your shell rc file
export PATH="$HOME/.akx/bin:$PATH"
export AKX_HOME="$HOME/.akx"
```

### Hello World

Create `hello.akx`:

```akx
(import "$AKX_HOME/stdlib/io.akx")
(println "Hello, World!")
```

Run it:

```bash
akx hello.akx
```

### Using the Standard Library

The stdlib provides common operations via the nucleus system:

```akx
(import "$AKX_HOME/stdlib/stdlib.akx")

(println "Math:" (+ 10 20 30))
(println "Result:" (* (+ 5 3) (- 10 2)))
(println "Equal?" (= 42 42))
```

<summary><h3>OS Module - System Interaction</h3></summary>

<details>

Access command-line arguments, environment variables, and system information:

```akx
(import "$AKX_HOME/stdlib/io.akx")

(let args (os/args))
(println "Script arguments:" args)

(let cwd (os/cwd))
(println "Current directory:" cwd)

(os/chdir "/tmp")
(println "Changed to:" (os/cwd))

(let home (os/env :get "HOME"))
(println "Home directory:" home)

(os/env :set "MY_VAR" "my_value")
(println "Set variable:" (os/env :get "MY_VAR"))
```

Run with arguments:
```bash
akx myscript.akx arg1 arg2 arg3
```
</details>

<summary><h3>Loading Your Own Functions</h3></summary>

<details>
Write your logic in C, AKX provides the environment:

```c
// my_function.c
akx_cell_t *my_function(akx_runtime_ctx_t *rt, akx_cell_t *args) {
    akx_cell_t *result = akx_rt_alloc_cell(rt, AKX_TYPE_STRING_LITERAL);
    akx_rt_set_string(rt, result, "Hello from my C code!");
    return result;
}
```

Load and use it:

```akx
(cjit-load-builtin greet :root "my_function.c" :as "my_function")
(import "$AKX_HOME/stdlib/io.akx")
(println (greet))
```
</details>

<summary><h2>Testing</h2></summary>

<details>
AKX includes a comprehensive test suite to ensure core language stability:

```bash
./tests/run.sh
```

The test suite uses expect-based assertions to validate:
- Variable binding (`let`, `set`)
- Lambda functions and closures
- Scope management
- Type system (integers, reals, strings)
- Filesystem operations
- OS integration
- Assertion builtins

See **[tests/README.md](tests/README.md)** for detailed testing documentation.
</details>

<summary><h2>Documentation</h2></summary>

<details>
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Complete architecture overview with diagrams
- **[nucleus/forms/forms.md](nucleus/forms/forms.md)** - Forms system: types, pattern matching, and affordances
- **[LAMBDA_IMPLEMENTATION.md](LAMBDA_IMPLEMENTATION.md)** - Lambda and scope system implementation
- **[ASSERTIONS.md](ASSERTIONS.md)** - Assertion builtins and signal handling
- **[tests/README.md](tests/README.md)** - Test suite documentation and guidelines
- **[pkg/rt/model.md](pkg/rt/model.md)** - Runtime model and API reference
- **[pkg/cell/model.md](pkg/cell/model.md)** - Cell memory model documentation
</details>

<summary><h2>Building</h2></summary>

<details>

AKX automatically fetches and installs AK24 if not found. No manual installation required.

```bash
make          # Build the project (compile-time tests will run)
make install  # Install to ~/.akx (or $AKX_HOME if set)
make test     # Run test suite (requires make install first - tests using installed system)
```

**Note:** `make test` requires `make install` to be run first, as tests depend on the stdlib being installed at `~/.akx`.

The build system will:
- Check for AK24 at `$AK24_HOME` or `~/.ak24`
- If not found, automatically clone, build, and install AK24
- Configure and build AKX with the nucleus system
- Generate builtin registration code from `nucleus/manifest.txt`

### Installation

By default, `make install` installs to `~/.akx`:

```
~/.akx/
├── bin/akx              # The runtime binary
├── nucleus/             # C source files for all nuclei
│   ├── core/           # if, lambda, let, set
│   ├── math/           # +, -, *, /, %, =
│   ├── io/             # print, println
│   └── assert/         # assert/true, assert/false, assert/eq, assert/ne
└── stdlib/             # AKX import files
    ├── stdlib.akx      # Load all common operations
    ├── math.akx        # Math operations only
    └── io.akx          # I/O operations only
```

To install elsewhere:

```bash
export AKX_HOME=/opt/akx
make install
```

### Build Options

By default, AKX uses the `akx.dev.0` branch of AK24. To use a different branch:

```bash
cmake -DAK24_GIT_BRANCH=main ..
```
</details>

<summary><h2>Features</h2></summary>

<details>

<summary><h3>Forms System - Type Definitions & Protocols</h3></summary>

<details>

AKX includes a `forms` system for defining custom types, pattern matching, and behavioral protocols. Forms enable type-safe module development without modifying the runtime:

```akx
(form/define :Temperature :real)

(form/add-affordance :Temperature :to-fahrenheit
  (lambda [c] (+ (* c 1.8) 32)))

(let temp 25.0)
(let fahrenheit (form/invoke temp :to-fahrenheit))
(io/putf "25°C = ")
(io/putf fahrenheit)
(io/putf "°F\n")
```

**Key capabilities:**
- **Custom types**: Define domain-specific types beyond built-in primitives
- **Pattern matching**: Validate data structures at runtime with `form/matches`
- **Affordances**: Attach behavioral protocols (trait-like interfaces) to types
- **Type safety**: Strict type checking with no implicit coercion
- **Composability**: Struct forms, optional types, repeatable types, and lists

See **[nucleus/forms/forms.md](nucleus/forms/forms.md)** for complete documentation with examples.
</details>

<summary><h3>OS Integration</h3></summary>

<details>

Built-in OS module provides system-level access:

- **`os/args`** - Access command-line arguments as a list
- **`os/cwd`** - Get current working directory
- **`os/chdir "path"`** - Change working directory
- **`os/env :get "VAR"`** - Read environment variables (returns `nil` if not found)
- **`os/env :set "VAR" "VALUE"`** - Set environment variables
</details>

<summary><h3>Tail Call Optimization</h3></summary>

<details>

AKX implements trampoline-based tail call optimization, allowing unbounded recursion for tail-recursive functions:

```akx
(let countdown (lambda [n]
  (if (eq n 0)
    0
    (countdown (- n 1)))))

(countdown 5000)  # No stack overflow!
```

The runtime automatically detects tail calls and converts them to iterative loops, eliminating C stack recursion.
</details>

<summary><h3>Universal Iterator - Transform Anything</h3></summary>

<details>

AKX includes an `iter` function that adapts to the type being iterated, enabling transformations at any level—from list elements to individual bits:

**Lists**: Transform each element
```akx
(iter '(1 2 3 4 5) (lambda [n] (* n 2)))
; → '(2 4 6 8 10)
```

**Strings**: Transform each UTF-8 character
```akx
(iter "hello" (lambda [c] 
  (if (eq c "o") "*" c)))
; → "hell*"
```

**Integers & Reals**: Transform each BIT (32-bit for int, 64-bit for real)
```akx
; Bitwise NOT via bit flipping
(iter 5 (lambda [bit] (if (eq bit 0) 1 0)))
; → -6

; Zero out all bits in a float
(iter 3.14159 (lambda [bit] 0))
; → 0.0

; Bit-level transformations enable low-level manipulation
; without leaving the language
```

This unified iteration model means you can transform data at any granularity—from high-level collections down to individual bits in numeric types. The same `iter` function adapts its behavior based on what you're iterating over.
</details>

<summary><h3>List Operations - cons, car, cdr</h3></summary>

<details>

AKX provides fundamental Lisp-style list manipulation functions for building and traversing lists:

**`cons`** - Prepend an element to a list
```akx
(cons 1 '(2 3))
; → '(1 2 3)

(cons "a" '())
; → '("a")
```

**`car`** - Get the first element of a list
```akx
(car '(10 20 30))
; → 10

(car '("first" "second"))
; → "first"
```

**`cdr`** - Get the rest of the list (everything after the first element)
```akx
(cdr '(1 2 3 4))
; → '(2 3 4)

(cdr '(only))
; → nil
```

**Building lists with cons:**
```akx
(let my-list (cons 1 (cons 2 (cons 3 '()))))
(car my-list)           ; → 1
(car (cdr my-list))    ; → 2
(car (cdr (cdr my-list))) ; → 3
```
</details>

</details>