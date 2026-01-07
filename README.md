

Praxis is a minimalist Lisp interpreter implemented in C. This project is a study in language implementation, focusing on parsing, evaluation, and runtime representation rather than completeness or performance. It follows the general structure proposed in [Build Your Own Lisp](https://www.buildyourownlisp.com/), with deliberate design choices and deviations made for clarity and experimentation.

The goal of Praxis is not to recreate an existing Lisp dialect, but to understand how a Lisp-like language works from the inside out.

## Architectural Overview

Praxis is a tree-walking interpreter built around three core components: parsing, value representation, and evaluation. The system is intentionally small and explicit, favoring readability and mechanical clarity over abstraction.

### Parsing and Syntax

Input is read as raw text and parsed into an abstract syntax tree using a grammar-based parser.

- **Grammar:**
  - The language is defined using a simple context-free grammar describing numbers, symbols, S-expressions, and Q-expressions.
  - Parentheses `()` represent S-expressions, which are evaluated.
  - Braces `{}` represent Q-expressions, which are treated as quoted lists.

- **AST Construction:**
  - The parser produces a concrete syntax tree which is then converted into internal Lisp values.
  - Tokens such as parentheses and grammar tags are explicitly filtered during this transformation step.

This separation allows parsing to remain declarative while evaluation operates on a clean, language-specific structure.

### Value Representation

All runtime values are represented using a tagged union, typically named `lval`.

Supported value types include:

- Numbers
- Symbols
- Errors
- Functions
- S-expressions
- Q-expressions

Each `lval` owns its memory, including dynamically allocated child values for expressions. This makes memory management explicit and forces correctness in construction and destruction.

- **Lists:** S-expressions and Q-expressions are represented as dynamic arrays of `lval*`.
- **Functions:** Builtins are represented as C function pointers, while user-defined functions capture their environment.

### Evaluation Model

Praxis uses a classic Lisp evaluation model.

- **S-expressions:**
  1. Each child expression is evaluated from left to right.
  2. The first element is treated as a function.
  3. The remaining elements are treated as arguments.
  4. The function is applied to the evaluated arguments.

- **Q-expressions:**
  - Q-expressions are not evaluated automatically.
  - They can be manipulated as data and explicitly converted into S-expressions.

This model keeps the evaluator small while still enabling non-trivial language features.

### Environment and Scoping

Variables and functions live inside an explicit environment structure.

- **Environment Structure:**
  - Each environment maps symbols to values.
  - Environments may have a parent, enabling lexical scoping.

- **Lookup Rules:**
  - Symbol resolution walks up the environment chain until a binding is found or an error is raised.

This design supports both global bindings and local scopes created during function calls.

### Built-in Functions

Praxis includes a small but expressive set of built-in functions implemented in C.

Categories include:

- Arithmetic operations
- List manipulation
- Comparison and conditionals
- Environment mutation and definition

Builtins are registered at startup and inserted into the global environment as first-class values.

### Error Handling

Errors are explicit values in the language.

- Any evaluation step may return an error `lval`.
- Errors propagate naturally through evaluation without requiring special control flow in C.

This keeps both the interpreter logic and the language semantics simple and predictable.

## REPL

Praxis provides an interactive Read-Eval-Print Loop.

- Input is read line by line.
- Each line is parsed, evaluated, and printed.
- Errors are reported without terminating the interpreter.

The REPL is the primary interface for interacting with the language during development.

## Memory Management

Memory management is manual and explicit.

- All `lval` instances are heap allocated.
- Each value has a corresponding destructor responsible for recursively freeing owned memory.
- Ownership rules are strict, making memory errors visible early during development.

The absence of garbage collection is a deliberate choice to keep resource lifetimes explicit and understandable.

## Building and Running

### Prerequisites

You will need:

- A C compiler with C99 support (Clang or GCC)
- Make
- A POSIX-compatible environment

### Build

```bash
make
````

### Run

```bash
./praxis
```

This launches the interactive REPL.

## Project Philosophy

Praxis is a learning project.

It prioritizes:

* Explicit control flow
* Visible data structures
* Minimal hidden behavior
* Understanding over convenience

The implementation favors clarity over cleverness, and correctness over optimization.

## References

* *Build Your Own Lisp* by Daniel Holden
* Classic Lisp interpreters and language texts
