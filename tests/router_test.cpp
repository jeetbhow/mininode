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
    // Router::find() is private; this fixture is a friend so tests can inspect the trie.
    RouteNode* find(std::string_view path) { return router.find(path); }

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

TEST_F(RouterTest, AddInvalidPathIsNoOp) {
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        router.add(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_P(RouterMethodTest, AddRegistersSingleRoute) {
    const auto method = GetParam();
    router.add(method, "/a");

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(method, "/a");
    EXPECT_TRUE(a);
}

TEST_F(RouterTest, AddRegistersTwoRoutesWithSameMethod) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/b");

    EXPECT_EQ(router.size(), 2u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);

    auto b = router.contains(Method::Get, "/b");
    EXPECT_TRUE(b);
}

TEST_P(RouterMethodPairTest, AddRegistersTwoRoutesWithDifferentMethods) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(second, "/b");

    EXPECT_EQ(router.size(), 2u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_b = router.contains(second, "/b");
    EXPECT_TRUE(second_b);

    auto first_b = router.contains(first, "/b");
    EXPECT_FALSE(first_b);

    auto second_a = router.contains(second, "/a");
    EXPECT_FALSE(second_a);
}

// Router::contains()

TEST_F(RouterTest, ContainsInvalidPathReturnsFalse) {
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(router.contains(Method::Get, path));
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

TEST_F(RouterTest, ContainsReturnsFalseForUnaddedRoute) {
    router.add(Method::Get, "/a");

    auto b = router.contains(Method::Get, "/b");
    EXPECT_FALSE(b);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_FALSE(a_b);
}

TEST_F(RouterTest, ContainsReturnsFalseForIntermediateNode) {
    router.add(Method::Get, "/a/b");

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);
}

TEST_P(RouterMethodPairTest, ContainsReturnsFalseForDifferentMethod) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");

    auto a = router.contains(second, "/a");
    EXPECT_FALSE(a);
}

// Router::match()

TEST_F(RouterTest, MatchInvalidPathReturnsFalse) {
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(router.match(Method::Get, path));
        EXPECT_EQ(router.size(), 1u);
        EXPECT_TRUE(router.contains(Method::Get, "/a"));
    }
}

// Router::remove()

TEST_F(RouterTest, RemoveOnEmptyRouterIsNoOp) {
    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 0u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);
}

TEST_F(RouterTest, RemoveInvalidPathIsNoOp) {
    router.add(Method::Get, "/a");

    for (const auto path : {"", "a"}) {
        SCOPED_TRACE(path);
        router.remove(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        auto a = router.contains(Method::Get, "/a");
        EXPECT_TRUE(a);
    }
}

TEST_F(RouterTest, RemoveNonExistentPathIsNoOp) {
    router.add(Method::Get, "/a");

    for (const auto path : {"/b", "/a/b"}) {
        SCOPED_TRACE(path);
        router.remove(Method::Get, path);

        EXPECT_EQ(router.size(), 1u);
        auto a = router.contains(Method::Get, "/a");
        EXPECT_TRUE(a);

        auto missing = router.contains(Method::Get, path);
        EXPECT_FALSE(missing);
    }
}

TEST_P(RouterMethodPairTest, RemoveNonExistentMethodIsNoOp) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");

    router.remove(second, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_FALSE(second_a);
}

TEST_P(RouterMethodPairTest, RemoveConservesOtherMethods) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(second, "/a");

    router.remove(first, "/a");
    EXPECT_EQ(router.size(), 1u);

    auto first_a = router.contains(first, "/a");
    EXPECT_FALSE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_TRUE(second_a);
}

TEST_F(RouterTest, RemoveClearsRouteOnEmptyMethods) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/b");

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);

    auto b = router.contains(Method::Get, "/b");
    EXPECT_TRUE(b);

    router.add(Method::Get, "/a");
    EXPECT_EQ(router.size(), 3u);
    a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

TEST_F(RouterTest, RemoveConservesInternalNode) {
    router.add(Method::Get, "/a/b");
    router.add(Method::Get, "/a/c");

    router.remove(Method::Get, "/a");

    EXPECT_EQ(router.size(), 2u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_FALSE(a);

    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_TRUE(a_b);

    auto a_c = router.contains(Method::Get, "/a/c");
    EXPECT_TRUE(a_c);
}

TEST_P(RouterMethodPairTest, RemoveDeletesChildNodeIfNoMethods) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(first, "/a/b");
    router.add(second, "/a/b");
    router.add(first, "/a/c");

    router.remove(first, "/a/b");
    EXPECT_EQ(router.size(), 3u);
    auto first_a_b = router.contains(first, "/a/b");
    EXPECT_FALSE(first_a_b);

    auto second_a_b = router.contains(second, "/a/b");
    EXPECT_TRUE(second_a_b);

    router.remove(second, "/a/b");
    EXPECT_EQ(router.size(), 2u);
    first_a_b = router.contains(first, "/a/b");
    EXPECT_FALSE(first_a_b);

    second_a_b = router.contains(second, "/a/b");
    EXPECT_FALSE(second_a_b);

    // The "b" node is gone, so the lookup stops at its parent.
    EXPECT_EQ(find("/a/b")->value, "a");

    auto a = router.contains(first, "/a");
    EXPECT_TRUE(a);

    auto a_c = router.contains(first, "/a/c");
    EXPECT_TRUE(a_c);
}

TEST_F(RouterTest, RemovePrunesChainOfRoutelessNodes) {
    router.add(Method::Get, "/a/b/c");
    router.add(Method::Get, "/d");

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    auto a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_FALSE(a_b_c);

    auto d = router.contains(Method::Get, "/d");
    EXPECT_TRUE(d);

    // The whole "a/b/c" chain is gone, so the lookup stops at the root.
    EXPECT_EQ(find("/a/b/c")->parent, nullptr);

    router.add(Method::Get, "/a/b/c");
    EXPECT_EQ(router.size(), 2u);
    a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_TRUE(a_b_c);
}

TEST_F(RouterTest, RemovePruningStopsAtAncestorWithRoute) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a/b/c");

    router.remove(Method::Get, "/a/b/c");

    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);

    auto a_b_c = router.contains(Method::Get, "/a/b/c");
    EXPECT_FALSE(a_b_c);

    // "c" and "b" are pruned, but "a" still holds a route.
    const RouteNode* node = find("/a/b/c");
    EXPECT_EQ(node->value, "a");
    EXPECT_TRUE(node->children.empty());
}

TEST_F(RouterTest, RemoveLastRouteLeavesRouterUsable) {
    router.add(Method::Get, "/a/b");
    router.remove(Method::Get, "/a/b");

    EXPECT_EQ(router.size(), 0u);
    auto a_b = router.contains(Method::Get, "/a/b");
    EXPECT_FALSE(a_b);

    // Only the root is left.
    const RouteNode* root = find("/a/b");
    EXPECT_EQ(root->parent, nullptr);
    EXPECT_TRUE(root->children.empty());

    router.add(Method::Get, "/a");
    EXPECT_EQ(router.size(), 1u);
    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

// Router::size()

TEST_F(RouterTest, SizeCountsDuplicateRouteOnce) {
    router.add(Method::Get, "/a");
    router.add(Method::Get, "/a");

    EXPECT_EQ(router.size(), 1u);

    auto a = router.contains(Method::Get, "/a");
    EXPECT_TRUE(a);
}

TEST_P(RouterMethodPairTest, SizeCountsEachMethodOnSamePath) {
    const auto [first, second] = GetParam();
    router.add(first, "/a");
    router.add(second, "/a");

    EXPECT_EQ(router.size(), 2u);

    auto first_a = router.contains(first, "/a");
    EXPECT_TRUE(first_a);

    auto second_a = router.contains(second, "/a");
    EXPECT_TRUE(second_a);
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
