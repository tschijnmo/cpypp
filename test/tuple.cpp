/** Tests for the utility for tuples.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Tuples can be constructed", "[Handle][Tuple]")
{
    Tuple tup(2);
    tup.setitem(0, Handle(1l));
    tup.setitem(1, Handle(2l));
    CHECK(tup.check_tuple());

    Handle ref("ii", 1, 2);
    CHECK(ref.check_tuple());
    CHECK(tup == ref);
}
