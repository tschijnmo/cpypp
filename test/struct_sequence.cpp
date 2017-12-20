/** Tests of handling of struct sequence types and objects.
 *
 * The type handling is also tested here.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

static PyStructSequence_Field fields[] = { { "field", "doc" }, { NULL, NULL } };

static PyStructSequence_Desc desc = { "StructSequence", "type doc", fields, 1 };

static Static_type tp{};

TEST_CASE("Struct sequence works", "[Handle][Struct_sequence][Static_type]")
{
    tp.init(desc);
    CHECK(tp.is_ready);

    Struct_sequence obj(tp);
    obj.setitem(0, Handle(1l));

    CHECK(obj.getattr("field").as<long>() == 1);
}
