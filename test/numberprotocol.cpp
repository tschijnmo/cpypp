/** Tests for Number protocol functions.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Handles maps basic number protocol", "[Handle]")
{
    // Basically two simple integers are used for the testing.
    long num1 = 6;
    long num2 = 13;
    Handle handle1(num1);
    Handle handle2(num2);

    SECTION("Gives correct number check")
    {
        CHECK(handle1.check_number());
        Handle empty{};
        CHECK_FALSE(empty.check_number());
        Handle tup("ii", 1, 2);
        CHECK_FALSE(tup.check_number());
    }

    SECTION("adds correctly")
    {
        auto res = handle1 + handle2;
        CHECK(res == Handle(num1 + num2));
    }

    SECTION("multiplies correctly")
    {
        auto res = handle1 * handle2;
        CHECK(res == Handle(num1 * num2));
    }

    SECTION("makes divmod correctly")
    {
        auto res = handle2.divmod(handle1);
        CHECK(res.first == Handle(num2 / num1));
        CHECK(res.second == Handle(num2 % num1));
    }
}
