
cmake_minimum_required(VERSION 3.20)
project(clear)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2 COMPONENTS graphics window system REQUIRED)

# CLEAR Editor
add_executable(clear
        editor.cpp
)

target_link_libraries(clear PRIVATE
        sfml-graphics
        sfml-window
        sfml-system
)

# CLEAR Interpreter
add_executable(clear_interpreter
        interpreter/main.cpp
        interpreter/lexer.cpp
        interpreter/parser.cpp
        interpreter/executer.cpp
)
