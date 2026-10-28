#include <gtest/gtest.h>

#include "http/router.hpp" 

using namespace std;
using namespace http;

TEST(Router, ErrorOnNoForwardSlashPrefix) {
    Router router{};
    router.add(Method::Get, "invalid");

    ASSERT_EQ(router.size(), 0);
    EXPECT_FALSE(router.contains(Method::Get, "invalid"));
}


TEST(Router, AddsSingleRoute) {
    Router router{};
    router.add(Method::Get, "/test");

    ASSERT_EQ(router.size(), 1);
    EXPECT_EQ(router.contains(Method::Get, "/test"), true);
}

TEST(Router, AddTwoRoutesSameMethod) {
    Router router{};
    router.add(Method::Get, "/test1");
    router.add(Method::Get, "/test2");

    ASSERT_EQ(router.size(), 2);
    EXPECT_EQ(router.contains(Method::Get, "/test1"), true);
    EXPECT_EQ(router.contains(Method::Get, "/test2"), true);
}

TEST(Router, AddTwoRoutesDiffMethods) {
    Router router{};
    router.add(Method::Get, "/test1");
    router.add(Method::Put, "/test2");

    ASSERT_EQ(router.size(), 2);
    EXPECT_EQ(router.contains(Method::Get, "/test1"), true);
    EXPECT_EQ(router.contains(Method::Put, "/test2"), true);
}

TEST(Router, DuplicateAddIsCountedOnce) {
    Router router{};
    router.add(Method::Get, "/test");
    router.add(Method::Get, "/test");

    ASSERT_EQ(router.size(), 1);
    EXPECT_EQ(router.contains(Method::Get, "/test"), true);
}