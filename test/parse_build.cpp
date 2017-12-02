/** Tests for the utilities to building Python objects.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Utilities can make simple integers", "[Handle][mk]")
{

    // Here we use simple integer of unity for the testing, since the result
    // can be tested with relative ease due to the Python singleton handling of
    // first few integers.

    PyObject* one = PyLong_FromLong(1);
    Py_ssize_t curr_count = Py_REFCNT(one);

    SECTION("the generic function works")
    {
        {
            auto from_gen = build_handle("i", 1);
            CHECK(from_gen.get() == one);
            CHECK(Py_REFCNT(one) == curr_count + 1);
        }
        CHECK(Py_REFCNT(one) == curr_count);
    }

    SECTION("built-in int can be constructed")
    {
        auto check_build_int = [&](auto v) {
            {
                auto from_gen = build_int(v);
                CHECK(from_gen.get() == one);
                CHECK(Py_REFCNT(one) == curr_count + 1);
            }
            CHECK(Py_REFCNT(one) == curr_count);
        };

        check_build_int(1l);
        check_build_int(1ul);
    }

    Py_DECREF(one);
}
