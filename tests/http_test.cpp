#include "http/http.hpp"
#include <gtest/gtest.h>

using namespace std;
using namespace http;

TEST(RequestLine, ParsesMethodTargetAndVersion) {
    string request = "GET /users/42 HTTP/1.1\r\n"
                     "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(r->method, Method::Get);
    EXPECT_EQ(r->path, "/users/42");
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

TEST_P(MethodNameTest, ConvertsMethodToString) {
    auto [name, method] = GetParam();

    EXPECT_EQ(to_string(method), name);
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

TEST(HeaderFields, ParsesHeaderFields) {
    string request = "GET / HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Accept: text/html\r\n"
                     "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    ASSERT_TRUE(r->headers.fields.size() == 2);
    EXPECT_EQ(r->headers.get("Host"), "example.com");
    EXPECT_EQ(r->headers.get("Accept"), "text/html");
}

TEST(HeaderFields, NoHeaderFields) {
    string request = "GET / HTTP/1.1\r\n"
                     "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_TRUE(r->headers.fields.empty());
}

TEST(HeaderFields, PreservesDuplicatesInOrder) {
    string request = "GET / HTTP/1.1\r\n"
                     "Accept: application/json\r\n"
                     "Host: example.com\r\n"
                     "Accept: text/html\r\n"
                     "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(r->headers.get_all("Accept"), (vector<string>{"application/json", "text/html"}));
}

TEST(HeaderFields, ValueMayContainColon) {
    string request = "GET / HTTP/1.1\r\n"
                     "Host: example.com:8080\r\n"
                     "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(r->headers.get("Host"), "example.com:8080");
}

TEST(HeaderFields, StopsAtEmptyLine) {
    string request = "POST / HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "\r\n"
                     "Not-A-Header: body\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    ASSERT_TRUE(r->headers.fields.size() == 1);
    EXPECT_FALSE(r->headers.get("Not-A-Header").has_value());
}

using WhitespaceCase = tuple<string, string>;

class HeaderWhitespaceTest : public ::testing::TestWithParam<WhitespaceCase> {};

TEST_P(HeaderWhitespaceTest, TrimsOptionalWhitespace) {
    auto [line, value] = GetParam();
    string request = "GET / HTTP/1.1\r\n" + line + "\r\n"
                                                   "\r\n";

    auto r = parse_request(request);
    ASSERT_TRUE(r.has_value());

    EXPECT_EQ(r->headers.get("X-Test"), value);
}

INSTANTIATE_TEST_SUITE_P(
    HeaderFields,
    HeaderWhitespaceTest,
    ::testing::Values(
        WhitespaceCase{"X-Test: value", "value"},
        WhitespaceCase{"X-Test:value", "value"},
        WhitespaceCase{"X-Test:   value   ", "value"},
        WhitespaceCase{"X-Test:\tvalue\t", "value"},
        WhitespaceCase{"X-Test: two words", "two words"},
        WhitespaceCase{"X-Test:", ""},
        WhitespaceCase{"X-Test:   ", ""}
    )
);

class MalformedHeaderTest : public ::testing::TestWithParam<string> {};

TEST_P(MalformedHeaderTest, ErrorOnMalformedHeader) {
    auto r = parse_request(GetParam());

    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), ParseError::BadRequest);
}

INSTANTIATE_TEST_SUITE_P(
    HeaderFields,
    MalformedHeaderTest,
    ::testing::Values(
        "GET / HTTP/1.1\r\nHost example.com\r\n\r\n",                   // missing colon
        "GET / HTTP/1.1\r\nHost : example.com\r\n\r\n",                 // space before colon
        "GET / HTTP/1.1\r\n: example.com\r\n\r\n",                      // empty name
        "GET / HTTP/1.1\r\nHost: example.com\r\n folded\r\n\r\n",       // obsolete line folding
        "GET / HTTP/1.1\r\nX(Bad): value\r\n\r\n",                      // non-token name
        "GET / HTTP/1.1\r\nHost: a\nAccept: b\r\n\r\n",                 // bare LF line ending
        "GET / HTTP/1.1\r\nHost: a\rAccept: b\r\n\r\n",                 // bare CR in value
        "GET / HTTP/1.1\r\nHost: a\0b\r\n\r\n"s,                        // NUL in value
        "GET / HTTP/1.1\r\nHost: example.com\r\n",                      // no empty line
        "GET / HTTP/1.1\r\n"                                            // no empty line, no headers
    )
);

TEST(Body, ParsesBodyWithValidContentLength) {
    string request = "POST / HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Content-Length: 4\r\n"
                     "\r\n"
                     "body";

    expected<Request, ParseError> r = parse_request(request);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->body, "body");
}

class ContentLengthValueTest : public ::testing::TestWithParam<string> {};

TEST_P(ContentLengthValueTest, ErrorOnInvalidContentLength) {
    string value = GetParam();

    string request = format("POST / HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Content-Length: {}\r\n"
                     "\r\n"
                     "body", value);

    expected<Request, ParseError> r = parse_request(request);
    EXPECT_FALSE(r.has_value());
}

INSTANTIATE_TEST_SUITE_P(
    Body,
    ContentLengthValueTest,
    ::testing::Values(
        "-2", "", "5"
    )
);

TEST(Body, MissingContentLengthSkipsBody) {
    string request = "POST / HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "\r\n"
                     "body\r\n";

    expected<Request, ParseError> r = parse_request(request);
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r->body.empty());
}

TEST(Body, TrimBodyWhenContentLengthShorter) {
    string request = "POST / HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Content-Length: 3\r\n"
                     "\r\n"
                     "body";

    expected<Request, ParseError> r = parse_request(request);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->body, "bod");
}

TEST(QueryParams, ParseSingleQueryParam) {
    string request = "POST /submit?param1=value1 HTTP/1.1\r\n"
                     "Host: example.com\r\n"
                     "Content-Length: 3\r\n"
                     "\r\n"
                     "body";

    expected<Request, ParseError> r = parse_request(request);

    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->query_params["param1"], "value1");
}

TEST(QueryParams, ParseMultipleQueryParams) {
    string request = "GET /search?param1=value1&param2=value2 HTTP/1.1\r\n"
                     "\r\n";

    expected<Request, ParseError> r = parse_request(request);

    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->path, "/search");
    EXPECT_EQ(r->query_params.size(), 2);
    EXPECT_EQ(r->query_params["param1"], "value1");
    EXPECT_EQ(r->query_params["param2"], "value2");
}

TEST(QueryParams, DecodesKeysAndValues) {
    string request = "GET /search?first%20name=a%26b HTTP/1.1\r\n"
                     "\r\n";

    expected<Request, ParseError> r = parse_request(request);

    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->query_params.size(), 1);
    EXPECT_EQ(r->query_params["first name"], "a&b");
}

using PlusCase = tuple<string, string, string>;

class QueryPlusTest : public ::testing::TestWithParam<PlusCase> {};

TEST_P(QueryPlusTest, DecodesPlusAsSpace) {
    auto [query, key, value] = GetParam();
    string request = "GET /search?" + query + " HTTP/1.1\r\n"
                                              "\r\n";

    expected<Request, ParseError> r = parse_request(request);

    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r->query_params.size(), 1);
    EXPECT_EQ(r->query_params[key], value);
}

INSTANTIATE_TEST_SUITE_P(
    QueryParams,
    QueryPlusTest,
    ::testing::Values(
        PlusCase{"q=hello+world", "q", "hello world"},     // plus in a value
        PlusCase{"first+name=Ada", "first name", "Ada"},   // plus in a key
        PlusCase{"q=a%2Bb", "q", "a+b"},                   // encoded plus stays literal
        PlusCase{"q=a+%2B+b", "q", "a + b"}                // both together
    )
);

using DecodeCase = tuple<string, string>;

class DecodePercentTest : public ::testing::TestWithParam<DecodeCase> {};

TEST_P(DecodePercentTest, DecodesEscapes) {
    auto [input, want] = GetParam();

    auto r = decode_percent(input);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, want);
}

INSTANTIATE_TEST_SUITE_P(
    QueryParams,
    DecodePercentTest,
    ::testing::Values(
        DecodeCase{"hello%20world", "hello world"},    // basic escape
        DecodeCase{"plain", "plain"},                  // no escapes
        DecodeCase{"", ""},                            // empty input
        DecodeCase{"%2f%2F", "//"},                    // lower- and upper-case hex
        DecodeCase{"a%26b%3Dc", "a&b=c"},              // reserved characters
        DecodeCase{"caf%C3%A9", "caf\xC3\xA9"},        // bytes above 127 (UTF-8 "café")
        DecodeCase{"100%25", "100%"},                  // literal percent
        DecodeCase{"%2541", "%41"},                    // decodes only once
        DecodeCase{"%41%42", "AB"}                     // back-to-back escapes
    )
);

class DecodePercentInvalidTest : public ::testing::TestWithParam<string> {};

TEST_P(DecodePercentInvalidTest, RejectsMalformedEscape) {
    auto r = decode_percent(GetParam());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error(), ParseError::BadRequest);
}

INSTANTIATE_TEST_SUITE_P(
    QueryParams,
    DecodePercentInvalidTest,
    ::testing::Values(
        "%",        // percent at the end
        "abc%",     // percent at the end after text
        "%4",       // one hex digit
        "%zz",      // non-hex digits
        "%4z"       // second digit not hex
    )
);
