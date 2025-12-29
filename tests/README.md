# AKX Core Test Suite

## Overview

This directory contains expect-based assertion tests for the AKX core language features. These tests ensure the core runtime is bulletproof and stable.

## Running Tests

```bash
./tests/run.sh
```

The test runner will:
1. Find the AKX binary at `build/bin/akx`
2. Run each `.akx` test file
3. Compare output against corresponding `.expect` files
4. Report pass/fail status with colored output

## Test File Format

Each test consists of two files:

- `XX_test_name.akx` - The test code
- `XX_test_name.expect` - The expected output (stdout + stderr)

### Example

**test_example.akx:**
```lisp
(let x 10)
(assert/eq x 10)
```

**test_example.expect:**
```
(empty file - no output expected)
```

## Test Coverage

### Core Language Features

| Test | Feature | Description |
|------|---------|-------------|
| `01_let_basic` | Variable binding | Basic `let` declarations |
| `02_set_basic` | Variable mutation | Basic `set` updates |
| `03_lambda_identity` | Lambdas | Simple identity function |
| `04_lambda_higher_order` | Higher-order functions | Lambdas returning lambdas |
| `05_assert_true` | Assertions | `assert/true` with various types |
| `06_assert_false` | Assertions | `assert/false` with falsy values |
| `07_assert_eq` | Assertions | Equality assertions |
| `08_assert_ne` | Assertions | Inequality assertions |
| `09_scope_nested` | Scoping | Nested scope shadowing |
| `10_lambda_multi_param` | Lambdas | Multiple parameter functions |
| `11_types_integer` | Types | Integer literals and operations |
| `12_types_string` | Types | String literals and comparison |
| `13_types_real` | Types | Real number literals |

## Writing New Tests

1. Create a new `.akx` file with a numbered prefix:
   ```bash
   touch tests/14_my_new_test.akx
   ```

2. Write your test code using assertions:
   ```lisp
   (let x 42)
   (assert/eq x 42)
   ```

3. Run the test once to capture output:
   ```bash
   ./build/bin/akx tests/14_my_new_test.akx > tests/14_my_new_test.expect 2>&1
   ```

4. Verify the output is correct, then run the test suite:
   ```bash
   ./tests/run.sh
   ```

## Test Naming Convention

Tests are numbered sequentially with descriptive names:

```
XX_category_description.akx
```

Where:
- `XX` = Two-digit number (01, 02, etc.)
- `category` = Feature category (let, lambda, assert, types, etc.)
- `description` = Brief description of what's being tested

## Exit Codes

- `0` - All tests passed
- `1` - One or more tests failed

## Color Output

- 🟢 **Green ✓** - Test passed
- 🔴 **Red ✗** - Test failed
- 🟡 **Yellow ⊘** - Test skipped (no .expect file)

## CI/CD Integration

Add to your CI pipeline:

```bash
make && ./tests/run.sh
```

This ensures the core language remains stable across all changes.

## Debugging Failed Tests

When a test fails, the runner shows:
1. Expected output
2. Actual output
3. Unified diff highlighting differences

Example:
```
Testing 07_assert_eq... ✗ FAIL

Expected output:
(empty)

Actual output:
Error: assertion failed

Diff:
+ Error: assertion failed
```

## Best Practices

1. **Keep tests focused** - One feature per test file
2. **Use assertions** - Leverage `assert/*` builtins for validation
3. **Test edge cases** - Include boundary conditions
4. **Document intent** - Use descriptive test names
5. **Expect silence** - Most tests should produce no output (empty .expect files)

## Future Enhancements

- [ ] Add performance benchmarks
- [ ] Add memory leak detection
- [ ] Add stress tests (deep recursion, large data)
- [ ] Add negative tests (expected failures)
- [ ] Add timeout handling for infinite loops

