#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <Python.h>

int main(int argc, char* argv[])
{
    Py_Initialize();
    int result = Catch::Session().run(argc, argv);
    Py_Finalize();
    return result;
}
