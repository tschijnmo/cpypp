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

//
// Test of building of struct sequences
//
// Here the utility about static type objects are also tested.
//

static PyStructSequence_Field fields[] = { { "field", "doc" }, { NULL, NULL } };

static PyStructSequence_Desc desc = { "StructSequence", "type doc", fields, 1 };

static Static_type tp{};

TEST_CASE("Struct sequence can be constructed",
    "[Handle][Struct_sequence][Static_type]")
{
    tp.init(desc);
    CHECK(tp.is_ready);

    Struct_sequence obj(tp);
    obj.setitem(0, Handle(1l));

    CHECK(obj.getattr("field").as<long>() == 1);
}
