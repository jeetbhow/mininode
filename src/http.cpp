/**
 * @file http.cpp
 * @author jeetbhowm (jeetbhowm@domain.com)
 * @brief Helpful functions and definitions for creating HTTP requests and responses.
 * @version 0.1
 * @date 2026-10-24
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "http/http.hpp"

#include <algorithm>
#include <cstdio>
#include <ranges>

using namespace std;

namespace http {
    static bool case_insensitive_equals(std::string_view a, std::string_view b) {
        return ranges::equal(a, b, [](char x, char y) {
            return tolower(static_cast<unsigned char>(x)) == tolower(static_cast<unsigned char>(y));
        });
    }

    optional<string> Headers::get(string_view name) const {
        optional<string> result;
        
        for (const auto& [key, val] : fields) {
            if (!case_insensitive_equals(key, name)) {
                continue;
            }
            
            if (result) {
                *result += ", ";
            } else {
                result = "";
            }

            *result += val;
        }

        return result;
    }

    vector<string> Headers::get_all(string_view name) const {
        vector<string> result;

        for (const auto& [key, val] : fields) {
            if (!case_insensitive_equals(key, name)) {
                continue;
            }
            
            result.push_back(val);
        }

        return result;
    }

    void Headers::add(string name, string value) {
        fields.emplace_back(name, value);
    }

    void Headers::remove(string_view name) {
        erase_if(fields, [name](const pair<string, string>& p) {
            return case_insensitive_equals(name, p.first);
        });
    }

    static optional<Method> parse_method(string_view s) {
        // Methods are case-sensitive (RFC 9110 §9.1), so "get" is not GET.
        if (s == "GET") return Method::Get;
        if (s == "HEAD") return Method::Head;
        if (s == "POST") return Method::Post;
        if (s == "PUT") return Method::Put;
        if (s == "DELETE") return Method::Delete;
        if (s == "PATCH") return Method::Patch;
        if (s == "OPTIONS") return Method::Options;
        return nullopt;
    }

    // HTTP-version = "HTTP/" DIGIT "." DIGIT
    static bool is_http_version(string_view s) {
        auto is_digit = [](char c) { return c >= '0' && c <= '9'; };
        return s.size() == 8 && s.starts_with("HTTP/") && is_digit(s[5]) && s[6] == '.' && is_digit(s[7]);
    }

    static optional<Version> parse_version(string_view s) {
        if (s == "HTTP/1.0") return Version::Http10;
        if (s == "HTTP/1.1") return Version::Http11;
        return nullopt;
    }

    expected<Request, ParseError> parse_request(string_view request) {
        // <method> <target> <version>\r\n
        auto request_line_end = request.find("\r\n");
        if (request_line_end == string_view::npos) {
            return std::unexpected(ParseError::BadRequest);
        }

        string_view request_line = request.substr(0, request_line_end);

        vector<string_view> parts;

        for (auto part : request_line | views::split(' ')) {
            parts.emplace_back(part.begin(), part.end());
        }

        // Parts are separated by exactly one space, so a doubled or leading
        // space shows up as an empty part.
        if (parts.size() != 3 || ranges::any_of(parts, [](string_view p) { return p.empty(); })) {
            return std::unexpected(ParseError::BadRequest);
        }

        string_view method_str = parts[0];
        string_view target = parts[1]; 
        string_view version_str = parts[2];

        // Check syntax before support, so "GET / garbage" is 400 rather than 505.
        if (!is_http_version(version_str)) {
            return std::unexpected(ParseError::BadRequest);
        }

        auto method = parse_method(method_str);
        if (!method) {
            return std::unexpected(ParseError::UnsupportedMethod);
        }

        auto version = parse_version(version_str);
        if (!version) {
            return std::unexpected(ParseError::UnsupportedVersion);
        }

        Request r{};
        r.method = *method;
        r.target = string(target);
        r.version = *version;

        // TODO: parse header fields and body.

        return r;
    }
}
