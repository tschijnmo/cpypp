/** Tests for the utilities to related numeric objects.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Integer can be built and parsed", "[Handle]")
{

    // Here we use simple integer of unity again, with the Python singleton
    // handling of it.

    PyObject* one = PyLong_FromLong(1);
    Py_ssize_t curr_count = Py_REFCNT(one);

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

    SECTION("long") { check_build_int(1l); }

    SECTION("unsigned long") { check_build_int(1ul); }

    Py_DECREF(one);
}
