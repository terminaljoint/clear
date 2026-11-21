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

The C++ interpreter currently supports integer arithmetic, variables, and print. Other language features are under development.
