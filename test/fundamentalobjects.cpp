/** Tests for the utility for fundamental objects.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

//
// Test of the utility for static types.
//
// Based on the noddy example in the CPython C API tutorial.

struct Noddy_obj {
    PyObject_HEAD
    /* Type-specific fields go here. */
};

static Static_type noddy_type({
    PyVarObject_HEAD_INIT(NULL, 0) /* Python object header */
    "Noddy", /* tp_name */
    sizeof(Noddy_obj) - 1, /* tp_basicsize (made smaller by one) */
    0, /* tp_itemsize */
    0, /* tp_dealloc */
    0, /* tp_print */
    0, /* tp_getattr */
    0, /* tp_setattr */
    0, /* tp_reserved */
    0, /* tp_repr */
    0, /* tp_as_number */
    0, /* tp_as_sequence */
    0, /* tp_as_mapping */
    0, /* tp_hash  */
    0, /* tp_call */
    0, /* tp_str */
    0, /* tp_getattro */
    0, /* tp_setattro */
    0, /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT, /* tp_flags */
    "1234", /* tp_doc */
});

TEST_CASE("Trivial static type has basic features", "[Handle][Static_type]")
{

    noddy_type.make_ready([](auto tp) {
        tp->tp_new = PyType_GenericNew;
        ++tp->tp_basicsize; // Restore the basic size.
    });

    CHECK(noddy_type.is_ready());
    auto noddy_ptr = noddy_type.tp_obj();
    auto init_count = Py_REFCNT(noddy_ptr);

    SECTION("Gives correct pointer to the underlying type object")
    {
        CHECK((void*)noddy_ptr == (void*)noddy_type.tp());
    }

    SECTION("Borrowing handles can be created")
    {
        {
            auto handle = noddy_type.get_handle();
            CHECK(handle.get() == noddy_ptr);
            CHECK(handle.if_borrow());
            CHECK(Py_REFCNT(noddy_ptr) == init_count);
        }
        CHECK(Py_REFCNT(noddy_ptr) == init_count);
    }

    SECTION("Owning handles can be created")
    {
        {
            auto handle = noddy_type.get_handle(true);
            CHECK(handle.get() == noddy_ptr);
            CHECK(!handle.if_borrow());
            CHECK(Py_REFCNT(noddy_ptr) == init_count + 1);
        }
        CHECK(Py_REFCNT(noddy_ptr) == init_count);
    }

    SECTION("Type has the right info")
    {
        auto if_type = PyType_Check(noddy_type.get_handle());
        CHECK(if_type);

        // Here we use the doc string as example to test if the information
        // given in the static initializer list is correctly read.
        for (size_t i = 0; i < 4; ++i) {
            CHECK(noddy_type.tp()->tp_doc[i] == '1' + i);
        }
        CHECK(noddy_type.tp()->tp_doc[4] == '\0');

        // Additional operation done in the initialization.
        CHECK(noddy_type.tp()->tp_new == PyType_GenericNew);
    }

    SECTION("The type is not repeatedly initialized")
    {
        // If it is repeated more than once, it will be bumped multiple times.
        CHECK(noddy_type.tp()->tp_basicsize == sizeof(Noddy_obj));

        // Just to really make sure.
        bool executed = false;
        noddy_type.make_ready([&](auto tp) {
            executed = true;
            return;
        });
        CHECK_FALSE(executed);
    }
}
