#include <gtest/gtest.h>
#include <cctype>
#include <string>
#include <vector>
#include <fermat/action/split.h>
#include <fermat/action/split_when.h>
#include <fermat/algorithm/move.h>
#include <fermat/core.h>
#include <fermat/view/c_str.h>
#include <fermat/view/iota.h>
#include <fermat/range/conversion.h>

using namespace fermat::ranges;

TEST(ActionSplitTest, SplitVectorByValue) {
    auto v = views::ints(1, 21) | to<std::vector>();
    std::vector<std::vector<int>> rgv = actions::split(v, 10);
    EXPECT_EQ(rgv.size(), 2u);
    EXPECT_TRUE((rgv[0] == std::vector<int>{1,2,3,4,5,6,7,8,9}));
    EXPECT_TRUE((rgv[1] == std::vector<int>{11,12,13,14,15,16,17,18,19,20}));
}

TEST(ActionSplitTest, SplitWhenVector) {
    auto v = views::ints(1, 21) | to<std::vector>();
    using I = std::vector<int>::iterator;
    std::vector<std::vector<int>> rgv2 = actions::split_when(
        v, [](I b, I) { return std::make_pair(0 == (*b) % 2, next(b)); });
    EXPECT_EQ(rgv2.size(), 10u);
    EXPECT_TRUE((rgv2[0] == std::vector<int>{1}));
    EXPECT_TRUE((rgv2[1] == std::vector<int>{3}));
    EXPECT_TRUE((rgv2[2] == std::vector<int>{5}));
    EXPECT_TRUE((rgv2[3] == std::vector<int>{7}));
    EXPECT_TRUE((rgv2[4] == std::vector<int>{9}));
    EXPECT_TRUE((rgv2[5] == std::vector<int>{11}));
    EXPECT_TRUE((rgv2[6] == std::vector<int>{13}));
    EXPECT_TRUE((rgv2[7] == std::vector<int>{15}));
    EXPECT_TRUE((rgv2[8] == std::vector<int>{17}));
    EXPECT_TRUE((rgv2[9] == std::vector<int>{19}));
}

TEST(ActionSplitTest, SplitStringByCStr) {
    std::string s{"This is his face"};
    std::vector<std::string> rgs = actions::split(s, views::c_str(" "));
    EXPECT_EQ(rgs.size(), 4u);
    EXPECT_EQ(rgs[0], "This");
    EXPECT_EQ(rgs[1], "is");
    EXPECT_EQ(rgs[2], "his");
    EXPECT_EQ(rgs[3], "face");
}

TEST(ActionSplitTest, SplitStringByCStrMove) {
    std::string s{"This is his face"};
    std::vector<std::string> rgs = std::move(s) | actions::split(views::c_str(" "));
    EXPECT_EQ(rgs.size(), 4u);
    EXPECT_EQ(rgs[0], "This");
    EXPECT_EQ(rgs[1], "is");
    EXPECT_EQ(rgs[2], "his");
    EXPECT_EQ(rgs[3], "face");
}

TEST(ActionSplitTest, SplitStringByCharArray) {
    std::string s{"This is his face"};
    char ch[] = {' '};
    std::vector<std::string> rgs = actions::split(s, ch);
    EXPECT_EQ(rgs.size(), 4u);
    EXPECT_EQ(rgs[0], "This");
    EXPECT_EQ(rgs[1], "is");
    EXPECT_EQ(rgs[2], "his");
    EXPECT_EQ(rgs[3], "face");
}

TEST(ActionSplitTest, SplitStringByCharArrayMove) {
    std::string s{"This is his face"};
    char ch[] = {' '};
    std::vector<std::string> rgs = std::move(s) | actions::split(ch);
    EXPECT_EQ(rgs.size(), 4u);
    EXPECT_EQ(rgs[0], "This");
    EXPECT_EQ(rgs[1], "is");
    EXPECT_EQ(rgs[2], "his");
    EXPECT_EQ(rgs[3], "face");
}

TEST(ActionSplitTest, SplitViewByValue) {
    auto rgi = views::ints(1, 21);
    std::vector<std::vector<int>> rgv3 = actions::split(rgi, 10);
    EXPECT_EQ(rgv3.size(), 2u);
    EXPECT_TRUE((rgv3[0] == std::vector<int>{1,2,3,4,5,6,7,8,9}));
    EXPECT_TRUE((rgv3[1] == std::vector<int>{11,12,13,14,15,16,17,18,19,20}));
}

TEST(ActionSplitTest, SplitViewByValueMove) {
    auto rgi = views::ints(1, 21);
    std::vector<std::vector<int>> rgv3 = std::move(rgi) | actions::split(10);
    EXPECT_EQ(rgv3.size(), 2u);
    EXPECT_TRUE((rgv3[0] == std::vector<int>{1,2,3,4,5,6,7,8,9}));
    EXPECT_TRUE((rgv3[1] == std::vector<int>{11,12,13,14,15,16,17,18,19,20}));
}

TEST(ActionSplitTest, SplitWhenStringBySpace) {
    std::string str("now  is \t the\ttime");
    auto toks = actions::split_when(str, +[](int i) { return std::isspace(i); });
    static_assert(std::is_same_v<decltype(toks), std::vector<std::string>>);
    EXPECT_EQ(toks.size(), 4u);
    if (toks.size() == 4u) {
        EXPECT_EQ(toks[0], "now");
        EXPECT_EQ(toks[1], "is");
        EXPECT_EQ(toks[2], "the");
        EXPECT_EQ(toks[3], "time");
    }
}

TEST(ActionSplitTest, SplitWhenStringBySpaceMove) {
    std::string str("now  is \t the\ttime");
    auto toks = std::move(str) | actions::split_when(+[](int i) { return std::isspace(i); });
    static_assert(std::is_same_v<decltype(toks), std::vector<std::string>>);
    EXPECT_EQ(toks.size(), 4u);
    if (toks.size() == 4u) {
        EXPECT_EQ(toks[0], "now");
        EXPECT_EQ(toks[1], "is");
        EXPECT_EQ(toks[2], "the");
        EXPECT_EQ(toks[3], "time");
    }
}