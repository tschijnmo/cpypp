# Download the Catch2 single header.

file(DOWNLOAD
    "https://github.com/catchorg/Catch2/releases/download/v2.0.1/catch.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/catch.hpp"
)

# The main test driver.
add_executable(testmain
    testmain.cpp
    handle.cpp
    exc_utils.cpp
    number_proto.cpp
    iter_proto.cpp
    parse_build.cpp
    tuple.cpp
    module.cpp
    struct_sequence.cpp
)
target_include_directories(testmain
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE "${PROJECT_SOURCE_DIR}/include"
    PRIVATE ${PYTHON_INCLUDE_DIRS}
)
target_link_libraries(testmain
    PRIVATE ${PYTHON_LIBRARIES}
)

