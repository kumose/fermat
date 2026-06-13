// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <fermat/container/string.h>
#include <gtest/gtest.h>
#include <string_view>
#include <gtest/gtest.h>
#include <string>
#include <list>
#include <sstream>
#include <stdexcept>
#include  <turbo/log/logging.h>
#include <atomic>
#include <cstdlib>
#include <iomanip>
#include <list>
#include <random>
#include <sstream>

using namespace std;
using namespace fermat;

#if FOLLY_CPLUSPLUS >= 202002
template<typename A, typename B>
using detect_3w = decltype(FOLLY_DECLVAL(A) <= > FOLLY_DECLVAL(B));
#endif

static_assert(!std::is_constructible_v<KString, nullptr_t>);
static_assert(!std::is_assignable_v<KString, nullptr_t>);


#if FOLLY_CPLUSPLUS >= 202002
static_assert(!is_detected_v<detect_3w, KString, nullptr_t>);
static_assert(!is_detected_v<detect_3w, nullptr_t, KString>);
#endif

namespace {
    static const int seed = 100;
    using RandomT = std::mt19937;
    static RandomT rng(seed);
    static const size_t maxString = 100;
    static const bool avoidAliasing = true;

    template<class Integral1, class Integral2>
    Integral2 random(Integral1 low, Integral2 up) {
        std::uniform_int_distribution<> range(low, up);
        return range(rng);
    }

    template<class String>
    void randomString(String *toFill, uint32_t maxSize = 1000) {
        assert(toFill);
        auto r = random(0, maxSize);
        toFill->resize(r);

        for (auto &c: *toFill) {
            c = random('a', 'z');
        }
    }

    template<class String, class Integral>
    void Num2String(String &str, Integral n) {
        std::string tmp = turbo::str_cat(n);
        str = String(tmp.begin(), tmp.end());
    }

    std::list<char> RandomList(uint32_t maxSize) {
        std::list<char> lst(random(0u, maxSize));
        std::list<char>::iterator i = lst.begin();
        for (; i != lst.end(); ++i) {
            *i = random('a', 'z');
        }
        return lst;
    }
} // namespace

////////////////////////////////////////////////////////////////////////////////
// Tests begin here
////////////////////////////////////////////////////////////////////////////////

template<class String>
void clause11_21_4_2_a(String &test) {
    test.String::~String();
    new(&test) String();
}

template<class String>
void clause11_21_4_2_b(String &test) {
    String test2(test);
    assert(test2 == test);
}

template<class String>
void clause11_21_4_2_c(String &test) {
    // Test move constructor. There is a more specialized test, see
    // testMoveCtor test
    String donor(test);
    String test2(std::move(donor));
    EXPECT_EQ(test2, test);
    // Technically not required, but all implementations that actually
    // support move will move large strings. Make a guess for 128 as the
    // maximum small string optimization that's reasonable.
    EXPECT_LE(donor.size(), 128);
}

template<class String>
void clause11_21_4_2_d(String &test) {
    // Copy constructor with position and length
    const size_t pos = random(0, test.size());
    String s(
        test,
        pos,
        random(0, 9)
            ? random(0, (size_t) (test.size() - pos))
            : String::npos); // test for npos, too, in 10% of the cases
    test = s;
}

template<class String>
void clause11_21_4_2_e(String &test) {
    // Constructor from char*, size_t
    const size_t pos = random(0, test.size()), n = random(0, test.size() - pos);
    String before(test.data(), test.size());

    String s(test.c_str() + pos, n);

    String after(test.data(), test.size());
    EXPECT_EQ(before, after);
    test.swap(s);
}

template<class String>
void clause11_21_4_2_f(String &test) {
    // Constructor from char*
    const size_t pos = random(0, test.size());
    String before(test.data(), test.size());
    String s(test.c_str() + pos);
    String after(test.data(), test.size());
    EXPECT_EQ(before, after);
    test.swap(s);
}

template<class String>
void clause11_21_4_2_g(String &test) {
    // Constructor from size_t, char
    const size_t n = random(0, test.size());
    const auto c = test.front();
    test = String(n, c);
}

template<class String>
void clause11_21_4_2_h(String &test) {
    // Constructors from various iterator pairs
    // Constructor from char*, char*
    String s1(test.begin(), test.end());
    EXPECT_EQ(test, s1);
    String s2(test.data(), test.data() + test.size());
    EXPECT_EQ(test, s2);
    // Constructor from other iterators
    std::list<char> lst;
    for (auto c: test) {
        lst.push_back(c);
    }
    String s3(lst.begin(), lst.end());
    EXPECT_EQ(test, s3);
    // Constructor from wchar_t iterators
    std::list<wchar_t> lst1;
    for (auto c: test) {
        lst1.push_back(c);
    }
    String s4(lst1.begin(), lst1.end());
    EXPECT_EQ(test, s4);
    // Constructor from wchar_t pointers
    wchar_t t[20];
    t[0] = 'a';
    t[1] = 'b';
    KString s5(t, t + 2);
    EXPECT_EQ("ab", s5);
}

template<class String>
void clause11_21_4_2_i(String &test) {
    // From initializer_list<char>
    std::initializer_list<typename String::value_type> il = {
        'h', 'e', 'l', 'l', 'o'
    };
    String s(il);
    test.swap(s);
}

template<class String>
void clause11_21_4_2_j(String &test) {
    // Assignment from const String&
    auto size = random(0, 2000);
    String s(size, '\0');
    EXPECT_EQ(s.size(), size);
    for (auto i = 0; i < s.size(); i++) {
        s[i] = random('a', 'z');
    }
    test = s;
}

template<class String>
void clause11_21_4_2_k(String &test) {
    // Assignment from String&&
    auto size = random(0, 2000);
    String s(size, '\0');
    EXPECT_EQ(s.size(), size);
    for (auto i = 0; i < s.size(); i++) {
        s[i] = random('a', 'z');
    }
    test = std::move(s);
    if (std::is_same<String, KString>::value) {
        EXPECT_LE(s.size(), 128);
    }
}

template<class String>
void clause11_21_4_2_l(String &test) {
    // Assignment from char*
    String s(random(0, 1000), '\0');
    size_t i = 0;
    for (; i != s.size(); ++i) {
        s[i] = random('a', 'z');
    }
    test = s.c_str();
}

template<class String>
void clause11_21_4_2_lprime(String &test) {
    // Aliased assign
    const size_t pos = random(0, test.size());
    if (avoidAliasing) {
        test = String(test.c_str() + pos);
    } else {
        test = test.c_str() + pos;
    }
}

template<class String>
void clause11_21_4_2_m(String &test) {
    // Assignment from char
    using value_type = typename String::value_type;
    test = random(static_cast<value_type>('a'), static_cast<value_type>('z'));
}

template<class String>
void clause11_21_4_2_n(String &test) {
    // Assignment from initializer_list<char>
    initializer_list<typename String::value_type> il = {'h', 'e', 'l', 'l', 'o'};
    test = il;
}

template<class String>
void clause11_21_4_3(String &test) {
    // Iterators. The code below should leave test unchanged
    EXPECT_EQ(test.size(), test.end() - test.begin());
    EXPECT_EQ(test.size(), test.rend() - test.rbegin());
    EXPECT_EQ(test.size(), test.cend() - test.cbegin());
    EXPECT_EQ(test.size(), test.crend() - test.crbegin());

    auto s = test.size();
    test.resize(test.end() - test.begin());
    EXPECT_EQ(s, test.size());
    test.resize(test.rend() - test.rbegin());
    EXPECT_EQ(s, test.size());
}

template<class String>
void clause11_21_4_4(String &test) {
    // exercise capacity, size, max_size
    EXPECT_EQ(test.size(), test.length());
    EXPECT_LE(test.size(), test.max_size());
    EXPECT_LE(test.capacity(), test.max_size());
    EXPECT_LE(test.size(), test.capacity());

    // exercise shrink_to_fit. Nonbinding request so we can't really do
    // much beyond calling it.
    auto copy = test;
    copy.reserve(copy.capacity() * 3);
    copy.shrink_to_fit();
    EXPECT_EQ(copy, test);

    // exercise empty
    string empty("empty");
    string notempty("not empty");
    if (test.empty()) {
        test = String(empty.begin(), empty.end());
    } else {
        test = String(notempty.begin(), notempty.end());
    }
}

template<class String>
void clause11_21_4_5(String &test) {
    // exercise element access
    if (!test.empty()) {
        EXPECT_EQ(test[0], test.front());
        EXPECT_EQ(test[test.size() - 1], test.back());
        auto const i = random(0, test.size() - 1);
        EXPECT_EQ(test[i], test.at(i));
        test = test[i];
    }

    EXPECT_THROW(void(test.at(test.size())), std::out_of_range);
    EXPECT_THROW(void(std::as_const(test).at(test.size())), std::out_of_range);
}

template<class String>
void clause11_21_4_6_1(String &test) {
    using string_view_type = std::basic_string_view<
        typename String::value_type,
        typename String::traits_type>;
    auto orig = test;

    // 21.3.5 modifiers (+=)
    String test1;
    randomString(&test1);
    assert(
        test1.size() ==
        char_traits<typename String::value_type>::length(test1.c_str()));
    auto len = test.size();
    test += test1;
    EXPECT_EQ(test.size(), test1.size() + len);
    for (auto i = 0; i < test1.size(); i++) {
        EXPECT_EQ(test[len + i], test1[i]);
    }
    // aliasing modifiers
    String test2 = test;
    auto dt = test2.data();
    auto sz = test.c_str();
    len = test.size();
    KLOG(INFO)<<"1111111111111111";
    EXPECT_EQ(memcmp(sz, dt, len), 0);
    String copy(test.data(), test.size());
    EXPECT_EQ(
        char_traits<typename String::value_type>::length(test.c_str()), len);
    test += test;
    // test.append(test);
    EXPECT_EQ(test.size(), 2 * len);
    EXPECT_EQ(
        char_traits<typename String::value_type>::length(test.c_str()), 2 * len);
    for (auto i = 0; i < len; i++) {
        EXPECT_EQ(test[i], copy[i]);
        EXPECT_EQ(test[i], test[len + i]);
    }

    KLOG(INFO)<<"22222222222222";
    len = test.size();
    EXPECT_EQ(
        char_traits<typename String::value_type>::length(test.c_str()), len);
    // more aliasing
    auto const pos = random(0, test.size());
    EXPECT_EQ(
        char_traits<typename String::value_type>::length(test.c_str() + pos),
        len - pos);
    if (avoidAliasing) {
        String addMe(test.c_str() + pos);
        EXPECT_EQ(addMe.size(), len - pos);
        test += addMe;
    } else {
        test += test.c_str() + pos;
    }
    EXPECT_EQ(test.size(), 2 * len - pos);
    KLOG(INFO)<<"333333333333333";

    // single char
    len = test.size();
    test += random('a', 'z');
    EXPECT_EQ(test.size(), len + 1);
    // initializer_list
    initializer_list<typename String::value_type> il{'a', 'b', 'c'};
    test += il;
    // string_view
    auto testsv = orig;
    testsv += string_view_type{orig};
    EXPECT_EQ(orig + orig, testsv);
    KLOG(INFO)<<"4444444444444444444";

    // like string_view
    auto testlsv = orig;
    testlsv += std::invoke([&] { return string_view_type{orig}; });
    EXPECT_EQ(orig + orig, testlsv);
    KLOG(INFO)<<"55555555555555555";

}


template<class String>
void clause11_21_4_6_2(String &test) {
    using string_view_type = std::basic_string_view<
        typename String::value_type,
        typename String::traits_type>;
    auto orig = test;

    // 21.3.5 modifiers (append, push_back)
    String s;

    // Test with a small string first
    char c = random('a', 'z');
    s.push_back(c);
    EXPECT_EQ(s[s.size() - 1], c);
    EXPECT_EQ(s.size(), 1);
    s.resize(s.size() - 1);

    randomString(&s, maxString);
    test.append(s);
    randomString(&s, maxString);
    test.append(s, random(0, s.size()), random(0, maxString));
    randomString(&s, maxString);
    test.append(s.c_str(), random(0, s.size()));
    randomString(&s, maxString);
    test.append(s.c_str());
    test.append(static_cast<size_t>(random(0, maxString)), random('a', 'z'));
    std::list<char> lst(RandomList(maxString));
    test.append(lst.begin(), lst.end());
    c = random('a', 'z');
    test.push_back(c);
    EXPECT_EQ(test[test.size() - 1], c);
    // initializer_list
    initializer_list<typename String::value_type> il{'a', 'b', 'c'};
    test.append(il);
    // string_view
    auto testsv = orig;
    testsv.append(string_view_type{orig});
    EXPECT_EQ(orig + orig, testsv);
    // like string_view
    auto testlsv = orig;
    testlsv.append(std::invoke([&] { return string_view_type{orig}; }));
    EXPECT_EQ(orig + orig, testlsv);
}

template<class String>
void clause11_21_4_6_3_a(String &test) {
    /// assign
    String s;
    randomString(&s);
    test.assign(s);
    EXPECT_EQ(test, s);
    // move assign
    test.assign(std::move(s));
    if (std::is_same<String, KString>::value) {
        EXPECT_LE(s.size(), 128);
    }
}

template<class String>
void clause11_21_4_6_3_b(String &test) {
    /// assign
    String s;
    randomString(&s, maxString);
    test.assign(s, random(0, s.size()), random(0, maxString));
}

template<class String>
void clause11_21_4_6_3_c(String &test) {
    /// assign
    String s;
    randomString(&s, maxString);
    test.assign(s.c_str(), random(0, s.size()));
}

template<class String>
void clause11_21_4_6_3_d(String &test) {
    /// assign
    String s;
    randomString(&s, maxString);
    test.assign(s.c_str());
}

template<class String>
void clause11_21_4_6_3_e(String &test) {
    /// assign
    String s;
    randomString(&s, maxString);
    test.assign(random(0, maxString), random('a', 'z'));
}

template<class String>
void clause11_21_4_6_3_f(String &test) {
    /// assign from bidirectional iterator
    std::list<char> lst(RandomList(maxString));
    test.assign(lst.begin(), lst.end());
}

template<class String>
void clause11_21_4_6_3_g(String &test) {
    /// assign from aliased source
    test.assign(test);
}

template<class String>
void clause11_21_4_6_3_h(String &test) {
    /// assign from aliased source

    test.assign(test, random(0, test.size()), random(0, maxString));
}

template<class String>
void clause11_21_4_6_3_i(String &test) {
    /// assign from aliased source
    test.assign(test.c_str(), random(0, test.size()));
}

template<class String>
void clause11_21_4_6_3_j(String &test) {
    /// assign from aliased source
    test.assign(test.c_str());
}

template<class String>
void clause11_21_4_6_3_k(String &test) {
    /// assign from initializer_list
    initializer_list<typename String::value_type> il{'a', 'b', 'c'};
    test.assign(il);
}

template<class String>
void clause11_21_4_6_4(String &test) {
    /// insert
    String s;
    randomString(&s, maxString);
    test.insert(random(0, test.size()), s);
    randomString(&s, maxString);
    test.insert(
        random(0, test.size()), s, random(0, s.size()), random(0, maxString));
    randomString(&s, maxString);
    test.insert(random(0, test.size()), s.c_str(), random(0, s.size()));
    randomString(&s, maxString);
    test.insert(random(0, test.size()), s.c_str());
    test.insert(random(0, test.size()), random(0, maxString), random('a', 'z'));
    typename String::size_type pos = random(0, test.size());
    typename String::iterator res =
            test.insert(test.begin() + pos, random('a', 'z'));
    EXPECT_EQ(res - test.begin(), pos);
    std::list<char> lst(RandomList(maxString));
    pos = random(0, test.size());
    // Uncomment below to see a bug in gcc

    test.insert(test.begin() + pos, lst.begin(), lst.end());
    /// insert from initializer_list
    initializer_list<typename String::value_type> il{'a', 'b', 'c'};
    pos = random(0, test.size());
    // Uncomment below to see a bug in gcc

    test.insert(test.begin() + pos, il);

    // Test with actual input iterators
    stringstream ss;
    ss << "hello cruel world";
    auto i = istream_iterator<char>(ss);
    test.insert(test.begin(), i, istream_iterator<char>());
}

template<class String>
void clause11_21_4_6_5(String &test) {
    // erase and pop_back
    if (!test.empty()) {
        test.erase(random(0, test.size()), random(0, maxString));
    }
    if (!test.empty()) {
        // TODO: is erase(end()) allowed?
        test.erase(test.begin() + random(0, test.size() - 1));
    }
    if (!test.empty()) {
        auto const i = test.begin() + random(0, test.size());
        if (i != test.end()) {
            test.erase(i, i + random(0, size_t(test.end() - i)));
        }
    }
    if (!test.empty()) {
        // Can't test pop_back with std::string, doesn't support it yet.
        // test.pop_back();
    }
}

template<class String>
void clause11_21_4_6_6(String &test) {
    auto pos = random(0, test.size());
    if (avoidAliasing) {
        test.replace(pos, random(0, test.size() - pos), String(test));
    } else {
        test.replace(pos, random(0, test.size() - pos), test);
    }
    pos = random(0, test.size());
    String s;
    randomString(&s, maxString);
    test.replace(pos, pos + random(0, test.size() - pos), s);
    auto pos1 = random(0, test.size());
    auto pos2 = random(0, test.size());
    if (avoidAliasing) {
        test.replace(
            pos1,
            pos1 + random(0, test.size() - pos1),
            String(test),
            pos2,
            pos2 + random(0, test.size() - pos2));
    } else {
        test.replace(
            pos1,
            pos1 + random(0, test.size() - pos1),
            test,
            pos2,
            pos2 + random(0, test.size() - pos2));
    }
    pos1 = random(0, test.size());
    String str;
    randomString(&str, maxString);
    pos2 = random(0, str.size());
    test.replace(
        pos1,
        pos1 + random(0, test.size() - pos1),
        str,
        pos2,
        pos2 + random(0, str.size() - pos2));
    pos = random(0, test.size());
    if (avoidAliasing) {
        test.replace(
            pos, random(0, test.size() - pos), String(test).c_str(), test.size());
    } else {
        test.replace(pos, random(0, test.size() - pos), test.c_str(), test.size());
    }
    pos = random(0, test.size());
    randomString(&str, maxString);
    test.replace(
        pos, pos + random(0, test.size() - pos), str.c_str(), str.size());
    pos = random(0, test.size());
    randomString(&str, maxString);
    test.replace(pos, pos + random(0, test.size() - pos), str.c_str());
    pos = random(0, test.size());
    test.replace(
        pos,
        random(0, test.size() - pos),
        random(0, maxString),
        random('a', 'z'));
    pos = random(0, test.size());
    if (avoidAliasing) {
        auto newString = String(test);
        test.replace(
            test.begin() + pos,
            test.begin() + pos + random(0, test.size() - pos),
            newString);
    } else {
        test.replace(
            test.begin() + pos,
            test.begin() + pos + random(0, test.size() - pos),
            test);
    }
    pos = random(0, test.size());
    if (avoidAliasing) {
        auto newString = String(test);
        test.replace(
            test.begin() + pos,
            test.begin() + pos + random(0, test.size() - pos),
            newString.c_str(),
            test.size() - random(0, test.size()));
    } else {
        test.replace(
            test.begin() + pos,
            test.begin() + pos + random(0, test.size() - pos),
            test.c_str(),
            test.size() - random(0, test.size()));
    }
    pos = random(0, test.size());
    auto const n = random(0, test.size() - pos);
    typename String::iterator b = test.begin();
    String str1;
    randomString(&str1, maxString);
    const String &str3 = str1;
    const typename String::value_type *ss = str3.c_str();
    test.replace(b + pos, b + pos + n, ss);
    pos = random(0, test.size());
    test.replace(
        test.begin() + pos,
        test.begin() + pos + random(0, test.size() - pos),
        random(0, maxString),
        random('a', 'z'));
}

template<class String>
void clause11_21_4_6_7(String &test) {
    std::vector<typename String::value_type> vec(random(0, maxString));
    if (vec.empty()) {
        return;
    }
    test.copy(vec.data(), vec.size(), random(0, test.size()));
}

template<class String>
void clause11_21_4_6_8(String &test) {
    String s;
    randomString(&s, maxString);
    s.swap(test);
}

/*
template<class String>
void clause11_21_4_7_1(String &test) {
    // 21.3.6 string operations
    // exercise c_str() and data()
    assert(test.c_str() == test.data());
    // exercise get_allocator()
    String s;
    randomString(&s, maxString);
    DKCHECK(test.get_allocator() == s.get_allocator());
}
*/
template<class String>
void clause11_21_4_7_2_a(String &test) {
    String str = test.substr(random(0, test.size()), random(0, test.size()));
    Num2String(test, test.find(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_2_a1(String &test) {
    String str =
            String(test).substr(random(0, test.size()), random(0, test.size()));
    Num2String(test, test.find(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_2_a2(String &test) {
    auto const &cTest = test;
    String str = cTest.substr(random(0, test.size()), random(0, test.size()));
    Num2String(test, test.find(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_2_b(String &test) {
    auto from = random(0, test.size());
    auto length = random(0, test.size() - from);
    String str = test.substr(from, length);
    Num2String(
        test,
        test.find(str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_2_b1(String &test) {
    auto from = random(0, test.size());
    auto length = random(0, test.size() - from);
    String str = String(test).substr(from, length);
    Num2String(
        test,
        test.find(str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_2_b2(String &test) {
    auto from = random(0, test.size());
    auto length = random(0, test.size() - from);
    const auto &cTest = test;
    String str = cTest.substr(from, length);
    Num2String(
        test,
        test.find(str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_2_c(String &test) {
    String str = test.substr(random(0, test.size()), random(0, test.size()));
    Num2String(test, test.find(str.c_str(), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_2_c1(String &test) {
    KLOG(INFO) << "before:" << test;
    String str =
            String(test).substr(random(0, test.size()), random(0, test.size()));
    auto n = random(0, test.size());
    auto f = test.find(str.c_str(), n);
    KLOG(INFO) << "medium:" << str << " medium size:" << str.size() << " c_str:" << str.c_str() << " n:" << n << " f:"
 << f;
    Num2String(test, f);
    KLOG(INFO) << "after:" << test;
}

template<class String>
void clause11_21_4_7_2_c2(String &test) {
    const auto &cTest = test;
    String str = cTest.substr(random(0, test.size()), random(0, test.size()));
    Num2String(test, test.find(str.c_str(), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_2_d(String &test) {
    Num2String(test, test.find(random('a', 'z'), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_3_a(String &test) {
    String str = test.substr(random(0, test.size()), random(0, test.size()));
    Num2String(test, test.rfind(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_3_b(String &test) {
    String str = test.substr(random(0, test.size()), random(0, test.size()));
    Num2String(
        test,
        test.rfind(str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_3_c(String &test) {
    String str = test.substr(random(0, test.size()), random(0, test.size()));
    Num2String(test, test.rfind(str.c_str(), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_3_d(String &test) {
    Num2String(test, test.rfind(random('a', 'z'), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_4_a(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_first_of(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_4_b(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(
        test,
        test.find_first_of(
            str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_4_c(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_first_of(str.c_str(), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_4_d(String &test) {
    Num2String(
        test, test.find_first_of(random('a', 'z'), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_5_a(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_last_of(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_5_b(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(
        test,
        test.find_last_of(
            str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_5_c(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_last_of(str.c_str(), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_5_d(String &test) {
    Num2String(test, test.find_last_of(random('a', 'z'), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_6_a(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_first_not_of(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_6_b(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(
        test,
        test.find_first_not_of(
            str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_6_c(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_first_not_of(str.c_str(), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_6_d(String &test) {
    Num2String(
        test, test.find_first_not_of(random('a', 'z'), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_7_a(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_last_not_of(str, random(0, test.size())));
}

template<class String>
void clause11_21_4_7_7_b(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(
        test,
        test.find_last_not_of(
            str.c_str(), random(0, test.size()), random(0, str.size())));
}

template<class String>
void clause11_21_4_7_7_c(String &test) {
    String str;
    randomString(&str, maxString);
    Num2String(test, test.find_last_not_of(str.c_str(), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_7_d(String &test) {
    Num2String(
        test, test.find_last_not_of(random('a', 'z'), random(0, test.size())));
}

template<class String>
void clause11_21_4_7_8(String &test) {
    test = test.substr(random(0, test.size()), random(0, test.size()));
}

template<class String>
void clause11_21_4_7_9_a(String &test) {
    String s;
    randomString(&s, maxString);
    int tristate = test.compare(s);
    if (tristate > 0) {
        tristate = 1;
    } else if (tristate < 0) {
        tristate = 2;
    }
    Num2String(test, tristate);
}

template<class String>
void clause11_21_4_7_9_b(String &test) {
    String s;
    randomString(&s, maxString);
    int tristate =
            test.compare(random(0, test.size()), random(0, test.size()), s);
    if (tristate > 0) {
        tristate = 1;
    } else if (tristate < 0) {
        tristate = 2;
    }
    Num2String(test, tristate);
}

template<class String>
void clause11_21_4_7_9_c(String &test) {
    String str;
    randomString(&str, maxString);
    int tristate = test.compare(
        random(0, test.size()),
        random(0, test.size()),
        str,
        random(0, str.size()),
        random(0, str.size()));
    if (tristate > 0) {
        tristate = 1;
    } else if (tristate < 0) {
        tristate = 2;
    }
    Num2String(test, tristate);
}

template<class String>
void clause11_21_4_7_9_d(String &test) {
    String s;
    randomString(&s, maxString);
    int tristate = test.compare(s.c_str());
    if (tristate > 0) {
        tristate = 1;
    } else if (tristate < 0) {
        tristate = 2;
    }
    Num2String(test, tristate);
}

template<class String>
void clause11_21_4_7_9_e(String &test) {
    String str;
    randomString(&str, maxString);
    int tristate = test.compare(
        random(0, test.size()),
        random(0, test.size()),
        str.c_str(),
        random(0, str.size()));
    if (tristate > 0) {
        tristate = 1;
    } else if (tristate < 0) {
        tristate = 2;
    }
    Num2String(test, tristate);
}

template<class String>
void clause11_21_4_8_1_a(String &test) {
    String s1;
    randomString(&s1, maxString);
    String s2;
    randomString(&s2, maxString);
    test = s1 + s2;
}

template<class String>
void clause11_21_4_8_1_b(String &test) {
    String s1;
    randomString(&s1, maxString);
    String s2;
    randomString(&s2, maxString);
    test = std::move(s1) + s2;
}

template<class String>
void clause11_21_4_8_1_c(String &test) {
    String s1;
    randomString(&s1, maxString);
    String s2;
    randomString(&s2, maxString);
    test = s1 + std::move(s2);
}

template<class String>
void clause11_21_4_8_1_d(String &test) {
    String s1;
    randomString(&s1, maxString);
    String s2;
    randomString(&s2, maxString);
    test = std::move(s1) + std::move(s2);
}

template<class String>
void clause11_21_4_8_1_e(String &test) {
    String s;
    randomString(&s, maxString);
    String s1;
    randomString(&s1, maxString);
    test = s.c_str() + s1;
}

template<class String>
void clause11_21_4_8_1_f(String &test) {
    String s;
    randomString(&s, maxString);
    String s1;
    randomString(&s1, maxString);
    test = s.c_str() + std::move(s1);
}

template<class String>
void clause11_21_4_8_1_g(String &test) {
    String s;
    randomString(&s, maxString);
    test = typename String::value_type(random('a', 'z')) + s;
}

template<class String>
void clause11_21_4_8_1_h(String &test) {
    String s;
    randomString(&s, maxString);
    test = typename String::value_type(random('a', 'z')) + std::move(s);
}

template<class String>
void clause11_21_4_8_1_i(String &test) {
    String s;
    randomString(&s, maxString);
    String s1;
    randomString(&s1, maxString);
    test = s + s1.c_str();
}

template<class String>
void clause11_21_4_8_1_j(String &test) {
    String s;
    randomString(&s, maxString);
    String s1;
    randomString(&s1, maxString);
    test = std::move(s) + s1.c_str();
}

template<class String>
void clause11_21_4_8_1_k(String &test) {
    String s;
    randomString(&s, maxString);
    test = s + typename String::value_type(random('a', 'z'));
}

template<class String>
void clause11_21_4_8_1_l(String &test) {
    String s;
    randomString(&s, maxString);
    String s1;
    randomString(&s1, maxString);
    test = std::move(s) + s1.c_str();
}

// Numbering here is from C++11
template<class String>
void clause11_21_4_8_9_a(String &test) {
    basic_stringstream<typename String::value_type> stst(test.c_str());
    String str;
    while (stst) {
        stst >> str;
        test += str + test;
    }
}

TEST(KString, testAllClauses) {
    EXPECT_TRUE(1) << "Starting with seed: " << seed;
    std::string r;
    fermat::KString c;
    int count = 0;

    auto l = [&](const char *const clause,
                 void (*f_string)(std::string &),
                 void (*f_fbstring)(fermat::KString &)) {
        do {
            if (true) {
            } else {
                EXPECT_TRUE(1) << "Testing clause " << clause;
            }
            randomString(&r);
            c = r;
            EXPECT_EQ(c.size(), r.size());
            EXPECT_EQ(c, r);
            auto localSeed = seed + count;
            rng = RandomT(localSeed);
            f_string(r);
            rng = RandomT(localSeed);
            f_fbstring(c);
            EXPECT_EQ(r, c)
          << "Lengths: " << r.size() << " vs. " << c.size() << "\nReference: '"
          << r << "'" << "\nActual:    '" << c.data()[0] << "' loop:" << count;
        } while (++count % 100 != 0);
    };

#define TEST_CLAUSE(x)             \
  KLOG(INFO)<<#x;                  \
  l(#x,                            \
    clause11_##x<std::string>,     \
    clause11_##x<fermat::KString>);

    TEST_CLAUSE(21_4_2_a);
    TEST_CLAUSE(21_4_2_b);
    TEST_CLAUSE(21_4_2_c);
    TEST_CLAUSE(21_4_2_d);
    TEST_CLAUSE(21_4_2_e);
    TEST_CLAUSE(21_4_2_f);
    TEST_CLAUSE(21_4_2_g);
    TEST_CLAUSE(21_4_2_h);
    TEST_CLAUSE(21_4_2_i);
    TEST_CLAUSE(21_4_2_j);
    TEST_CLAUSE(21_4_2_k);
    TEST_CLAUSE(21_4_2_l);
    TEST_CLAUSE(21_4_2_lprime);
    TEST_CLAUSE(21_4_2_m);
    TEST_CLAUSE(21_4_2_n);
    TEST_CLAUSE(21_4_3);
    TEST_CLAUSE(21_4_4);
    TEST_CLAUSE(21_4_5);
    TEST_CLAUSE(21_4_6_1);
    TEST_CLAUSE(21_4_6_2);

    TEST_CLAUSE(21_4_6_3_a);
    TEST_CLAUSE(21_4_6_3_b);
    TEST_CLAUSE(21_4_6_3_c);
    TEST_CLAUSE(21_4_6_3_d);
    TEST_CLAUSE(21_4_6_3_e);
    TEST_CLAUSE(21_4_6_3_f);
    TEST_CLAUSE(21_4_6_3_g);
    TEST_CLAUSE(21_4_6_3_h);
    TEST_CLAUSE(21_4_6_3_i);
    TEST_CLAUSE(21_4_6_3_j);
    TEST_CLAUSE(21_4_6_3_k);
    TEST_CLAUSE(21_4_6_4);
    TEST_CLAUSE(21_4_6_5);
    TEST_CLAUSE(21_4_6_6);
    TEST_CLAUSE(21_4_6_7);
    TEST_CLAUSE(21_4_6_8);
    //TEST_CLAUSE(21_4_7_1);

    TEST_CLAUSE(21_4_7_2_a);
    TEST_CLAUSE(21_4_7_2_a1);
    TEST_CLAUSE(21_4_7_2_a2);
    TEST_CLAUSE(21_4_7_2_b);
    TEST_CLAUSE(21_4_7_2_b1);
    TEST_CLAUSE(21_4_7_2_b2);
    TEST_CLAUSE(21_4_7_2_c);
    TEST_CLAUSE(21_4_7_2_c1);
    TEST_CLAUSE(21_4_7_2_c2);
    TEST_CLAUSE(21_4_7_2_d);
    TEST_CLAUSE(21_4_7_3_a);
    TEST_CLAUSE(21_4_7_3_b);
    TEST_CLAUSE(21_4_7_3_c);
    TEST_CLAUSE(21_4_7_3_d);
    TEST_CLAUSE(21_4_7_4_a);
    TEST_CLAUSE(21_4_7_4_b);
    TEST_CLAUSE(21_4_7_4_c);
    TEST_CLAUSE(21_4_7_4_d);
    TEST_CLAUSE(21_4_7_5_a);
    TEST_CLAUSE(21_4_7_5_b);
    TEST_CLAUSE(21_4_7_5_c);
    TEST_CLAUSE(21_4_7_5_d);
    TEST_CLAUSE(21_4_7_6_a);
    TEST_CLAUSE(21_4_7_6_b);
    TEST_CLAUSE(21_4_7_6_c);
    TEST_CLAUSE(21_4_7_6_d);
    TEST_CLAUSE(21_4_7_7_a);
    TEST_CLAUSE(21_4_7_7_b);
    TEST_CLAUSE(21_4_7_7_c);
    TEST_CLAUSE(21_4_7_7_d);
    TEST_CLAUSE(21_4_7_8);
    TEST_CLAUSE(21_4_7_9_a);
    TEST_CLAUSE(21_4_7_9_b);
    TEST_CLAUSE(21_4_7_9_c);
    TEST_CLAUSE(21_4_7_9_d);
    TEST_CLAUSE(21_4_7_9_e);
    TEST_CLAUSE(21_4_8_1_a);
    TEST_CLAUSE(21_4_8_1_b);
    TEST_CLAUSE(21_4_8_1_c);
    TEST_CLAUSE(21_4_8_1_d);
    TEST_CLAUSE(21_4_8_1_e);
    TEST_CLAUSE(21_4_8_1_f);
    TEST_CLAUSE(21_4_8_1_g);
    TEST_CLAUSE(21_4_8_1_h);
    TEST_CLAUSE(21_4_8_1_i);
    TEST_CLAUSE(21_4_8_1_j);
    TEST_CLAUSE(21_4_8_1_k);
    TEST_CLAUSE(21_4_8_1_l);
    TEST_CLAUSE(21_4_8_9_a);
}

TEST(KString, testGetline) {
    string s1 =
            "\
Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras accumsan \n\
elit ut urna consectetur in sagittis mi auctor. Nulla facilisi. In nec \n\
dolor leo, vitae imperdiet neque. Donec ut erat mauris, a faucibus \n\
elit. Integer consectetur gravida augue, sit amet mattis mauris auctor \n\
sed. Morbi congue libero eu nunc sodales adipiscing. In lectus nunc, \n\
vulputate a fringilla at, venenatis quis justo. Proin eu velit \n\
nibh. Maecenas vitae tellus eros. Pellentesque habitant morbi \n\
tristique senectus et netus et malesuada fames ac turpis \n\
egestas. Vivamus faucibus feugiat consequat. Donec fermentum neque sit \n\
amet ligula suscipit porta. Phasellus facilisis felis in purus luctus \n\
quis posuere leo tempor. Nam nunc purus, luctus a pharetra ut, \n\
placerat at dui. Donec imperdiet, diam quis convallis pulvinar, dui \n\
est commodo lorem, ut tincidunt diam nibh et nibh. Maecenas nec velit \n\
massa, ut accumsan magna. Donec imperdiet tempor nisi et \n\
laoreet. Phasellus lectus quam, ultricies ut tincidunt in, dignissim \n\
id eros. Mauris vulputate tortor nec neque pellentesque sagittis quis \n\
sed nisl. In diam lacus, lobortis ut posuere nec, ornare id quam.";

    vector<KString> v = turbo::str_split(s1, turbo::ByAnyChar("\n"));
    {
        istringstream input(s1);
        KString line;
        for (const auto &expected: v) {
            EXPECT_TRUE(!getline(input, line).fail());
            EXPECT_EQ(line, expected);
        }
    }
}

TEST(KString, testMoveCtor) {
    // Move constructor. Make sure we allocate a large string, so the
    // small string optimization doesn't kick in.
    auto size = random(100, 2000);
    KString s(size, 'a');
    EXPECT_EQ(size, s.size());
    KString test = std::move(s);
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size, test.size());
}


TEST(KString, testMoveAssign) {
    // Move constructor. Make sure we allocate a large string, so the
    // small string optimization doesn't kick in.
    auto size = random(100, 2000);
    KString s(size, 'a');
    KString test;
    test = std::move(s);
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(size, test.size());
}

TEST(KString, testMoveOperatorPlusLhs) {
    // Make sure we allocate a large string, so the
    // small string optimization doesn't kick in.
    auto size1 = random(100, 2000);
    auto size2 = random(100, 2000);
    KString s1(size1, 'a');
    KString s2(size2, 'b');
    KString test;
    test = std::move(s1) + s2;
    EXPECT_TRUE(s1.empty());
    EXPECT_EQ(size1 + size2, test.size());
}

TEST(KString, testMoveOperatorPlusRhs) {
    // Make sure we allocate a large string, so the
    // small string optimization doesn't kick in.
    auto size1 = random(100, 2000);
    auto size2 = random(100, 2000);
    KString s1(size1, 'a');
    KString s2(size2, 'b');
    KString test;
    test = s1 + std::move(s2);
    EXPECT_EQ(size1 + size2, test.size());
}

// The GNU C++ standard library throws an std::logic_error when an std::string
// is constructed with a null pointer. Verify that we mirror this behavior.
//
// N.B. We behave this way even if the C++ library being used is something
//      other than libstdc++. Someday if we deem it important to present
//      identical undefined behavior for other platforms, we can re-visit this.
TEST(KString, testConstructionFromLiteralZero) {
    EXPECT_NO_THROW(KString s(static_cast<const char*>(nullptr)));
}

TEST(KString, testFixedBugsD479397) {
    KString str(1337, 'f');
    KString cp = str;
    cp.clear();
    cp.c_str();
    EXPECT_EQ(str.front(), 'f');
}

TEST(KString, assin) {
    KString str(1337, 'f');
    KString cp;
    cp.clear();
    cp.assign(str.begin(), str.end());
    EXPECT_EQ(str, cp);
}

TEST(KString, testFixedBugsD481173) {
    KString str(1337, 'f');
    for (int i = 0; i < 2; ++i) {
        KString cp = str;
        cp[1] = 'b';
        EXPECT_EQ(cp.size(), 1337);
        EXPECT_EQ(cp.c_str()[cp.size()], '\0');
        cp.push_back('?');
    }
}

TEST(KString, testFixedBugsD580267PushBack) {
    KString str(1337, 'f');
    KString cp = str;
    cp.push_back('f');
}

TEST(KString, testFixedBugsD580267OperatorAddAssign) {
    KString str(1337, 'f');
    KString cp = str;
    cp += "bb";
}

TEST(KString, testFixedBugsD661622) {
    fermat::BasicString<wchar_t, 0> s;
    EXPECT_EQ(0, s.size());
}

TEST(KString, testFixedBugsD785057) {
    KString str(1337, 'f');
    std::swap(str, str);
    EXPECT_EQ(1337, str.size());
}

TEST(KString, testFixedBugsD1012196AllocatorMalloc) {
    KString str(128, 'f');
    str.clear(); // Empty medium string.
    KString copy(str); // Medium string of 0 capacity.
    copy.push_back('b');
    EXPECT_GE(copy.capacity(), 1);
}

TEST(KString, testFixedBugsD2813713) {
    KString s1("a");
    s1.reserve(8); // Trigger the optimized code path.
    KLOG(INFO) << "rrr:" << s1.size();
    auto test1 = '\0' + std::move(s1);
    EXPECT_EQ(2, test1.size());
    KLOG(INFO) << "##################" << s1.size();
    KString s2(1, '\0');
    s2.reserve(8);
    auto test2 = "a" + std::move(s2);
    EXPECT_EQ(2, test2.size());
}

TEST(KString, testFixedBugsD3698862) {
    EXPECT_EQ(KString().find(KString(), 4), KString::npos);
}

TEST(KString, testFixedBugsD4355440) {
    KString str(1337, 'f');
    str.reserve(3840);
    EXPECT_NE(str.capacity(), 3840);

    EXPECT_EQ(
        str.capacity(),
        str.get_allocator().good_size(3840) - sizeof(char));
}

TEST(KString, findWithNpos) {
    KString kstr("localhost:80");
    EXPECT_EQ(KString::npos, kstr.find(":", KString::npos));
}

#if FOLLY_HAVE_WCHAR_SUPPORT
TEST(KString, testHashChar16) {
    using u16fbstring = fermat::BasicString<char16_t>;
    u16fbstring a;
    u16fbstring b;
    a.push_back(0);
    a.push_back(1);
    b.push_back(0);
    b.push_back(2);
    std::hash<u16fbstring> hashfunc;
    EXPECT_NE(hashfunc(a), hashfunc(b));
}
#endif

TEST(KString, testFrontBack) {
    KString str("hello");
    EXPECT_EQ(str.front(), 'h');
    EXPECT_EQ(str.back(), 'o');
    str.front() = 'H';
    EXPECT_EQ(str.front(), 'H');
    str.back() = 'O';
    EXPECT_EQ(str.back(), 'O');
    EXPECT_EQ(str, "HellO");
}

TEST(KString, noexcept) {
    EXPECT_TRUE(noexcept(KString()));
    KString x;
    EXPECT_FALSE(noexcept(KString(x)));
    EXPECT_TRUE(noexcept(KString(std::move(x))));
    KString y;
    EXPECT_FALSE(noexcept(y = x));
    EXPECT_TRUE(noexcept(y = std::move(x)));
}

TEST(KString, iomanip) {
    stringstream ss;
    KString kstr("Hello");

    ss << setw(6) << kstr;
    EXPECT_EQ(ss.str(), " Hello");
    ss.str("");

    ss << left << setw(6) << kstr;
    EXPECT_EQ(ss.str(), "Hello ");
    ss.str("");

    ss << right << setw(6) << kstr;
    EXPECT_EQ(ss.str(), " Hello");
    ss.str("");

    ss << setw(4) << kstr;
    EXPECT_EQ(ss.str(), "Hello");
    ss.str("");

    ss << setfill('^') << setw(6) << kstr;
    EXPECT_EQ(ss.str(), "^Hello");
    ss.str("");
}

TEST(KString, rvalueIterators) {
    // you cannot take &* of a move-iterator, so use that for testing
    KString s = "base";
    KString r = "hello";
    r.replace(
        r.begin(),
        r.end(),
        make_move_iterator(s.begin()),
        make_move_iterator(s.end()));
    EXPECT_EQ("base", r);

    // The following test is probably not required by the standard.
    // i.e. this could be in the realm of undefined behavior.
    KString b = "123abcXYZ";
    auto ait = b.begin() + 3;
    auto Xit = b.begin() + 6;
    b.replace(ait, b.end(), b.begin(), Xit);
    EXPECT_EQ("123123abc", b); // if things go wrong, you'd get "123123123"
}

TEST(KString, moveTerminator) {
    // The source of a move must remain in a valid state
    KString s(100, 'x'); // too big to be in-situ
    KString k;
    k = std::move(s);

    EXPECT_EQ(0, s.size());
    EXPECT_EQ('\0', *s.c_str());
}

namespace {
    struct TestStructDefaultAllocator {
        fermat::BasicString<char, 0> stringMember;
    };

    std::atomic<size_t> allocatorConstructedCount(0);

    struct TestStructStringAllocator : std::allocator<char> {
        TestStructStringAllocator() { ++allocatorConstructedCount; }
    };
} // namespace

TEST(FBStringCtorTest, DefaultInitStructDefaultAlloc) {
    TestStructDefaultAllocator t1{};
    EXPECT_TRUE(t1.stringMember.empty());
}

TEST(FBStringCtorTest, NullZeroConstruction) {
    char *p = nullptr;
    int n = 0;
    fermat::KString f(p, n);
    EXPECT_EQ(f.size(), 0);
}

// Tests for the comparison operators. I use EXPECT_TRUE rather than EXPECT_LE
// because what's under test is the operator rather than the relation between
// the objects.

TEST(KString, compareToStdString) {
    using fermat::KString;
    using namespace std::string_literals;
    auto stdA = "a"s;
    auto stdB = "b"s;
    KString fbA("a");
    KString fbB("b");
    EXPECT_TRUE(stdA == fbA);
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}

TEST(U16FBString, compareToStdU16String) {
    using fermat::BasicString;
    using namespace std::string_literals;
    auto stdA = u"a"s;
    auto stdB = u"b"s;
    BasicString<char16_t, 0> fbA(u"a");
    BasicString<char16_t, 0> fbB(u"b");
    EXPECT_TRUE(stdA == fbA) << "s:" << fbA.size() << "a:" << static_cast<int>(*fbA.data()) << "std:" << static_cast<
                                int>(*stdA.data());
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}

TEST(U32FBString, compareToStdU32String) {
    using fermat::BasicString;
    using namespace std::string_literals;
    auto stdA = U"a"s;
    auto stdB = U"b"s;
    BasicString<char32_t, 0> fbA(U"a");
    BasicString<char32_t, 0> fbB(U"b");

    EXPECT_TRUE(stdA == fbA);
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}

TEST(WFBString, compareToStdWString) {
    using fermat::BasicString;
    using namespace std::string_literals;
    auto stdA = L"a"s;
    auto stdB = L"b"s;
    BasicString<wchar_t, 0> fbA(L"a");
    BasicString<wchar_t, 0> fbB(L"b");
    EXPECT_TRUE(stdA == fbA) << stdA << " " << std::basic_string<wchar_t>(fbA.data(), fbA.size());
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}

// Same again, but with a more challenging input - a common prefix and different
// lengths.

TEST(KString, compareToStdStringLong) {
    using fermat::KString;
    using namespace std::string_literals;
    auto stdA = "1234567890a"s;
    auto stdB = "1234567890ab"s;
    KString fbA("1234567890a");
    KString fbB("1234567890ab");
    EXPECT_TRUE(stdA == fbA);
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}

TEST(U16FBString, compareToStdU16StringLong) {
    using fermat::BasicString;
    using namespace std::string_literals;
    auto stdA = u"1234567890a"s;
    auto stdB = u"1234567890ab"s;
    BasicString<char16_t, 0> fbA(u"1234567890a");
    BasicString<char16_t, 0> fbB(u"1234567890ab");
    EXPECT_TRUE(stdA == fbA);
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}

#if FOLLY_HAVE_WCHAR_SUPPORT
TEST(U32FBString, compareToStdU32StringLong) {
    using fermat::BasicString;
    using namespace std::string_literals;
    auto stdA = U"1234567890a"s;
    auto stdB = U"1234567890ab"s;
    BasicString<char32_t> fbA(U"1234567890a");
    BasicString<char32_t> fbB(U"1234567890ab");
    EXPECT_TRUE(stdA == fbA);
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}

TEST(WFBString, compareToStdWStringLong) {
    using fermat::BasicString;
    using namespace std::string_literals;
    auto stdA = L"1234567890a"s;
    auto stdB = L"1234567890ab"s;
    BasicString<wchar_t> fbA(L"1234567890a");
    BasicString<wchar_t> fbB(L"1234567890ab");
    EXPECT_TRUE(stdA == fbA);
    EXPECT_TRUE(fbB == stdB);
    EXPECT_TRUE(stdA != fbB);
    EXPECT_TRUE(fbA != stdB);
    EXPECT_TRUE(stdA < fbB);
    EXPECT_TRUE(fbA < stdB);
    EXPECT_TRUE(stdB > fbA);
    EXPECT_TRUE(fbB > stdA);
    EXPECT_TRUE(stdA <= fbB);
    EXPECT_TRUE(fbA <= stdB);
    EXPECT_TRUE(stdA <= fbA);
    EXPECT_TRUE(fbA <= stdA);
    EXPECT_TRUE(stdB >= fbA);
    EXPECT_TRUE(fbB >= stdA);
    EXPECT_TRUE(stdB >= fbB);
    EXPECT_TRUE(fbB >= stdB);
}
#endif

struct custom_traits : public std::char_traits<char> {
};

TEST(KString, convertFromStringView) {
    {
        fermat::KString test{std::string_view("foo")};
        std::string control{std::string_view("foo")};
        EXPECT_EQ(test, "foo");
        EXPECT_EQ(test, control);
    }
    {
        fermat::KString test{std::string_view("abcfooabc"), 3, 3};
        std::string control{std::string_view("abcfooabc"), 3, 3};
        EXPECT_EQ(test, "foo");
        EXPECT_EQ(test, control);
    }
    {
        using sv_type = std::basic_string_view<char, custom_traits>;
        fermat::BasicString<char, 0, custom_traits> test{sv_type("foo")};
        std::basic_string<char, custom_traits> control{sv_type("foo")};
        EXPECT_EQ(test, "foo");
        EXPECT_EQ(test, control);
    }
    {
        using sv_type = std::basic_string_view<char, custom_traits>;
        fermat::BasicString<char, 0, custom_traits> test{sv_type("abcfooabc"), 3, 3};
        std::basic_string<char, custom_traits> control{sv_type("abcfooabc"), 3, 3};
        EXPECT_EQ(test, "foo");
        EXPECT_EQ(test, control);
    }
}

TEST(KString, convertToStringView) {
    fermat::KString s("foo");
    std::string_view sv = s;
    EXPECT_EQ(sv, "foo");
    fermat::BasicString<char, 0, custom_traits> s2("bar");
    std::basic_string_view<char, custom_traits> sv2 = s2;
    EXPECT_EQ(sv2, "bar");
}

TEST(KString, testHash) {
    KString a;
    KString b;
    a.push_back(0);
    a.push_back(1);
    b.push_back(0);
    b.push_back(2);
    std::hash<KString> hashfunc;
    EXPECT_NE(hashfunc(a), hashfunc(b));
    EXPECT_NE(turbo::Hash<KString>()(a), turbo::Hash<KString>()(b));
}

TEST(KString, fmt) {
    EXPECT_EQ("  foo", turbo::str_cat("  ", fermat::KString("foo")));
    EXPECT_EQ("  foo", turbo::str_format("  %v", fermat::KString("foo")));
}


TEST(KString, FindEmptyString) {
    // Case 1: pos within range, pos == size() (string "d", size=1, pos=1)
    {
        fermat::KString s1 = "d";
        std::string s2 = "d";
        size_t pos = 1;
        size_t r1 = s1.find("", pos);
        size_t r2 = s2.find("", pos);
        KLOG(INFO) << "fermat::KString find(\"\", 1) on \"d\" -> " << r1;
        KLOG(INFO) << "std::string   find(\"\", 1) on \"d\" -> " << r2;
        EXPECT_EQ(r1, r2);
    }

    // Case 2: pos within range, pos < size() (string "abc", pos=1)
    {
        fermat::KString s1 = "abc";
        std::string s2 = "abc";
        size_t pos = 1;
        size_t r1 = s1.find("", pos);
        size_t r2 = s2.find("", pos);
        KLOG(INFO) << "fermat::KString find(\"\", 1) on \"abc\" -> " << r1;
        KLOG(INFO) << "std::string   find(\"\", 1) on \"abc\" -> " << r2;
        EXPECT_EQ(r1, r2);
    }

    // Case 3: pos exactly equal to size() (string "abc", pos=3)
    {
        fermat::KString s1 = "abc";
        std::string s2 = "abc";
        size_t pos = 3;
        size_t r1 = s1.find("", pos);
        size_t r2 = s2.find("", pos);
        KLOG(INFO) << "fermat::KString find(\"\", 3) on \"abc\" -> " << r1;
        KLOG(INFO) << "std::string   find(\"\", 3) on \"abc\" -> " << r2;
        EXPECT_EQ(r1, r2);
    }

    // Case 4: pos out of range (pos > size())
    {
        fermat::KString s1 = "abc";
        std::string s2 = "abc";
        size_t pos = 5;
        size_t r1 = s1.find("", pos);
        size_t r2 = s2.find("", pos);
        KLOG(INFO) << "fermat::KString find(\"\", 5) on \"abc\" -> " << r1;
        KLOG(INFO) << "std::string   find(\"\", 5) on \"abc\" -> " << r2;
        EXPECT_EQ(r1, r2);
    }

    fermat::KString s1;
    KLOG(INFO) << strlen(s1.c_str());
}
