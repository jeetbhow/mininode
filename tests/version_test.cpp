#include <gtest/gtest.h>

#include <http/version.hpp>

TEST(Version, LinksAgainstLibrary) {
    EXPECT_EQ(http::version(), "0.0.1");
}
