/** Tests for mapping of iterator protocol functions.
 */

#include <algorithm>
#include <iterator>
#include <vector>

#include <catch.hpp>

#include <Python.h>

#include <cpypp.hpp>

using namespace cpypp;

TEST_CASE("Handles has basic iterator protocol", "[Handle][iterator]")
{
    // A simple list of integers.
    auto one_two_three = build_handle("[iii]", 1, 2, 3);

    SECTION("can be correctly iterated over with unary ending predicate")
    {
        long curr = 1;
        for (auto i = one_two_three.begin(); i.has_val(); ++i) {
            check_exc();
            CHECK(*i == build_int(curr));
            check_exc();
            ++curr;
        }
    }

    SECTION("works well with C++ range-for loop")
    {
        long curr = 1;
        for (const auto& i : one_two_three) {
            check_exc();
            CHECK(i.as<long>() == curr);
            check_exc();
            ++curr;
        }
    }
    SECTION("works well with STL algorithm")
    {
        std::vector<long> res{};
        res.reserve(3);
        std::transform(one_two_three.begin(), one_two_three.end(),
            std::back_inserter(res),
            [](const Handle& i) { return i.as<long>(); });

        REQUIRE(res.size() == 3);
        for (size_t i = 0; i < 3; ++i) {
            CHECK(res[i] == i + 1);
        }
    }
}
