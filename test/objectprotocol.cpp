/** Tests for the basic object protocol.
 */

#include <memory>

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Handles can manipulate attributes", "[Handle]")
{
    SECTION("can get attributes")
    {
        Handle one(1l);
        Handle real_part = one.getattr("real");

        // Singleton requirement of the initial intergers.
        CHECK(real_part.is(one));
    }

    SECTION("can mutate attributes")
    {
        Handle one(1l);
        Handle obj(PyModule_New("dummymodule"));
        obj.setattr("aa", one);
        REQUIRE(obj.getattr("aa").is(one));

        obj.delattr("aa");
        CHECK_THROWS_AS(obj.getattr("aa"), Exc_set);
        PyObject* exc = PyErr_Occurred();
        REQUIRE(exc != nullptr);
        CHECK(PyErr_GivenExceptionMatches(exc, PyExc_AttributeError));
        PyErr_Clear();
    }
}

TEST_CASE("Handles support comparison operations", "[Handle]")
{
    // In order to make sure that we actually delegate to the Python comparison
    // operation, simple lists with different identities are used.

    Handle small("[ii]", 1, 1);
    Handle small2("[ii]", 1, 1);
    Handle big("[ii]", 1, 2);

    SECTION("The handled objects has different identity")
    {
        // Here we parallel raw pointer comparison with `is` comparison to have
        // `is` tested as well.
        CHECK(small.get() != small2.get());
        CHECK_FALSE(small.is(small2));

        Handle dup{ small };
        CHECK(dup.get() == small.get());
        CHECK(dup.is(small));
    }

    SECTION("Supports less-than")
    {
        CHECK(small < big);
        CHECK_FALSE(small < small2);
        CHECK_FALSE(big < small);
    }

    SECTION("Supports less-than-or-equal")
    {
        CHECK(small <= big);
        CHECK(small <= small2);
        CHECK_FALSE(big <= small);
    }

    SECTION("Supports equal")
    {
        CHECK(small == small2);
        CHECK_FALSE(small == big);
    }

    SECTION("Supports not-equal")
    {
        CHECK_FALSE(small != small2);
        CHECK(small != big);
    }

    SECTION("Supports greater-than")
    {
        CHECK_FALSE(small > small2);
        CHECK(big > small);
        CHECK_FALSE(small > big);
    }

    SECTION("Supports greater-than-or-equal")
    {
        CHECK(small >= small2);
        CHECK(big >= small);
        CHECK_FALSE(small >= big);
    }

    SECTION("Reports invalid comparisons")
    {
        Handle one(1l);
        CHECK_THROWS_AS(small < one, Exc_set);
        PyObject* exc = PyErr_Occurred();
        REQUIRE(exc != nullptr);
        CHECK(PyErr_GivenExceptionMatches(exc, PyExc_TypeError));
        PyErr_Clear();
    }
}
