/** Tests for the basic object handle.
 */

#include <memory>

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Handles correctly manages reference counts", "[Handle]")
{

    // In this basic test case, after each operation, we generally check the
    // Python object, if_borrow, or boolean conversion first, before checking
    // the reference counts.
    //
    // Due to the basic nature of the test, higher level utilities from cpypp
    // is deliberately unused.

    SECTION("Owning handles")
    {
        PyObject* one = Py_BuildValue("i", 1);
        // Py_BuildValue gives a new reference to be possibly stolen by
        // Handles.  Here we increment the reference count again to make sure
        // that even after that reference is destroyed, we still have at least
        // one reference.  Practically this is unnecessary in that the integer
        // unity normally has hundreds of references to prevent it from being
        // collected.
        Py_INCREF(one);
        Py_ssize_t init_count = Py_REFCNT(one);

        SECTION("steals the reference from initialization.")
        {
            {
                Handle handle(one);

                CHECK(handle.get() == one);
                CHECK(!handle.if_borrow());
                CHECK(handle);
                CHECK(Py_REFCNT(one) == init_count);
            }
            CHECK(Py_REFCNT(one) == init_count - 1);
        }

        SECTION("can create new reference from initialization.")
        {
            {
                Handle handle(one, NEW);

                CHECK(handle.get() == one);
                CHECK(!handle.if_borrow());
                CHECK(handle);
                CHECK(Py_REFCNT(one) == init_count + 1);
            }
            CHECK(Py_REFCNT(one) == init_count);
            Py_DECREF(one);
        }

        SECTION("can be default initialized")
        {
            Handle handle{};
            CHECK(handle.get() == nullptr);
            CHECK(!handle);
        }

        SECTION("can be copy initialized")
        {
            {
                Handle handle(one);

                Handle handle2(handle);
                CHECK(handle2.get() == one);
                CHECK(!handle2.if_borrow());
                CHECK(Py_REFCNT(one) == init_count + 1);

                // The copied handle should not be mutated.
                CHECK(handle.get() == one);
                CHECK(!handle.if_borrow());
            }
            CHECK(Py_REFCNT(one) == init_count - 1);
        }

        SECTION("can be move initialized")
        {
            {
                Handle handle(one);

                Handle handle2(std::move(handle));
                CHECK(handle2.get() == one);
                CHECK(!handle2.if_borrow());
                CHECK(Py_REFCNT(one) == init_count);

                CHECK(!handle);
            }
            CHECK(Py_REFCNT(one) == init_count - 1);
        }

        SECTION("throw exception at null object when asked")
        {
            CHECK_THROWS_AS(Handle(nullptr), Exc_set);
        }

        SECTION("throw no exception at null object when disabled")
        {
            Handle handle(nullptr, STEAL, true);
            CHECK(!handle);
        }

        SECTION("can create new references")
        {
            {
                Handle handle(one);

                CHECK(handle.get_new() == one);
                CHECK(handle.get() == one);
                CHECK(Py_REFCNT(one) == init_count + 1);
            }
            CHECK(Py_REFCNT(one) == init_count);
            Py_DECREF(one);
        }

        SECTION("can release ownership")
        {
            {
                Handle handle(one);

                CHECK(handle.release() == one);
                CHECK(!handle);
                CHECK(Py_REFCNT(one) == init_count);
            }
            CHECK(Py_REFCNT(one) == init_count);
            Py_DECREF(one); // Decrement of RC for the handle.
        }

        SECTION("can be extracted a new reference")
        {
            {
                Handle handle(one);
                auto ref = get_new(handle);
                CHECK(ref == one);
                CHECK(handle.get() == one);
                CHECK(Py_REFCNT(one) == init_count + 1);
                Py_DECREF(one);

                ref = get_new(std::move(handle));
                CHECK(ref == one);
                CHECK(!handle);
                CHECK(Py_REFCNT(one) == init_count);
            }

            Py_DECREF(one);
            CHECK(Py_REFCNT(one) == init_count - 1);
        }

        SECTION("has correct assignment assignments")
        {
            PyObject* two = Py_BuildValue("i", 2);
            Py_INCREF(two); // For the same rational as one.
            Py_ssize_t init_count2 = Py_REFCNT(two);

            SECTION("can be copy assigned with existing managed object")
            {
                {
                    Handle handle(one);
                    Handle handle2(two);
                    CHECK(Py_REFCNT(one) == init_count);
                    CHECK(Py_REFCNT(two) == init_count2);

                    handle = handle2;
                    CHECK(handle.get() == two);
                    CHECK(!handle.if_borrow());
                    CHECK(handle2.get() == two);
                    CHECK(!handle2.if_borrow());
                    CHECK(Py_REFCNT(one) == init_count - 1);
                    CHECK(Py_REFCNT(two) == init_count2 + 1);
                }

                CHECK(Py_REFCNT(one) == init_count - 1);
                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            SECTION("can be move assigned with existing managed object")
            {
                {
                    Handle handle(one);
                    Handle handle2(two);
                    CHECK(Py_REFCNT(one) == init_count);
                    CHECK(Py_REFCNT(two) == init_count2);

                    handle = std::move(handle2);
                    CHECK(handle.get() == two);
                    CHECK(!handle.if_borrow());
                    CHECK(!handle2);
                    CHECK(Py_REFCNT(one) == init_count - 1);
                    CHECK(Py_REFCNT(two) == init_count2);
                }

                CHECK(Py_REFCNT(one) == init_count - 1);
                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            SECTION("can be reset with managed object")
            {
                {
                    Handle handle{ one };
                    handle.reset(two);

                    CHECK(handle.get() == two);
                    CHECK(!handle.if_borrow());
                    CHECK(Py_REFCNT(one) == init_count - 1);
                    CHECK(Py_REFCNT(two) == init_count2);
                }

                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            SECTION("can be copy assigned with no managed object")
            {
                {
                    Handle handle{};
                    Handle handle2(two);
                    CHECK(!handle);
                    CHECK(Py_REFCNT(two) == init_count2);

                    handle = handle2;
                    CHECK(handle.get() == handle2.get());
                    CHECK(!handle.if_borrow());
                    CHECK(Py_REFCNT(two) == init_count2 + 1);
                }

                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            SECTION("can be move assigned with no management object")
            {
                {
                    Handle handle{};
                    Handle handle2(two);
                    CHECK(Py_REFCNT(two) == init_count2);

                    handle = std::move(handle2);
                    CHECK(handle.get() == two);
                    CHECK(!handle.if_borrow());
                    CHECK(Py_REFCNT(two) == init_count2);
                    CHECK(!handle2);
                }

                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            SECTION("can be reset with no managed object")
            {
                {
                    Handle handle{};
                    CHECK(!handle);

                    handle.reset(two);
                    CHECK(handle.get() == two);
                    CHECK(!handle.if_borrow());
                    CHECK(Py_REFCNT(two) == init_count2);
                }

                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            SECTION("can be swapped with another handle")
            {
                {
                    Handle handle{ one };
                    Handle handle2{ two };

                    handle.swap(handle2);
                    CHECK(handle.get() == two);
                    CHECK(!handle.if_borrow());
                    CHECK(handle2.get() == one);
                    CHECK(!handle2.if_borrow());
                    CHECK(Py_REFCNT(one) == init_count);
                    CHECK(Py_REFCNT(two) == init_count2);
                }

                CHECK(Py_REFCNT(one) == init_count - 1);
                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            Py_DECREF(two);
        }

        Py_DECREF(one);
    }

    SECTION("Borrowing handles")
    {
        // Testing of borrowing handles is relatively easy, no matter what
        // happens, the reference count should never be touched.
        PyObject* one = Py_BuildValue("i", 1);
        PyObject* two = Py_BuildValue("i", 2);
        Py_ssize_t init_count1 = Py_REFCNT(one);
        Py_ssize_t init_count2 = Py_REFCNT(two);
        auto check_ref = [&]() {
            CHECK(Py_REFCNT(one) == init_count1);
            CHECK(Py_REFCNT(two) == init_count2);
        };

        SECTION("touch no reference count from initialization.")
        {
            {
                Handle handle(one, BORROW);

                CHECK(handle.get() == one);
                CHECK(handle.if_borrow());
                CHECK(handle);
                check_ref();
            }
            check_ref();
        }

        SECTION("can be copy initialized")
        {
            {
                Handle handle(one, BORROW);

                Handle handle2(handle);
                CHECK(handle2.get() == one);
                CHECK(handle2.if_borrow());
                check_ref();

                // The copied handle should not be mutated.
                CHECK(handle.get() == one);
                CHECK(handle.if_borrow());
            }
            check_ref();
        }

        SECTION("can be move initialized")
        {
            {
                Handle handle(one, BORROW);

                Handle handle2(std::move(handle));
                CHECK(handle2.get() == one);
                CHECK(handle2.if_borrow());
                check_ref();

                CHECK(!handle);
            }
            check_ref();
        }

        SECTION("throw exception at null object when asked")
        {
            CHECK_THROWS_AS(Handle(nullptr, BORROW), Exc_set);
        }

        SECTION("throw no exception at null object when disabled")
        {
            Handle handle(nullptr, BORROW, true);
            CHECK(!handle);
        }

        SECTION("can create new references")
        {
            {
                Handle handle(one, BORROW);
                check_ref();

                CHECK(handle.get_new() == one);
                CHECK(handle.get() == one);
                CHECK(Py_REFCNT(one) == init_count1 + 1);
            }
            CHECK(Py_REFCNT(one) == init_count1 + 1);
            Py_DECREF(one);
        }

        SECTION("can release ownership")
        {
            {
                Handle handle(one, BORROW);
                check_ref();
                CHECK(handle.release() == one);
                check_ref();
            }
            check_ref();
        }

        SECTION("can be extracted a new reference")
        {
            {
                Handle handle(one, BORROW);
                check_ref();

                auto ref = get_new(handle);
                CHECK(ref == one);
                CHECK(handle.get() == one);
                CHECK(Py_REFCNT(one) == init_count1 + 1);
                Py_DECREF(one);
                check_ref();

                // Moving from borrowing handle does not touch the handle
                // itself.
                ref = get_new(std::move(handle));
                CHECK(ref == one);
                CHECK(handle.get() == one);
                CHECK(Py_REFCNT(one) == init_count1 + 1);
                Py_DECREF(one);
                check_ref();
            }

            check_ref();
        }

        SECTION("can be copy assigned with existing managed object")
        {
            {
                Handle handle(one, BORROW);
                Handle handle2(two, BORROW);
                check_ref();

                handle = handle2;
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                CHECK(handle2.get() == two);
                CHECK(handle2.if_borrow());
                check_ref();
            }

            check_ref();
        }

        SECTION("can be move assigned with existing managed object")
        {
            {
                Handle handle(one, BORROW);
                Handle handle2(two, BORROW);
                check_ref();

                handle = std::move(handle2);
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                CHECK(!handle2);
                check_ref();
            }

            check_ref();
        }

        SECTION("can be reset with managed object")
        {
            {
                Handle handle{ one, BORROW };
                handle.reset(two, BORROW);

                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                check_ref();
            }

            check_ref();
        }

        SECTION("can read in the Python object to handle")
        {
            {
                Handle handle{};
                CHECK(!handle);

                *handle.read() = one;

                CHECK(handle.get() == one);
                CHECK(handle.if_borrow());
                CHECK(handle);
                check_ref();
            }
            check_ref();
        }

        SECTION("can be copy assigned with no managed object")
        {
            {
                Handle handle{};
                Handle handle2(two, BORROW);
                CHECK(!handle);
                check_ref();

                handle = handle2;
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                CHECK(handle2.get() == two);
                CHECK(handle2.if_borrow());
                check_ref();
            }

            check_ref();
        }

        SECTION("can be move assigned with no management object")
        {
            {
                Handle handle{};
                Handle handle2(two, BORROW);

                handle = std::move(handle2);
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                CHECK(!handle2);
                check_ref();
            }

            check_ref();
        }

        SECTION("can be reset with no managed object")
        {
            {
                Handle handle{};
                CHECK(!handle);

                handle.reset(two, BORROW);
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                check_ref();
            }

            check_ref();
        }

        SECTION("can be swapped with another handle")
        {
            {
                Handle handle{ one, BORROW };
                Handle handle2{ two, BORROW };

                handle.swap(handle2);
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                CHECK(handle2.get() == one);
                CHECK(handle2.if_borrow());
                check_ref();
            }

            check_ref();
        }

        Py_DECREF(two);
        Py_DECREF(one);
    }
}

TEST_CASE("Handles support comparison operations", "[Handle][Comparison]")
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
