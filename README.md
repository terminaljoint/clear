# CLEAR

CLEAR is a programming language and editor project.

## Requirements

- C++14 compiler
- CMake 3.20+
- SFML 2

Fedora:

    sudo dnf install gcc-c++ cmake SFML-devel

## Build

From the project root:

    cmake -S . -B build
    cmake --build build

## Run

    ./build/clear

Press F5 inside the editor to execute the current CLEAR program.

## Interpreter

The C++ interpreter now supports:
- Integer arithmetic, comparisons, precedence, parentheses and unary signs
- Variables, multiple assignment and block-local scope
- Print, input, ternary expressions
- Nested if / elif / else and inclusive ascending or descending loops
- Triple-quoted comments
- Module and function definitions using @@@m / @@@f, calls with @ or @@, and return

Run the smoke test:
    ./interpreter/clear_interpreter tests/interpreter_features.clear

The editor output panel supports mouse selection and Ctrl+C, and Ctrl+A when output is focused.
