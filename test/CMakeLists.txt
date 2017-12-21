# Download the Catch2 single header.

file(DOWNLOAD
    "https://github.com/catchorg/Catch2/releases/download/v2.0.1/catch.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/catch.hpp"
)

# The main test driver.
add_executable(testmain
    testmain.cpp
    handle.cpp
    exc.cpp
    parsebuild.cpp
    objectprotocol.cpp
    numberprotocol.cpp
    iteratorprotocol.cpp
    fundamentalobjects.cpp
    numericobjects.cpp
    sequenceobjects.cpp
    otherobjects.cpp
)

target_include_directories(testmain
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE "${PROJECT_SOURCE_DIR}/include"
    PRIVATE ${PYTHON_INCLUDE_DIRS}
)
target_link_libraries(testmain
    PRIVATE ${PYTHON_LIBRARIES}
)

