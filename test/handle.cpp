/** Tests for the basic object handle.
 *
 * Here we basically covers the handling of reference count by the handles.  In
 * this basic test case, after each operation, we generally check the Python
 * object pointer, if_borrow, and/or boolean conversion first, before checking
 * the reference counts.
 *
 * Due to the basic nature of the test, higher level utilities from cpypp is
 * deliberately unused.
 */

#include <memory>

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Owning handles correctly manages reference counts", "[Handle]")
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

    PyObject* two = Py_BuildValue("i", 2);
    Py_INCREF(two); // For the same rational as one.
    Py_ssize_t init_count2 = Py_REFCNT(two);

    // Normally, after the handle stole the reference, the reference count
    // should stay the same.  When the handle is destroyed, the reference
    // count will be one less than the above initial value.

    // Check that the reference counts stay at the initial value.
    auto check_init = [&]() {
        CHECK(Py_REFCNT(one) == init_count);
        CHECK(Py_REFCNT(two) == init_count2);
    };

    // Check that both one and two are properly released.
    auto check_both_released = [&]() {
        CHECK(Py_REFCNT(one) == init_count - 1);
        CHECK(Py_REFCNT(two) == init_count2 - 1);
    };

    // Check that one has been properly released with two unused.
    auto check_one_released = [&]() {
        Py_DECREF(two);
        check_both_released();
    };

    SECTION("steals the reference from initialization.")
    {
        {
            Handle handle(one);

            CHECK(handle.get() == one);
            CHECK(handle.is(one));
            CHECK(!handle.if_borrow());
            CHECK(handle);
            check_init();
        }
        check_one_released();
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
        check_one_released();
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
            CHECK(handle2.is(handle));
            CHECK(!handle2.if_borrow());
            CHECK(Py_REFCNT(one) == init_count + 1);

            // The copied handle should not be mutated.
            CHECK(handle.get() == one);
            CHECK(!handle.if_borrow());
        }
        check_one_released();
    }

    SECTION("can be move initialized")
    {
        {
            Handle handle(one);

            Handle handle2(std::move(handle));
            CHECK(handle2.get() == one);
            CHECK(!handle2.if_borrow());
            check_init();

            CHECK(handle.get() == one);
            CHECK(handle.is(one));
            CHECK(handle.if_borrow());
        }
        check_one_released();
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

    SECTION("can be copy assigned with existing managed object")
    {
        {
            Handle handle(one);
            Handle handle2(two);
            CHECK_FALSE(handle.is(handle2));
            check_init();

            handle = handle2;
            CHECK(handle.get() == two);
            CHECK(handle.is(handle2));
            CHECK(!handle.if_borrow());

            CHECK(handle2.get() == two);
            CHECK(!handle2.if_borrow());

            CHECK(Py_REFCNT(one) == init_count - 1);
            CHECK(Py_REFCNT(two) == init_count2 + 1);
        }

        check_both_released();
    }

    SECTION("can be move assigned with existing managed object")
    {
        {
            Handle handle(one);
            Handle handle2(two);
            CHECK_FALSE(handle.is(handle2));
            check_init();

            handle = std::move(handle2);
            CHECK(handle.get() == two);
            CHECK(handle.is(handle2));

            CHECK(!handle.if_borrow());
            CHECK(handle2.get() == two);
            CHECK(handle2.if_borrow());
            CHECK(Py_REFCNT(one) == init_count - 1);
            CHECK(Py_REFCNT(two) == init_count2);
        }

        check_both_released();
    }

    SECTION("can be copy assigned with no managed object")
    {
        {
            Handle handle{};
            Handle handle2(one);
            CHECK(!handle);
            CHECK_FALSE(handle.is(handle2));
            check_init();

            handle = handle2;
            CHECK(handle.get() == one);
            CHECK(handle.is(handle2));
            CHECK(!handle.if_borrow());
            CHECK(Py_REFCNT(one) == init_count + 1);

            CHECK(handle2.get() == one);
            CHECK(!handle2.if_borrow());
        }

        check_one_released();
    }

    SECTION("can be move assigned with no management object")
    {
        {
            Handle handle{};
            Handle handle2(one);
            check_init();
            CHECK_FALSE(handle.is(handle2));

            handle = std::move(handle2);
            CHECK(handle.get() == one);
            CHECK(!handle.if_borrow());
            CHECK(Py_REFCNT(one) == init_count);

            CHECK(handle.is(handle2));
            CHECK(handle2.get() == one);
            CHECK(handle2.if_borrow());
        }

        check_one_released();
    }

    SECTION("can be implicitly cast into raw pointer")
    {
        {
            Handle handle(one);

            auto check_cast
                = [&](PyObject* to_check) { CHECK(to_check == one); };

            check_cast(handle);

            check_init();
        }
        check_one_released();
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
        check_one_released();
    }

    SECTION("can release ownership")
    {
        {
            Handle handle(one);
            check_init();

            CHECK(handle.release() == one);
            check_init();

            CHECK(handle.get() == one);
            CHECK(handle.if_borrow());
        }

        CHECK(Py_REFCNT(one) == init_count);
        Py_DECREF(one); // Decrement of RC for the handle.
        check_one_released();
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
            CHECK(handle.get() == one);
            CHECK(handle.if_borrow());
            CHECK(Py_REFCNT(one) == init_count);
        }

        Py_DECREF(one);
        check_one_released();
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

        check_both_released();
    }

    SECTION("can be reset with no managed object")
    {
        {
            Handle handle{};
            CHECK(!handle);

            handle.reset(one);
            CHECK(handle.get() == one);
            CHECK(!handle.if_borrow());
            check_init();
        }

        check_one_released();
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
            check_init();
        }

        check_both_released();
    }

    Py_DECREF(one);
    Py_DECREF(two);
}

TEST_CASE("Borrowing handles correctly treats reference counts", "[Handle]")
{
    // Testing of borrowing handles is relatively easy, no matter what
    // happens, the reference count should never be touched.
    PyObject* one = Py_BuildValue("i", 1);
    PyObject* two = Py_BuildValue("i", 2);
    Py_ssize_t init_count = Py_REFCNT(one);
    Py_ssize_t init_count2 = Py_REFCNT(two);
    auto check_ref = [&]() {
        CHECK(Py_REFCNT(one) == init_count);
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

            CHECK(handle.get() == one);
            CHECK(handle.if_borrow());
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

    SECTION("can be copy assigned with existing managed object")
    {
        {
            Handle handle(one, BORROW);
            Handle handle2(two, BORROW);
            CHECK_FALSE(handle.is(handle2));
            check_ref();

            handle = handle2;
            CHECK(handle.is(handle2));
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
            CHECK_FALSE(handle.is(handle2));
            check_ref();

            handle = std::move(handle2);
            CHECK(handle.is(handle2));
            CHECK(handle.get() == two);
            CHECK(handle.if_borrow());
            CHECK(handle2.get() == two);
            CHECK(handle2.if_borrow());
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
            CHECK_FALSE(handle.is(handle2));
            check_ref();

            handle = handle2;
            CHECK(handle.is(handle2));
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
            CHECK_FALSE(handle.is(handle2));

            handle = std::move(handle2);
            CHECK(handle.is(handle2));
            CHECK(handle.get() == two);
            CHECK(handle.if_borrow());
            CHECK(handle2.get() == two);
            CHECK(handle2.if_borrow());
            check_ref();
        }

        check_ref();
    }

    SECTION("can create new references")
    {
        {
            Handle handle(one, BORROW);
            check_ref();

            CHECK(handle.get_new() == one);
            CHECK(handle.get() == one);
            CHECK(Py_REFCNT(one) == init_count + 1);
        }
        CHECK(Py_REFCNT(one) == init_count + 1);
        Py_DECREF(one);
        check_ref();
    }

    SECTION("can release ownership")
    {
        {
            Handle handle(one, BORROW);
            check_ref();
            CHECK(handle.release() == one);
            CHECK(Py_REFCNT(one) == init_count + 1);
            Py_DECREF(one);

            CHECK(handle.get() == one);
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
            CHECK(Py_REFCNT(one) == init_count + 1);
            Py_DECREF(one);
            check_ref();

            // Moving from borrowing handle does not touch the handle
            // itself.
            ref = get_new(std::move(handle));
            CHECK(ref == one);
            CHECK(handle.get() == one);
            CHECK(Py_REFCNT(one) == init_count + 1);
            Py_DECREF(one);
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
