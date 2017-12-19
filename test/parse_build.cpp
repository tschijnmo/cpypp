/** Tests for the utilities to building and parsing Python objects.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE(
    "Utilities can make and parse simple integers", "[Handle][build][parse]")
{

    // Here we use simple integer of unity for the testing, since the result
    // can be tested with relative ease due to the Python singleton handling of
    // first few integers.

    PyObject* one = PyLong_FromLong(1);
    Py_ssize_t curr_count = Py_REFCNT(one);

    SECTION("the generic function works")
    {
        {
            Handle from_gen("i", 1);
            CHECK(from_gen.get() == one);
            CHECK(Py_REFCNT(one) == curr_count + 1);

            long val;
            from_gen.as(val);
            CHECK(val == 1);
            CHECK(from_gen.as<long>() == 1);
        }
        CHECK(Py_REFCNT(one) == curr_count);
    }

    SECTION("special integer building function works")
    {
        auto check_build_int = [&](auto v) {
            {
                Handle from_gen(v);
                CHECK(from_gen.get() == one);
                CHECK(Py_REFCNT(one) == curr_count + 1);

                long val;
                from_gen.as(val);
                CHECK(val == 1);
                CHECK(from_gen.as<long>() == 1);
            }
            CHECK(Py_REFCNT(one) == curr_count);
        };

        check_build_int(1l);
        check_build_int(1ul);
    }

    Py_DECREF(one);
}
