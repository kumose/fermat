#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <fermat/action/remove.h>

using namespace ranges;

struct Data {
    int i;
    Data() = default;
    explicit Data(int j) : i(j) {}
    bool operator==(const Data& other) const { return other.i == i; }
    bool operator!=(const Data& other) const { return other.i != i; }
};

TEST(ActionRemoveTest, Simple) {
    std::vector<Data> list;
    list.emplace_back(1);
    list.emplace_back(2);
    list.emplace_back(3);
    list.emplace_back(4);

    Data d2{2};
    const auto remove_data = actions::remove(d2);
    list |= remove_data;
    EXPECT_TRUE((list == std::vector<Data>{Data{1}, Data{3}, Data{4}}));

    list |= actions::remove(3, &Data::i);
    EXPECT_TRUE((list == std::vector<Data>{Data{1}, Data{4}}));
}

TEST(ActionRemoveTest, String) {
    std::vector<std::string> list = {"aaa", "bbb", "ccc"};
    list |= actions::remove("bbb");
    EXPECT_TRUE((list == std::vector<std::string>{"aaa", "ccc"}));
}