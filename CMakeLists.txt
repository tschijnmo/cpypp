cmake_minimum_required(VERSION 3.6)
project(cpypp VERSION 0.1.0 LANGUAGES CXX)

# OPTIONS
option(BUILD_TESTS "Build unit tests" ON)

# Set the building options.
set(CMAKE_CXX_STANDARD 14)

# Find the Python runtime.
include(FindPythonLibs)
message(STATUS
    "Find Python header at ${PYTHON_INCLUDE_DIRS}\nand library at ${PYTHON_LIBRARIES}"
)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()

