/** Tests for iterator protocol functionality.
 */

#include <algorithm>
#include <iterator>
#include <vector>

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Handles has basic iterator protocol", "[Handle][Iter_handle]")
{
    // A simple list of integers.
    Handle one_two_three("[iii]", 1, 2, 3);

    SECTION("can be correctly iterated over with unary ending predicate")
    {
        long curr = 1;
        for (auto i = one_two_three.begin(); i.has_val(); ++i) {
            check_exc();
            CHECK(*i == Handle(curr));
            check_exc();
            ++curr;
        }
    }

    SECTION("works well with C++ range-for loop")
    {
        long curr = 1;
        for (const auto& i : one_two_three) {
            check_exc();
            CHECK(i.as<long>() == curr);
            check_exc();
            ++curr;
        }
    }
    SECTION("works well with STL algorithm")
    {
        std::vector<long> res{};
        res.reserve(3);
        std::transform(one_two_three.begin(), one_two_three.end(),
            std::back_inserter(res),
            [](const Handle& i) { return i.as<long>(); });

        REQUIRE(res.size() == 3);
        for (size_t i = 0; i < 3; ++i) {
            CHECK(res[i] == i + 1);
        }
    }
}

TEST_CASE("Iterator utilities report wrong types", "[Handle][Iter_handle]")
{
    // A non-iterable and non-iterator integer.
    Handle one(1l);

    SECTION("reports non-iterable objects")
    {
        CHECK_THROWS_AS(one.begin(), Exc_set);
        PyObject* exc = PyErr_Occurred();
        REQUIRE(exc != nullptr);
        CHECK(PyErr_GivenExceptionMatches(exc, PyExc_TypeError));
        PyErr_Clear();
    }

    SECTION("reports non-iterator")
    {
        CHECK_THROWS_AS(Iter_handle(one.get_new()), Exc_set);
        PyObject* exc = PyErr_Occurred();
        REQUIRE(exc != nullptr);
        CHECK(PyErr_GivenExceptionMatches(exc, PyExc_TypeError));
        PyErr_Clear();
    }
}
