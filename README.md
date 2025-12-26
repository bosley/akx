# AKX - AK24 S-Expression Tokenizer

An S-Expression parser using AK24's scanner that scans groups of `()` to retrieve identifiers and other static primitives. This is a tokenizer without built-in language constructs.

## Architecture

### AK24 Dependencies

- **ak24/sourceloc**: All files utilize sourceloc for maintaining source information in each token via `ak_sourceloc_range_t`. File imports create a new `akx_file_ctx_t` containing the source location and all scanned-in tokens.

- **ak24/intern**: ALL strings from tokens are read into the string interning library `ak24/intern` to ensure minimal memory overhead from strings that persist in token structures.

- **ak24/signals**: Main application uses signals + application macros for setup in `cmd/akx/main.c`.

- **ak24/list**: Maintains a `list_t` of tokens, defined in `ak24/list` (generic list structure).

- **ak24 Memory**: The most important aspect is the unified memory solution for GC/non-GC. All mallocs, allocs, frees, etc MUST use the `AK24_*` variants: `AK24_MALLOC`, `AK24_FREE`. See `kernel/kernel.h` for more information.

- **ak24/threads**: When the user calls `akx/pkg/tokenizer`, threading support is available.

## Building

Requires AK24 to be installed (shared library build recommended):

```bash
cd /path/to/ak24
make install
```

Then build AKX:

```bash
make
```

Run the demo:

```bash
make run
```

## Project Structure

```
akx/
├── cmd/
│   └── akx/           # Main application binary
│       ├── main.c     # Entry point with signal handling
│       └── CMakeLists.txt
├── pkg/
│   └── core/          # Core AKX library
│       ├── akx_core.h
│       ├── akx_core.c
│       └── CMakeLists.txt
├── CMakeLists.txt     # Root build configuration
├── Makefile           # Convenience wrapper
└── README.md
```

## Status

Currently implements:
- Basic AKX core initialization/deinitialization
- Signal handling demo from AK24
- Links against installed AK24 shared library
- Proves that pkg/core and cmd/akx can reach AK24 without linking TCC directly

## Virtual lists, and parsing.

In most lisps, you MUST start an expression with a `(` in akx if we are expecting a `(` on a top-level list (only on top-level lists) and we see a "symbol" type from the scanner, then we add a "virtual (" and enforce there being no ')' following the list, and instead, a newline. 

```
put "this is some text"     

; Notice the newline, so becomes -> (put "this is some text")
; if a comment was following, with no newline, the same behavior (comments act as newlines too)
```

Handeling inners will be simple. the ak24 scanned can scan "groups" `()` `""` `[]` etc. so, if in the upper list and we get a `(` all we have to do is scan the `()` group in, then tokenize to get the following behaviour (correct):

```
put (strcat "moot"

"toot"

"loot"

"foot") "this also comes out" "and this" "and this" ;   newline/comment here terminates the virtual list

```