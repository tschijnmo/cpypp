/** Tests for the utilities related to Python exceptions.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Python exceptions can be detected", "[exception]")
{
    CHECK(check_exc() == 0);

    PyErr_SetString(PyExc_RuntimeError, "Test error");
    CHECK_THROWS_AS(check_exc(), Exc_set);
    PyErr_Clear();
}
