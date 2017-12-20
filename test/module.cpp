/** Tests for the handle for modules.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Modules have basic features", "[Handle][Module]")
{
    auto mod_ptr = PyModule_New("testmodule");
    auto mod_count = Py_REFCNT(mod_ptr);

    SECTION("only borrows reference by default")
    {
        {
            Module mod(mod_ptr);
        }
        CHECK(Py_REFCNT(mod_ptr) == mod_count);
    }

    SECTION("can be added objects")
    {
        Module mod(mod_ptr);

        mod.add_object("name", Handle(1l));
        CHECK(mod.getattr("name") == Handle(1l));
    }

    Py_DECREF(mod_ptr);
}
