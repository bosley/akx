# Forms System

The Forms system provides extensible type definitions, pattern matching, and behavioral affordances for AKX modules. It enables runtime type validation and protocol-like behavior without inheritance.

## Overview

Forms are a sophisticated type system built on top of the ak24 forms library. They allow:

- **Extensible Types**: Define custom types beyond the hardcoded `akx_type_t` enum
- **Pattern Matching**: Runtime validation of data structures against type patterns
- **Behavioral Affordances**: Attach behaviors (like traits/protocols) to types
- **Module Composability**: Forms can be registered and shared across modules

## Core Concepts

### Forms

A **form** is a type definition that describes the structure and constraints of data. Forms can be:

- **Primitive**: Basic atomic types (int, real, byte, string)
- **Compound**: Multiple forms combined together
- **Named**: Aliases to other forms
- **List**: Homogeneous collections
- **Optional**: Values that may be absent
- **Repeatable**: Values that can occur multiple times

### Affordances

An **affordance** is a behavior attached to a form. Affordances are similar to:
- **Traits** in Rust
- **Protocols** in Clojure
- **Interfaces** in Go

They define what operations can be performed on instances of a form without requiring inheritance.

### Keywords

Forms use **keyword syntax** (symbols starting with `:`) to reference form names. This provides:
- Clear distinction from regular symbols
- No evaluation ambiguity
- Clean, readable syntax

## Nucleus Builtins

### form-define

Define a new form.

**Syntax:**
```lisp
(form-define :FormName :base-type)
(form-define :FormName { :type1 :type2 ... })
(form-define :FormName [:field1 :type1 :field2 :type2 ...])
(form-define :FormName (optional :type))
(form-define :FormName (repeatable :type))
(form-define :FormName (list :type))
```

**Examples:**
```lisp
(form-define :Point :int)
(form-define :Temperature :real)
(form-define :Name :string)
(form-define :Coordinate { :int :int })
(form-define :Person [:name :string :age :int :email :string])
(form-define :OptionalAge (optional :int))
(form-define :Tags (repeatable :string))
(form-define :Numbers (list :int))
```

**Parameters:**
- `:FormName` - Keyword identifying the new form
- `:base-type` - Primitive type or existing form to base on
- `{ :type1 ... }` - Compound form with multiple types
- `[:field1 :type1 ...]` - Struct form with named fields
- `(optional :type)` - Optional form (value may be absent)
- `(repeatable :type)` - Repeatable form (zero or more occurrences)
- `(list :type)` - Homogeneous list of elements

**Returns:** Symbol with the form name

**Form Aliasing:**
```lisp
(form-define :Age :int)
(form-define :PersonAge :Age)
```

### form-matches

Check if a value matches a form pattern.

**Syntax:**
```lisp
(form-matches value :FormName)
```

**Examples:**
```lisp
(let x 42)
(if (form-matches x :Point)
  (io/putf "x is a Point\n")
  (io/putf "x is not a Point\n"))

(let temp 98.6)
(form-matches temp :Temperature)
```

**Parameters:**
- `value` - Expression to evaluate and check
- `:FormName` - Keyword identifying the form to match against

**Returns:** Integer (1 for match, 0 for no match)

**Type Safety:**
- Integers match `:int` forms
- Reals match `:real` forms
- Strings match `:string` forms
- Type mismatches return 0 (false)

### form-add-affordance

Attach a behavioral affordance to a form.

**Syntax:**
```lisp
(form-add-affordance :FormName :affordance-name lambda)
```

**Examples:**
```lisp
(form-define :Counter :int)
(form-add-affordance :Counter :increment
  (lambda [x] (+ x 1)))

(form-add-affordance :Counter :decrement
  (lambda [x] (- x 1)))

(form-add-affordance :Counter :reset
  (lambda [x] 0))
```

**Parameters:**
- `:FormName` - Keyword identifying the form
- `:affordance-name` - Keyword identifying the affordance
- `lambda` - Lambda expression implementing the behavior

**Returns:** Integer (1 for success, 0 for failure)

**Multiple Affordances:**
A single form can have multiple affordances, enabling rich behavioral protocols.

### form-invoke

Invoke an affordance on a value that matches a form.

**Syntax:**
```lisp
(form-invoke value :affordance-name [additional-args...])
```

**Examples:**
```lisp
(form-define :Counter :int)
(form-add-affordance :Counter :increment (lambda [x] (+ x 1)))

(let counter 10)
(let result (form-invoke counter :increment))

(form-add-affordance :Counter :add (lambda [x y] (+ x y)))
(let result (form-invoke counter :add 5))
```

**Parameters:**
- `value` - An instantiated value (variable) to invoke the affordance on
- `:affordance-name` - Keyword identifying the affordance to invoke
- `additional-args...` - Optional additional arguments passed to the affordance lambda

**Returns:** The result of the affordance lambda invocation

**Behavior:**
1. Evaluates the value expression
2. Searches registered forms to find one matching the value's structure
3. Retrieves the affordance from the matching form
4. Invokes the affordance's lambda with the value as the first argument
5. Returns the result

**Important:** Affordances operate on instantiated values (variables), not literal expressions. Use `let` to bind literals to variables before invoking affordances:

```lisp
(let x 10)
(form-invoke x :increment)
```

This design reflects that forms describe the structure of instantiated objects, enabling extensible behavior on actual data.

## Built-in Primitive Forms

The runtime automatically registers these primitive forms:

| Form | Base Type | Description |
|------|-----------|-------------|
| `int` | I64 | 64-bit signed integer |
| `real` | F64 | 64-bit floating point |
| `byte` | U8 | Unsigned 8-bit integer |
| `string` | List of CHAR | Character sequence |
| `symbol` | Named CHAR | Symbol identifier |
| `list` | List | Generic list |
| `nil` | Named U8 | Nil/null value |

## Type Matching Rules

### Primitive Matching

```lisp
(let i 42)           
(form-matches i :int)     ; 1 (true)
(form-matches i :real)    ; 0 (false)

(let r 3.14)
(form-matches r :real)    ; 1 (true)
(form-matches r :int)     ; 0 (false)

(let s "hello")
(form-matches s :string)  ; 1 (true)
(form-matches s :int)     ; 0 (false)
```

### Compound Matching

Compound forms use curly brace syntax to define ordered type sequences:

```lisp
(form-define :Point2D { :int :int })
(form-define :Point3D { :int :int :int })
(form-define :Record { :string :int :real })
```

Compound forms are useful for tuples and fixed-structure data.

### Non-existent Forms

Matching against non-existent forms returns 0 (false) without error:

```lisp
(form-matches x :NonExistent)  ; 0 (false)
```

## Usage Patterns

### Type Validation

```lisp
(form-define :Age :int)

(let age 25)
(if (form-matches age :Age)
  (io/putf "Valid age\n")
  (io/putf "Invalid age\n"))
```

### Protocol Definition

```lisp
(form-define :Drawable :int)

(form-add-affordance :Drawable :draw
  (lambda [x] (io/putf "Drawing...\n")))

(form-add-affordance :Drawable :erase
  (lambda [x] (io/putf "Erasing...\n")))
```

### Domain Modeling

```lisp
(form-define :Width :int)
(form-define :Height :int)
(form-define :Depth :int)

(let w 100)
(let h 200)
(let d 50)

(if (and (form-matches w :Width)
         (form-matches h :Height)
         (form-matches d :Depth))
  (io/putf "Valid dimensions\n")
  (io/putf "Invalid dimensions\n"))
```

### Struct Forms with Named Fields

Struct forms use square bracket syntax to define named fields:

```lisp
(form-define :Person [:name :string :age :int :email :string])
(form-define :Point [:x :int :y :int])
(form-define :Employee [:id :int :name :string :salary :real :active :int])
```

Struct forms provide clear field semantics and enable self-documenting data structures.

### Optional and Repeatable Forms

Optional forms represent values that may be absent:

```lisp
(form-define :OptionalPort (optional :int))
(form-define :MaybeString (optional :string))
```

Repeatable forms represent zero or more occurrences:

```lisp
(form-define :Tags (repeatable :string))
(form-define :IntSequence (repeatable :int))
```

List forms represent homogeneous collections:

```lisp
(form-define :Numbers (list :int))
(form-define :StringList (list :string))
(form-define :RealList (list :real))
```

These can be combined in struct forms:

```lisp
(form-define :Config [:host :string :port :OptionalPort])
(form-define :Document [:title :string :tags :Tags])
```

Or nested within each other:

```lisp
(form-define :TempList (list :int))
(form-define :OptionalList (optional :TempList))
```

## Implementation Details

### Runtime Integration

Forms are stored in the runtime context's `forms` map:

```c
struct akx_runtime_ctx_t {
  map_void_t forms;  // Form registry
  // ...
};
```

### Form Registration

Forms are registered using interned strings as keys:

```c
int akx_rt_register_form(akx_runtime_ctx_t *rt, 
                         const char *name, 
                         ak_form_t *form);
```

### Pattern Matching

Cell-to-form matching is performed by `akx_rt_cell_matches_form()`:

```c
int akx_rt_cell_matches_form(akx_runtime_ctx_t *rt, 
                              akx_cell_t *cell,
                              const char *form_name);
```

This function:
1. Looks up the form by name
2. Follows named form alias chains to the underlying form
3. Checks the cell's type against the form's pattern
4. Returns 1 for match, 0 for no match

Named forms automatically delegate to their base forms, enabling transparent aliasing.

### Memory Management

- Forms are allocated via ak24's allocation system
- Forms are freed when the runtime context is deinitialized
- Keyword symbols are not allocated or freed by form builtins

## Design Decisions

### Keyword Syntax

We chose keyword syntax (`:FormName`) over quoted symbols (`'FormName`) because:

1. **No Evaluation Ambiguity**: Keywords are symbols, not quoted expressions
2. **Simpler Memory Management**: No need to unwrap and free parsed cells
3. **Clearer Intent**: Keywords clearly indicate "this is a type reference"
4. **Consistent with AKX**: Keywords already used in other builtins (e.g., `fs/open`)

### No Implicit Coercion

Forms enforce strict type matching:
- Integers don't match real forms
- Reals don't match integer forms
- Strings don't match numeric forms

This prevents subtle bugs from implicit conversions.

### Affordances as Lambdas

Affordances are implemented as lambda expressions, allowing:
- Full access to the runtime environment
- Closures over captured variables
- Composition with other language features

## Examples

### Complete Example: Counter with Affordances

```lisp
(form-define :Counter :int)

(form-add-affordance :Counter :increment
  (lambda [x] (+ x 1)))

(form-add-affordance :Counter :decrement
  (lambda [x] (- x 1)))

(form-add-affordance :Counter :reset
  (lambda [x] 0))

(let counter 10)
(if (form-matches counter :Counter)
  (begin
    (io/putf "Counter is valid\n")
    (io/putf "Counter can be incremented, decremented, and reset\n"))
  (io/putf "Not a counter\n"))
```

### Complete Example: Dimensional Validation

```lisp
(form-define :Width :int)
(form-define :Height :int)
(form-define :Depth :int)

(let w 100)
(let h 200)
(let d 50)

(if (and (form-matches w :Width)
         (form-matches h :Height)
         (form-matches d :Depth))
  (begin
    (io/putf "Valid box dimensions:\n")
    (io/putf "  Width: ")
    (io/putf (+ w 0))
    (io/putf "\n  Height: ")
    (io/putf (+ h 0))
    (io/putf "\n  Depth: ")
    (io/putf (+ d 0))
    (io/putf "\n"))
  (io/putf "Invalid dimensions\n"))
```

## Runtime Enhancements

### Global `nil` Symbol

The runtime automatically defines a global `nil` symbol at initialization:

```lisp
(if (form-matches nil :int)
  (io/putf "nil matches int\n")
  (io/putf "nil does not match int\n"))  ; This branch executes
```

### Named Form Alias Chains

Named forms properly follow alias chains to their underlying types:

```lisp
(form-define :UserId :int)
(form-define :PersonId :UserId)

(let id 1001)
(form-matches id :PersonId)  ; 1 (true) - follows chain to :int
```

