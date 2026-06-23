/// scope_exit_gtest.cpp
/// Google Test conversion of range-v3 scope_exit test.
/// All comments in English, using /// Doxygen style.
/// No C++20 `requires` syntax. Uses only C++17 and below.

#include <gtest/gtest.h>

#include <fermat/utility/scope_exit.h>   /// fermat::ranges::make_scope_exit

namespace
{
    int i = 0;

    struct NoexceptFalse
    {
        NoexceptFalse() = default;
        NoexceptFalse(NoexceptFalse const &) noexcept(false)
        {}

        NoexceptFalse(NoexceptFalse &&) noexcept(false)
        {
            ADD_FAILURE() << "NoexceptFalse move constructor called unexpectedly";
        }

        void operator()() const
        {
            ++i;
        }
    };

    struct ThrowingCopy
    {
        ThrowingCopy() = default;
        [[noreturn]] ThrowingCopy(ThrowingCopy const &) noexcept(false)
        {
            throw 42;
        }

        ThrowingCopy(ThrowingCopy &&) noexcept(false)
        {
            ADD_FAILURE() << "ThrowingCopy move constructor called unexpectedly";
        }

        void operator()() const
        {
            ++i;
        }
    };
} // unnamed namespace

TEST(ScopeExitTest, BasicLambda)
{
    i = 0;
    {
        auto guard = fermat::ranges::make_scope_exit([&]{ ++i; });
        EXPECT_EQ(i, 0);
    }
    EXPECT_EQ(i, 1);
}

TEST(ScopeExitTest, NoexceptFalse)
{
    i = 0;
    {
        auto guard = fermat::ranges::make_scope_exit(NoexceptFalse{});
        EXPECT_EQ(i, 0);
    }
    EXPECT_EQ(i, 1);
}

TEST(ScopeExitTest, ThrowingCopy)
{
    i = 0;
    try
    {
        auto guard = fermat::ranges::make_scope_exit(ThrowingCopy{});
        // Should not reach here because the copy constructor of ThrowingCopy throws.
        ADD_FAILURE() << "Expected exception was not thrown";
    }
    catch (int)
    {
        // Expected exception
    }
    EXPECT_EQ(i, 1);
}
