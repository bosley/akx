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

### The Nucleus System

At build time, `nucleus/manifest.txt` defines which language features to compile in versus load at runtime:

- **Compiled-in nuclei** (fast): `if`, `lambda`, `let`, `set`, `assert-*`
- **Runtime-loadable nuclei** (flexible): `+`, `-`, `*`, `/`, `%`, `=`, `print`, `println`

The same C file works both ways. Change the manifest to rebalance speed vs. flexibility without touching any code.

### Cell Memory Model

The runtime uses a **cell memory model** where expressions are represented as linked cells. Each cell can be a symbol, integer, real number, string, or list. Lists are formed by linking cells horizontally (`next`) and vertically (`list_head`).

For detailed architecture diagrams, data flows, and implementation details, see [ARCHITECTURE.md](ARCHITECTURE.md).

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

### Loading Your Own Functions

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

## Testing

AKX includes a comprehensive test suite to ensure core language stability:

```bash
./tests/run.sh
```

The test suite uses expect-based assertions to validate:
- Variable binding (`let`, `set`)
- Lambda functions and closures
- Scope management
- Type system (integers, reals, strings)
- Assertion builtins

See **[tests/README.md](tests/README.md)** for detailed testing documentation.

## Documentation

- **[ARCHITECTURE.md](ARCHITECTURE.md)** - Complete architecture overview with diagrams
- **[LAMBDA_IMPLEMENTATION.md](LAMBDA_IMPLEMENTATION.md)** - Lambda and scope system implementation
- **[ASSERTIONS.md](ASSERTIONS.md)** - Assertion builtins and signal handling
- **[tests/README.md](tests/README.md)** - Test suite documentation and guidelines
- **[pkg/rt/model.md](pkg/rt/model.md)** - Runtime model and API reference
- **[pkg/cell/model.md](pkg/cell/model.md)** - Cell memory model documentation

## Building

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
│   └── assert/         # assert-true, assert-false, assert-eq, assert-ne
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

## Features

### Tail Call Optimization

AKX implements trampoline-based tail call optimization, allowing unbounded recursion for tail-recursive functions:

```akx
(let countdown (lambda [n]
  (if (eq n 0)
    0
    (countdown (- n 1)))))

(countdown 5000)  # No stack overflow!
```

The runtime automatically detects tail calls and converts them to iterative loops, eliminating C stack recursion.