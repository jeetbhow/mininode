#include "http/http.hpp"
#include <gtest/gtest.h>

using namespace std;
using namespace http;

TEST(RequestLine, ParsesMethodTargetAndVersion) {
    string request = "GET /users/42?sort=asc HTTP/1.1\r\n"
                     "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(r->method, Method::Get);
    EXPECT_EQ(r->target, "/users/42?sort=asc");
    EXPECT_EQ(r->version, Version::Http11);
}

using VersionCase = tuple<string, Version>;

class VersionTest : public ::testing::TestWithParam<VersionCase> {};

TEST_P(VersionTest, MapsVersionString) {
    auto [name, version] = GetParam();
    string request = "GET / " + name + "\r\n"
                                      "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(r->version, version);
}

INSTANTIATE_TEST_SUITE_P(
    RequestLine,
    VersionTest,
    ::testing::Values(
        VersionCase{"HTTP/1.0", Version::Http10},
        VersionCase{"HTTP/1.1", Version::Http11}
    )
);

using MethodCase = tuple<string, Method>;

class MethodNameTest : public ::testing::TestWithParam<MethodCase> {};

TEST_P(MethodNameTest, MapsMethodName) {
    auto [name, method] = GetParam();
    string request = name + " / HTTP/1.1\r\n"
                            "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(r->method, method);
}

INSTANTIATE_TEST_SUITE_P(
    RequestLine,
    MethodNameTest,
    ::testing::Values(
        MethodCase{"GET", Method::Get},
        MethodCase{"HEAD", Method::Head},
        MethodCase{"POST", Method::Post},
        MethodCase{"PUT", Method::Put},
        MethodCase{"DELETE", Method::Delete},
        MethodCase{"PATCH", Method::Patch},
        MethodCase{"OPTIONS", Method::Options}
    )
);

TEST(RequestLine, ErrorOnUnknownMethod) {
    string request = "INVALID / HTTP/1.1\r\n"
                     "\r\n";

    auto r = parse_request(request);

    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), ParseError::UnsupportedMethod);
}

TEST(RequestLine, ErrorOnLowercaseMethod) {
    string request = "get / HTTP/1.1\r\n"
                     "\r\n";

    auto r = parse_request(request);

    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), ParseError::UnsupportedMethod);
}

TEST(RequestLine, ErrorOnUnsupportedVersion) {
    string request = "GET / HTTP/2.0\r\n"
                     "\r\n";

    auto r = parse_request(request);

    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), ParseError::UnsupportedVersion);
}

class MalformedRequestLineTest : public ::testing::TestWithParam<string> {};

TEST_P(MalformedRequestLineTest, ErrorOnMalformedLine) {
    auto r = parse_request(GetParam());

    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), ParseError::BadRequest);
}

INSTANTIATE_TEST_SUITE_P(
    RequestLine,
    MalformedRequestLineTest,
    ::testing::Values(
        "GET /\r\n\r\n",                    // missing version
        "GET / HTTP/1.1 extra\r\n\r\n",     // extra part
        "GET  / HTTP/1.1\r\n\r\n",          // doubled space
        " GET / HTTP/1.1\r\n\r\n",          // leading space
        "GET / garbage\r\n\r\n",            // malformed version, not unsupported
        "\r\n"                              // empty line
    )
);
