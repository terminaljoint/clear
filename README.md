# CLEAR

CLEAR is a programming language and editor project created by Murugan Santhosh.

## Build prerequisites

- C++ compiler with C++14 support
- CMake 3.20 or newer
- SFML 2 (graphics, window, and system components)

On Fedora, install the build dependencies with:

```bash
sudo dnf install gcc-c++ cmake SFML-devel
```

## Build

Run these commands from the repository root:

```bash
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
```

The editor is built at `cmake-build-debug/clear`. The interpreter is placed at `interpreter/clear_interpreter`, the path used by the editor's F5 runner.

## Run the editor

Always start the editor from the repository root so its relative interpreter path and temporary source file resolve correctly:

```bash
./cmake-build-debug/clear
```

Inside the editor, press **F5** to run the current buffer.

## Current interpreter status

The C++ interpreter currently supports integer arithmetic expressions, variables, and `print`. Language features such as conditionals, loops, functions, and connector syntax are still under development.
