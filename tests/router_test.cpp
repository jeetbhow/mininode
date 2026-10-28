#include <gtest/gtest.h>

#include "http/router.hpp"

#include <array>
#include <string>
#include <utility>
#include <vector>

using namespace http;

constexpr std::array methods{
    Method::Get, Method::Head, Method::Post, Method::Put,
    Method::Delete, Method::Patch, Method::Options,
};

class RouterMethodTest : public testing::TestWithParam<Method> {};

TEST_P(RouterMethodTest, ErrorOnNoForwardSlashPrefix) {
    Router router{};
    const auto method = GetParam();

    EXPECT_EQ(router.add(method, "a"), RouterError::InvalidRoute);
    EXPECT_EQ(router.size(), 0u);

    auto result = router.contains(method, "a");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RouterError::InvalidRoute);
}

TEST_P(RouterMethodTest, AddsSingleRoute) {
    Router router{};
    const auto method = GetParam();
    ASSERT_EQ(router.add(method, "/a"), RouterError::Ok);

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(method, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);
}

TEST_P(RouterMethodTest, DoesNotContainUnaddedRoute) {
    Router router{};
    const auto method = GetParam();
    ASSERT_EQ(router.add(method, "/a"), RouterError::Ok);

    auto b = router.contains(method, "/b");
    ASSERT_TRUE(b.has_value());
    EXPECT_FALSE(*b);

    auto a_b = router.contains(method, "/a/b");
    ASSERT_TRUE(a_b.has_value());
    EXPECT_FALSE(*a_b);
}

TEST_P(RouterMethodTest, IntermediateNodeIsNotARoute) {
    Router router{};
    const auto method = GetParam();
    ASSERT_EQ(router.add(method, "/a/b"), RouterError::Ok);

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(method, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_FALSE(*a);

    auto a_b = router.contains(method, "/a/b");
    ASSERT_TRUE(a_b.has_value());
    EXPECT_TRUE(*a_b);
}

TEST_P(RouterMethodTest, AddTwoRoutesSameMethod) {
    Router router{};
    const auto method = GetParam();
    ASSERT_EQ(router.add(method, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(method, "/b"), RouterError::Ok);

    EXPECT_EQ(router.size(), 2u);

    auto a = router.contains(method, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);

    auto b = router.contains(method, "/b");
    ASSERT_TRUE(b.has_value());
    EXPECT_TRUE(*b);
}

TEST_P(RouterMethodTest, DuplicateAddIsCountedOnce) {
    Router router{};
    const auto method = GetParam();
    ASSERT_EQ(router.add(method, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(method, "/a"), RouterError::Ok);

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(method, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);
}

INSTANTIATE_TEST_SUITE_P(
    AllMethods, RouterMethodTest, testing::ValuesIn(methods),
    [](const testing::TestParamInfo<Method>& info) {
        return std::string(http::to_string(info.param));
    });

using MethodPair = std::pair<Method, Method>;

std::vector<MethodPair> distinct_method_pairs() {
    std::vector<MethodPair> pairs;
    for (const auto first : methods) {
        for (const auto second : methods) {
            if (first != second) {
                pairs.emplace_back(first, second);
            }
        }
    }
    return pairs;
}

class RouterMethodPairTest : public testing::TestWithParam<MethodPair> {};

TEST_P(RouterMethodPairTest, DoesNotContainRouteWithDifferentMethod) {
    Router router{};
    const auto [first, second] = GetParam();
    ASSERT_EQ(router.add(first, "/a"), RouterError::Ok);

    auto a = router.contains(second, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_FALSE(*a);
}

TEST_P(RouterMethodPairTest, AddTwoRoutesDiffMethods) {
    Router router{};
    const auto [first, second] = GetParam();
    ASSERT_EQ(router.add(first, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(second, "/b"), RouterError::Ok);

    EXPECT_EQ(router.size(), 2u);

    auto first_a = router.contains(first, "/a");
    ASSERT_TRUE(first_a.has_value());
    EXPECT_TRUE(*first_a);

    auto second_b = router.contains(second, "/b");
    ASSERT_TRUE(second_b.has_value());
    EXPECT_TRUE(*second_b);

    auto first_b = router.contains(first, "/b");
    ASSERT_TRUE(first_b.has_value());
    EXPECT_FALSE(*first_b);

    auto second_a = router.contains(second, "/a");
    ASSERT_TRUE(second_a.has_value());
    EXPECT_FALSE(*second_a);
}

TEST_P(RouterMethodPairTest, SamePathDiffMethodsCountsEach) {
    Router router{};
    const auto [first, second] = GetParam();
    ASSERT_EQ(router.add(first, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(second, "/a"), RouterError::Ok);

    EXPECT_EQ(router.size(), 2u);

    auto first_a = router.contains(first, "/a");
    ASSERT_TRUE(first_a.has_value());
    EXPECT_TRUE(*first_a);

    auto second_a = router.contains(second, "/a");
    ASSERT_TRUE(second_a.has_value());
    EXPECT_TRUE(*second_a);
}

INSTANTIATE_TEST_SUITE_P(
    DistinctMethods,
    RouterMethodPairTest,
    testing::ValuesIn(distinct_method_pairs()),
    [](const testing::TestParamInfo<MethodPair>& info) {
        return std::string(http::to_string(info.param.first)) + "_" +
               std::string(http::to_string(info.param.second));
    });
