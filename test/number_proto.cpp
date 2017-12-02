/** Tests for mapping of Number protocol functions.
 */

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Handles maps basic number protocol", "[Handle][Number]")
{
    // Basically two simple integers are used for the testing.
    long num1 = 6;
    long num2 = 13;
    auto handle1 = build_int(num1);
    auto handle2 = build_int(num2);

    SECTION("Gives correct number check")
    {
        CHECK(handle1.check_number());
        Handle empty{};
        CHECK_FALSE(empty.check_number());
        auto tup = build_handle("ii", 1, 2);
        CHECK_FALSE(tup.check_number());
    }

    SECTION("adds correctly")
    {
        auto res = handle1 + handle2;
        CHECK(res == build_int(num1 + num2));
    }

    SECTION("multiplies correctly")
    {
        auto res = handle1 * handle2;
        CHECK(res == build_int(num1 * num2));
    }

    SECTION("makes divmod correctly")
    {
        auto res = handle2.divmod(handle1);
        CHECK(res.first == build_int(num2 / num1));
        CHECK(res.second == build_int(num2 % num1));
    }
}
