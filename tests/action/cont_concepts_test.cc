#include <gtest/gtest.h>
#include <array>
#include <vector>
#include <memory>
#include <fermat/core.h>
#include <fermat/action/concepts.h>
#include <fermat/view/ref.h>

namespace {
    // compile-time checks
    static_assert(fermat::ranges::range<int[6]>);
    static_assert(!fermat::ranges::semi_container<int[6]>);

    static_assert(fermat::ranges::semi_container<std::array<int, 6>>);
    static_assert(!fermat::ranges::container<std::array<int, 6>>);

    static_assert(fermat::ranges::container<std::vector<int>>);
    static_assert(fermat::ranges::container<std::vector<std::unique_ptr<int>>>);

    static_assert(fermat::ranges::lvalue_container_like<decltype(std::declval<std::vector<std::unique_ptr<int>>&>())>);
    static_assert(!fermat::ranges::lvalue_container_like<decltype(std::declval<std::vector<std::unique_ptr<int>>>())>);

    static_assert(fermat::ranges::lvalue_container_like<decltype(fermat::ranges::views::ref(std::declval<std::vector<std::unique_ptr<int>>&>()))>);
} // namespace

TEST(RangeConceptsTest, CompileTimeChecks) {
    SUCCEED();
}