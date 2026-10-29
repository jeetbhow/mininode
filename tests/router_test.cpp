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

class RouterTest : public testing::Test {
protected:
    Router router;
};

class RouterMethodTest : public RouterTest,
                         public testing::WithParamInterface<Method> {};

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

class RouterMethodPairTest : public RouterTest,
                             public testing::WithParamInterface<MethodPair> {};

// Router::add()

TEST_F(RouterTest, AddAndContainsRejectPathsWithoutLeadingSlash) {
    EXPECT_EQ(router.add(Method::Get, "a"), RouterError::InvalidRoute);
    EXPECT_EQ(router.size(), 0u);

    auto result = router.contains(Method::Get, "a");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), RouterError::InvalidRoute);
}

TEST_P(RouterMethodTest, AddRegistersSingleRoute) {
    const auto method = GetParam();
    ASSERT_EQ(router.add(method, "/a"), RouterError::Ok);

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(method, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);
}

TEST_F(RouterTest, AddRegistersTwoRoutesWithSameMethod) {
    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(Method::Get, "/b"), RouterError::Ok);

    EXPECT_EQ(router.size(), 2u);

    auto a = router.contains(Method::Get, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);

    auto b = router.contains(Method::Get, "/b");
    ASSERT_TRUE(b.has_value());
    EXPECT_TRUE(*b);
}

TEST_P(RouterMethodPairTest, AddRegistersTwoRoutesWithDifferentMethods) {
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

// Router::contains()

TEST_F(RouterTest, ContainsReturnsFalseForUnaddedRoute) {
    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);

    auto b = router.contains(Method::Get, "/b");
    ASSERT_TRUE(b.has_value());
    EXPECT_FALSE(*b);

    auto a_b = router.contains(Method::Get, "/a/b");
    ASSERT_TRUE(a_b.has_value());
    EXPECT_FALSE(*a_b);
}

TEST_F(RouterTest, ContainsReturnsFalseForIntermediateNode) {
    ASSERT_EQ(router.add(Method::Get, "/a/b"), RouterError::Ok);

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_FALSE(*a);

    auto a_b = router.contains(Method::Get, "/a/b");
    ASSERT_TRUE(a_b.has_value());
    EXPECT_TRUE(*a_b);
}

TEST_P(RouterMethodPairTest, ContainsReturnsFalseForDifferentMethod) {
    const auto [first, second] = GetParam();
    ASSERT_EQ(router.add(first, "/a"), RouterError::Ok);

    auto a = router.contains(second, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_FALSE(*a);
}

// Router::remove()

TEST_F(RouterTest, RemoveOnEmptyRouterIsNoOp) {
    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 0u);
    auto a = router.contains(Method::Get, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_FALSE(*a);
}

TEST_F(RouterTest, RemoveInvalidPathIsNoOp) {
    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        router.remove(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        auto a = router.contains(Method::Get, "/a");
        ASSERT_TRUE(a.has_value());
        EXPECT_TRUE(*a);
    }
}

TEST_F(RouterTest, RemoveNonExistentPathIsNoOp) {
    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);

    for (const auto path : {"/b", "/a/b"}) {
        SCOPED_TRACE(path);
        router.remove(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        auto a = router.contains(Method::Get, "/a");
        ASSERT_TRUE(a.has_value());
        EXPECT_TRUE(*a);

        auto missing = router.contains(Method::Get, path);
        ASSERT_TRUE(missing.has_value());
        EXPECT_FALSE(*missing);
    }
}

TEST_P(RouterMethodPairTest, RemoveNonExistentMethodIsNoOp) {
    const auto [first, second] = GetParam();
    ASSERT_EQ(router.add(first, "/a"), RouterError::Ok);

    router.remove(second, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    ASSERT_TRUE(first_a.has_value());
    EXPECT_TRUE(*first_a);

    auto second_a = router.contains(second, "/a");
    ASSERT_TRUE(second_a.has_value());
    EXPECT_FALSE(*second_a);
}

TEST_P(RouterMethodPairTest, RemoveConservesOtherMethods) {
    const auto [first, second] = GetParam();
    ASSERT_EQ(router.add(first, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(second, "/a"), RouterError::Ok);

    router.remove(first, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    ASSERT_TRUE(first_a.has_value());
    EXPECT_FALSE(*first_a);

    auto second_a = router.contains(second, "/a");
    ASSERT_TRUE(second_a.has_value());
    EXPECT_TRUE(*second_a);
}

TEST_F(RouterTest, RemoveClearsRouteOnEmptyMethods) {
    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(Method::Get, "/a/b"), RouterError::Ok);
    ASSERT_EQ(router.add(Method::Get, "/b"), RouterError::Ok);

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_FALSE(*a);

    auto a_b = router.contains(Method::Get, "/a/b");
    ASSERT_TRUE(a_b.has_value());
    EXPECT_TRUE(*a_b);

    auto b = router.contains(Method::Get, "/b");
    ASSERT_TRUE(b.has_value());
    EXPECT_TRUE(*b);

    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);
    EXPECT_EQ(router.size(), 3u);
    a = router.contains(Method::Get, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);
}

TEST_F(RouterTest, RemoveConservesInternalNode) {
    ASSERT_EQ(router.add(Method::Get, "/a/b"), RouterError::Ok);
    ASSERT_EQ(router.add(Method::Get, "/a/c"), RouterError::Ok);

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_FALSE(*a);

    auto a_b = router.contains(Method::Get, "/a/b");
    ASSERT_TRUE(a_b.has_value());
    EXPECT_TRUE(*a_b);

    auto a_c = router.contains(Method::Get, "/a/c");
    ASSERT_TRUE(a_c.has_value());
    EXPECT_TRUE(*a_c);
}

TEST_P(RouterMethodPairTest, RemoveDeletesChildNodeIfNoMethods) {
    const auto [first, second] = GetParam();
    ASSERT_EQ(router.add(first, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(first, "/a/b"), RouterError::Ok);
    ASSERT_EQ(router.add(second, "/a/b"), RouterError::Ok);
    ASSERT_EQ(router.add(first, "/a/c"), RouterError::Ok);

    router.remove(first, "/a/b");
    EXPECT_EQ(router.size(), 3u);
    auto first_a_b = router.contains(first, "/a/b");
    ASSERT_TRUE(first_a_b.has_value());
    EXPECT_FALSE(*first_a_b);

    auto second_a_b = router.contains(second, "/a/b");
    ASSERT_TRUE(second_a_b.has_value());
    EXPECT_TRUE(*second_a_b);

    router.remove(second, "/a/b");
    EXPECT_EQ(router.size(), 2u);
    first_a_b = router.contains(first, "/a/b");
    ASSERT_TRUE(first_a_b.has_value());
    EXPECT_FALSE(*first_a_b);

    second_a_b = router.contains(second, "/a/b");
    ASSERT_TRUE(second_a_b.has_value());
    EXPECT_FALSE(*second_a_b);

    auto a = router.contains(first, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);

    auto a_c = router.contains(first, "/a/c");
    ASSERT_TRUE(a_c.has_value());
    EXPECT_TRUE(*a_c);
}

// Router::size()

TEST_F(RouterTest, SizeCountsDuplicateRouteOnce) {
    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);
    ASSERT_EQ(router.add(Method::Get, "/a"), RouterError::Ok);

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    ASSERT_TRUE(a.has_value());
    EXPECT_TRUE(*a);
}

TEST_P(RouterMethodPairTest, SizeCountsEachMethodOnSamePath) {
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
    AllMethods,
    RouterMethodTest,
    testing::ValuesIn(methods),
    [](const testing::TestParamInfo<Method>& info) {
        return std::string(http::to_string(info.param));
    }
);

INSTANTIATE_TEST_SUITE_P(
    DistinctMethods,
    RouterMethodPairTest,
    testing::ValuesIn(distinct_method_pairs()),
    [](const testing::TestParamInfo<MethodPair>& info) {
        return std::string(http::to_string(info.param.first)) + "_" +
               std::string(http::to_string(info.param.second));
    }
);
