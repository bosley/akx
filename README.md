<div align="center">

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/logo-dark.svg">
  <img src="assets/logo-light.svg" alt="AKX Logo" width="200" height="200">
</picture>

# AKX

**A self-extending runtime where the language grows itself.**

</div>

## What is AKX?

AKX is a dynamic runtime that extends itself through JIT compilation. Unlike traditional languages that ship with a fixed set of built-in functions, AKX ships with exactly **one native builtin**: `cjit-load-builtin`. This single function serves as the bootstrap mechanism for the entire runtime.

All other functionality—arithmetic operations, I/O, data structures, and more—is loaded dynamically at runtime by compiling C source files on-the-fly. This means:

- **Minimal core**: The runtime is tiny, containing only the essential evaluation engine
- **Maximum extensibility**: Add new language features without recompiling the runtime
- **Hot reloading**: Update builtins while the runtime is running, preserving state across reloads
- **Self-extending**: The language defines itself, repeatedly, at runtime

## Architecture Overview

AKX follows a unique bootstrap architecture where the runtime grows itself:

1. **Bootstrap**: The single native `cjit-load-builtin` function reads C source files
2. **JIT Compilation**: Uses TCC (Tiny C Compiler) to compile code at runtime
3. **Dynamic Linking**: Links compiled functions into the running process
4. **Registration**: Registers new functions as callable builtins from AKX code

The runtime uses a **cell memory model** where expressions are represented as linked cells. Each cell can be a symbol, integer, real number, string, or list. Lists are formed by linking cells horizontally (`next`) and vertically (`list_head`).

For detailed architecture diagrams, data flows, and implementation details, see [ARCHITECTURE.md](ARCHITECTURE.md).

## Quick Start

Load a builtin and use it:

```akx
(cjit-load-builtin println :root "examples/builtins/println.c")
(cjit-load-builtin add :root "examples/builtins/add.c")

(println "Hello from JIT!")
(println "Sum:" (add 1 2 3 4 5))
```

Run an AKX file:

```bash
./build/bin/akx examples/load_builtins.akx
```

## Examples

<details>
<summary><strong>Basic Builtin Loading</strong></summary>

Load and use simple builtins:

```akx
(cjit-load-builtin add :root "examples/builtins/add.c")
(cjit-load-builtin println :root "examples/builtins/println.c")

(println "Builtins loaded successfully!")
(println "Testing add:" (add 1 2 3 4 5))
```

See: [`examples/load_builtins.akx`](examples/load_builtins.akx)

</details>

<details>
<summary><strong>Lifecycle Hooks and Hot Reloading</strong></summary>

Builtins can define initialization, cleanup, and hot-reload hooks:

```akx
(cjit-load-builtin println :root "examples/builtins/println.c")
(cjit-load-builtin counter :root "examples/builtins/counter.c")

(println "Testing counter builtin with lifecycle hooks...")
(println "First call:" (counter))
(println "Second call:" (counter))
(println "Third call:" (counter))

(println "Hot reloading counter...")
(cjit-load-builtin counter :root "examples/builtins/counter.c")

(println "After reload - Fourth call:" (counter))
(println "After reload - Fifth call:" (counter))
```

State is preserved across hot reloads. See: [`examples/test_counter.akx`](examples/test_counter.akx)

</details>

<details>
<summary><strong>Advanced Multi-File Compilation</strong></summary>

Compile multiple C files together with custom include paths:

```akx
(cjit-load-builtin println :root "examples/builtins/println.c")

(cjit-load-builtin advanced_math
    :root "examples/builtins/advanced_math.c"
    :include-paths {
        "examples/builtins"
    }
    :implementation-files {
        "math_helper.c"
    })

(println "=== Testing Advanced Multi-File Builtin ===")
(println "advanced_math(5) = x^2 + 10x = 25 + 50 =" (advanced_math 5))
```

See: [`examples/test_advanced.akx`](examples/test_advanced.akx)

</details>

<details>
<summary><strong>Preprocessor Defines</strong></summary>

Pass compile-time configuration via preprocessor defines:

```akx
(cjit-load-builtin println :root "examples/builtins/println.c")

(cjit-load-builtin config_demo
    :root "examples/builtins/config_demo.c"
    :linker {
        :defines {
            ["DEBUG_MODE" ""]
        }
    })

(println "=== Testing Preprocessor Defines ===")
(println "Config status:" (config_demo))
```

See: [`examples/test_defines.akx`](examples/test_defines.akx)

</details>

<details>
<summary><strong>Mixed Builtins</strong></summary>

Use multiple builtins with different configurations:

```akx
(cjit-load-builtin println :root "examples/builtins/println.c")
(cjit-load-builtin simple :root "examples/builtins/simple.c")
(cjit-load-builtin counter :root "examples/builtins/counter.c")

(println "=== Testing Mixed Builtins ===")
(println "Simple (no hooks):" (simple))
(println "Counter (with hooks) - call 1:" (counter))
(println "Simple again:" (simple))
(println "Counter - call 2:" (counter))

(println "")
(println "=== Hot Reloading Both ===")
(cjit-load-builtin simple :root "examples/builtins/simple.c")
(cjit-load-builtin counter :root "examples/builtins/counter.c")

(println "After reload:")
(println "Simple:" (simple))
(println "Counter - call 3:" (counter))
```

See: [`examples/test_mixed.akx`](examples/test_mixed.akx)

</details>

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
- **[examples/README.md](examples/README.md)** - Comprehensive examples and builtin API guide

## Building

AKX automatically fetches and installs AK24 if not found. No manual installation required.

```bash
mkdir build && cd build
cmake ..
make
```

The build system will:
- Check for AK24 at `$AK24_HOME` or `~/.ak24`
- If not found, automatically clone, build, and install AK24
- Configure and build AKX

### Build Options

By default, AKX uses the `akx.dev.0` branch of AK24. To use a different branch:

```bash
cmake -DAK24_GIT_BRANCH=main ..
```

Run examples:

```bash
./build/bin/akx examples/load_builtins.akx
./build/bin/akx examples/test_counter.akx
./build/bin/akx examples/test_advanced.akx
```

