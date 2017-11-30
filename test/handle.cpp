/** Tests for the basic object handle.
 */

#include <memory>

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Handles correctly manages reference counts", "[Handle]")
{
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

                CHECK(Py_REFCNT(one) == init_count);
                CHECK(handle.get() == one);
                CHECK(!handle.if_borrow());
                CHECK(handle);
            }
            CHECK(Py_REFCNT(one) == init_count - 1);
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
            Handle handle(nullptr, false, true);
            CHECK(!handle);
        }

        SECTION("can release ownership")
        {
            {
                Handle handle(one);
                CHECK(Py_REFCNT(one) == init_count);
                CHECK(handle.release() == one);
                CHECK(Py_REFCNT(one) == init_count);
            }
            CHECK(Py_REFCNT(one) == init_count);
            Py_DECREF(one); // Decrement of RC for the handle.
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
                    CHECK(handle.get() == handle2.get());
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
                    CHECK(Py_REFCNT(one) == init_count - 1);
                    CHECK(Py_REFCNT(two) == init_count2);
                    CHECK(!handle2);
                }

                CHECK(Py_REFCNT(one) == init_count - 1);
                CHECK(Py_REFCNT(two) == init_count2 - 1);
            }

            SECTION("can be reset with managed object")
            {
                {
                    Handle handle{ one };
                    handle.reset(two);

                    CHECK(Py_REFCNT(one) == init_count - 1);
                    CHECK(Py_REFCNT(two) == init_count2);
                    CHECK(!handle.if_borrow());
                    CHECK(handle.get() == two);
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
        // Testing of borrowing handles are relatively easy, no matter what
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
                Handle handle(one, true);

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
                Handle handle(one, true);

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
                Handle handle(one, true);

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
            CHECK_THROWS_AS(Handle(nullptr, true), Exc_set);
        }

        SECTION("throw no exception at null object when disabled")
        {
            Handle handle(nullptr, true, true);
            CHECK(!handle);
        }

        SECTION("can release ownership")
        {
            {
                Handle handle(one, true);
                check_ref();
                CHECK(handle.release() == one);
                check_ref();
            }
            check_ref();
        }

        SECTION("can be copy assigned with existing managed object")
        {
            {
                Handle handle(one, true);
                Handle handle2(two, true);
                check_ref();

                handle = handle2;
                CHECK(handle.get() == handle2.get());
                CHECK(handle.if_borrow());
                check_ref();
            }

            check_ref();
        }

        SECTION("can be move assigned with existing managed object")
        {
            {
                Handle handle(one, true);
                Handle handle2(two, true);
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
                Handle handle{ one, true };
                handle.reset(two, true);

                CHECK(handle.if_borrow());
                CHECK(handle.get() == two);
                check_ref();
            }

            check_ref();
        }

        SECTION("can be copy assigned with no managed object")
        {
            {
                Handle handle{};
                Handle handle2(two, true);
                CHECK(!handle);
                check_ref();

                handle = handle2;
                CHECK(handle.get() == handle2.get());
                CHECK(handle.if_borrow());
                check_ref();
            }

            check_ref();
        }

        SECTION("can be move assigned with no management object")
        {
            {
                Handle handle{};
                Handle handle2(two, true);
                check_ref();

                handle = std::move(handle2);
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                CHECK(!handle2);
            }

            check_ref();
        }

        SECTION("can be reset with no managed object")
        {
            {
                Handle handle{};
                CHECK(!handle);

                handle.reset(two, true);
                CHECK(handle.get() == two);
                CHECK(handle.if_borrow());
                check_ref();
            }

            check_ref();
        }

        SECTION("can be swapped with another handle")
        {
            {
                Handle handle{ one, true };
                Handle handle2{ two, true };

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
