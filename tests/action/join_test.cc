#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <fermat/core.h>
#include <fermat/action/join.h>
#include <fermat/algorithm/move.h>
#include <fermat/algorithm/equal.h>
#include <fermat/view/transform.h>

TEST(ActionJoinTest, MoveAndJoin) {
    using namespace fermat::ranges;

    std::vector<std::string> v{"hello", " ", "world"};
    auto s = v | move | actions::join;
    static_assert(std::is_same_v<decltype(s), std::string>, "s should be std::string");
    EXPECT_EQ(s, "hello world");
}

TEST(ActionJoinTest, TransformAllAndJoin) {
    using namespace fermat::ranges;

    std::vector<std::string> v{"hello", " ", "world"};
    auto s2 = v | views::transform(views::all) | actions::join;
    static_assert(std::is_same_v<decltype(s2), std::vector<char>>, "s2 should be std::vector<char>");
    std::string result(s2.begin(), s2.end());
    EXPECT_EQ(result, "hello world");
}