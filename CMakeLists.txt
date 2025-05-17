cmake_minimum_required(VERSION 3.13)

project(codecrafters-interpreter)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Clang compiler detected. Forcing C++ standard library: libc++")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    list(APPEND CMAKE_EXE_LINKER_FLAGS "-lc++ -lc++abi")
endif()
# 1. Define your source files
# This GLOB_RECURSE will pick up all .cpp and .hpp files in src/ and its subdirectories
# including src/scanner/Scanner.cpp, src/scanner/OperatorTrie.cpp etc.
file(GLOB_RECURSE SOURCE_FILES 
    src/*.cpp 
    src/*.hpp 
    src/scanner/*.cpp 
    src/scanner/*.hpp
)
# Note: It's often better to list .cpp files explicitly or per directory
# and only .hpp files if they are somehow compiled (rare).
# For now, this glob should work to find the .cpp files.

# 2. Add the executable target
add_executable(interpreter ${SOURCE_FILES})

# 3. Set include directories for the target
# This tells CMake (and thus your LSP via compile_commands.json)
# that when compiling files for the "interpreter" target,
# it should look for headers in the "src" directory.
# So, from main.cpp, #include "scanner/Scanner.h" will resolve correctly.
# And from files within src/scanner/, #include "Scanner.h" or #include "OperatorTrie.h"
# will also resolve correctly (as they are relative or found via the src include path).
target_include_directories(interpreter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Optional: Ensure compile_commands.json is generated for your LSP

