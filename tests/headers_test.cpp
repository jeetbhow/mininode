#include "http/http.hpp"

#include <gtest/gtest.h>

using namespace std;
using namespace http;

TEST(Headers, NulloptOnNonExistentHeader) {
    Headers h; 

    h.add("Host", "example.com");

    auto get_result = h.get("Invalid");
    vector<string> get_all_result = h.get_all("Invalid");
    
    EXPECT_FALSE(get_result.has_value());
    EXPECT_TRUE(get_all_result.empty());
}

TEST(Headers, DuplicatesReturnCommaDelimitedList) {
    Headers h; 

    h.add("Accept", "application/json");
    h.add("Accept", "text/html");

    auto get_result = h.get("Accept");
    vector<string> get_all_result = h.get_all("Accept");
    
    ASSERT_TRUE(get_result.has_value());
    ASSERT_TRUE(get_all_result.size() == 2);
    EXPECT_EQ(*get_result, "application/json, text/html");
    EXPECT_EQ(get_all_result, (vector<string>{"application/json", "text/html"}));
}

// Field names are case-insensitive (RFC 9110 §5.1).
using CaseInsensitiveCase = tuple<string, string>;

class CaseInsensitiveLookupTest : public ::testing::TestWithParam<CaseInsensitiveCase> {};

TEST_P(CaseInsensitiveLookupTest, MatchesRegardlessOfCase) {
    auto [stored, lookup] = GetParam();
    Headers h;

    h.add(stored, "text/html");

    auto get_result = h.get(lookup);
    vector<string> get_all_result = h.get_all(lookup);

    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(*get_result, "text/html");
    EXPECT_EQ(get_all_result, (vector<string>{"text/html"}));
}

TEST_P(CaseInsensitiveLookupTest, RemovesRegardlessOfCase) {
    auto [stored, lookup] = GetParam();
    Headers h;

    h.add(stored, "text/html");
    h.remove(lookup);

    EXPECT_TRUE(h.fields.empty());
}

INSTANTIATE_TEST_SUITE_P(
    Headers,
    CaseInsensitiveLookupTest,
    ::testing::Values(
        CaseInsensitiveCase{"Content-Type", "Content-Type"},
        CaseInsensitiveCase{"Content-Type", "content-type"},
        CaseInsensitiveCase{"Content-Type", "CONTENT-TYPE"},
        CaseInsensitiveCase{"Content-Type", "cOnTeNt-TyPe"},
        CaseInsensitiveCase{"content-type", "Content-Type"},
        CaseInsensitiveCase{"CONTENT-TYPE", "content-type"}
    )
);

TEST(Headers, RemovingNonExistentHeaderNoOps) {
    Headers h;
    
    h.add("Host", "example.com");
    h.remove("Invalid");

    ASSERT_TRUE(h.fields.size() == 1);
    EXPECT_EQ(h.fields[0].first, "Host");
    EXPECT_EQ(h.fields[0].second, "example.com");
}

TEST(Headers, RemovesDuplicates) {
    Headers h;
    
    h.add("Host", "example.com");
    h.add("Accept", "application/json");
    h.add("Accept", "text/html");

    ASSERT_TRUE(h.fields.size() == 3);

    h.remove("Accept");

    ASSERT_TRUE(h.fields.size() == 1);
    EXPECT_EQ(h.fields[0].first, "Host");
    EXPECT_EQ(h.fields[0].second, "example.com");
}

TEST(Headers, RemovePreservesOrderOfRemainingFields) {
    Headers h;

    h.add("Host", "example.com");
    h.add("Accept", "application/json");
    h.add("User-Agent", "curl/8.0");
    h.add("Accept", "text/html");
    h.add("Connection", "close");

    h.remove("Accept");

    ASSERT_TRUE(h.fields.size() == 3);
    EXPECT_EQ(h.fields[0].first, "Host");
    EXPECT_EQ(h.fields[1].first, "User-Agent");
    EXPECT_EQ(h.fields[2].first, "Connection");
}

TEST(Headers, InterleavedDuplicatesJoinInOrder) {
    Headers h;

    h.add("Accept", "application/json");
    h.add("Host", "example.com");
    h.add("Accept", "text/html");

    auto get_result = h.get("Accept");
    vector<string> get_all_result = h.get_all("Accept");

    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(*get_result, "application/json, text/html");
    EXPECT_EQ(get_all_result, (vector<string>{"application/json", "text/html"}));
}

TEST(Headers, EmptyValueIsDistinctFromMissing) {
    Headers h;

    h.add("X-Empty", "");

    auto get_result = h.get("X-Empty");
    vector<string> get_all_result = h.get_all("X-Empty");

    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(*get_result, "");
    EXPECT_EQ(get_all_result, (vector<string>{""}));
}

TEST(Headers, PrefixOfNameDoesNotMatch) {
    Headers h;

    h.add("Content-Type", "text/html");

    EXPECT_FALSE(h.get("Content").has_value());
    EXPECT_FALSE(h.get("Content-Type-Extra").has_value());
    EXPECT_TRUE(h.get_all("Content").empty());
    EXPECT_TRUE(h.get_all("Content-Type-Extra").empty());
}

TEST(Headers, NulloptOnEmptyHeaders) {
    Headers h;

    EXPECT_FALSE(h.get("Host").has_value());
    EXPECT_TRUE(h.get_all("Host").empty());
}

TEST(Headers, RemoveThenAddReplacesValue) {
    Headers h;

    h.add("Content-Type", "text/plain");
    h.remove("Content-Type");
    h.add("Content-Type", "application/json");

    auto get_result = h.get("Content-Type");

    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(*get_result, "application/json");
    EXPECT_TRUE(h.fields.size() == 1);
}

TEST(Headers, SubscriptMatchesGet) {
    Headers h;

    h.add("Accept", "application/json");
    h.add("Accept", "text/html");

    EXPECT_EQ(h["Accept"], h.get("Accept"));
    EXPECT_EQ(h["Invalid"], h.get("Invalid"));
}