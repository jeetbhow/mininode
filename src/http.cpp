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
#include <cctype>
#include <charconv>
#include <cstdio>
#include <expected>
#include <ranges>

using namespace std;

namespace http {
    static bool case_insensitive_equals(string_view a, string_view b) {
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

    string_view to_string(Method method) {
        switch (method) {
            case Method::Get: return "GET";
            case Method::Head: return "HEAD";
            case Method::Post: return "POST";
            case Method::Put: return "PUT";
            case Method::Delete: return "DELETE";
            case Method::Patch: return "PATCH";
            case Method::Options: return "OPTIONS";
        }
        unreachable();
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

    // OWS = *( SP / HTAB ) (RFC 9110 §5.6.3)
    constexpr string_view ows = " \t";

    // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
    //         "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA (RFC 9110 §5.6.2)
    static bool is_tchar(char c) {
        return isalnum(static_cast<unsigned char>(c)) || "!#$%&'*+-.^_`|~"sv.contains(c);
    }

    static string_view trim(string_view s) {
        size_t first = s.find_first_not_of(ows);
        if (first == string_view::npos) {
            return "";
        }

        size_t last = s.find_last_not_of(ows);
        return s.substr(first, last - first + 1);
    }

    // `lines` is the header section without the empty line that ends it, so
    // every line, including the last, ends in CRLF.
    static expected<Headers, ParseError> parse_headers(string_view lines) {
        Headers headers;

        while (!lines.empty()) {
            size_t line_end = lines.find("\r\n");
            string_view line = lines.substr(0, line_end);

            // This removes the current line being processed from `lines` in place. A 
            // while loop ends when `lines` becomes empty. Since a `string_view` does
            // not take ownership, the underlying string is preserved.
            lines.remove_prefix(line_end + 2);

            size_t colon = line.find(':');
            if (colon == string_view::npos) {
                return unexpected(ParseError::BadRequest);
            }

            string_view name = line.substr(0, colon);
            string_view value = trim(line.substr(colon + 1));

            if (name.empty() || !ranges::all_of(name, is_tchar)) {
                return unexpected(ParseError::BadRequest);
            }

            // A bare CR or LF here means a line wasn't terminated by CRLF.
            if (value.find_first_of("\r\n\0"sv) != string_view::npos) {
                return unexpected(ParseError::BadRequest);
            }

            headers.add(string(name), string(value));
        }

        return headers;
    }

    static optional<size_t> parse_ulong(string_view s) {
        size_t result;
        const char* end = s.data() + s.size();
        auto [ptr, ec] = from_chars(s.data(), end, result);

        if (ec != errc{} || ptr != end) {
            return nullopt;
        }

        return result;
    }

    expected<string, ParseError> decode_percent(string_view s) {
        string result = "";
        size_t i = 0;
        while (i < s.size()) {
            if (s[i] != '%') {
                result += s[i];
                i++;
                continue;
            }

            if (i + 2 >= s.size()) {
                return unexpected(ParseError::BadRequest);
            }

            unsigned char byte = 0;
            const char* begin = s.data() + i + 1;
            const char* end = begin + 2;
            auto [ptr, ec] = from_chars(begin, end, byte, 16);

            if (ec != errc{} || ptr != end) {
                return unexpected(ParseError::BadRequest);
            }

            result += byte;
            i += 3;
        }
        return result;
    }

    static expected<string, ParseError> decode_query_component(string_view s) {
        // Form encoding sends spaces as '+'. Replace them before percent-decoding
        // so an encoded "%2B" survives as a literal '+'.
        string with_spaces(s);
        ranges::replace(with_spaces, '+', ' ');
        return decode_percent(with_spaces);
    }

    static expected<unordered_map<string, string>, ParseError> parse_query_params(string_view query) {
        unordered_map<string, string> result;

        for (auto part : query | views::split('&')) {
            string_view param(part.begin(), part.end());

            // "a=1&&b=2" and a trailing "&" leave empty pieces; skip them.
            if (param.empty()) {
                continue;
            }

            size_t equal = param.find('=');
            if (equal == string_view::npos) {
                return unexpected(ParseError::BadRequest);
            }

            expected<string, ParseError> key = decode_query_component(param.substr(0, equal));
            expected<string, ParseError> val = decode_query_component(param.substr(equal + 1));

            if (!key) {
                return unexpected(key.error());
            }

            if (!val) {
                return unexpected(val.error());
            }

            result[*key] = *val;
        }

        return result;
    }

    expected<Request, ParseError> parse_request(string_view request) {
        size_t request_line_end = request.find("\r\n");
        if (request_line_end == string_view::npos) {
            return unexpected(ParseError::BadRequest);
        }

        string_view request_line_sv = request.substr(0, request_line_end);

        vector<string_view> parts;
        for (auto part : request_line_sv | views::split(' ')) {
            parts.emplace_back(part.begin(), part.end());
        }

        // Parts are separated by exactly one space, so a doubled or leading
        // space shows up as an empty part.
        if (parts.size() != 3 || ranges::any_of(parts, [](string_view p) { return p.empty(); })) {
            return unexpected(ParseError::BadRequest);
        }

        string_view method_str = parts[0];
        string_view target = parts[1]; 
        string_view version_str = parts[2];

        // Check syntax before support, so "GET / garbage" is 400 rather than 505.
        if (!is_http_version(version_str)) {
            return unexpected(ParseError::BadRequest);
        }

        optional<Method> method = parse_method(method_str);
        if (!method) {
            return unexpected(ParseError::UnsupportedMethod);
        }

        optional<Version> version = parse_version(version_str);
        if (!version) {
            return unexpected(ParseError::UnsupportedVersion);
        }

        size_t question_mark = target.find('?');
        string_view path_sv = target.substr(0, question_mark);
        string_view query_sv = question_mark == string_view::npos ? "" : target.substr(question_mark + 1);

        expected<unordered_map<string, string>, ParseError> query_params = parse_query_params(query_sv);
        if (!query_params) {
            return unexpected(query_params.error());
        }

        Request r{};
        r.method = *method;
        r.path = path_sv;
        r.version = *version;
        r.query_params = *query_params;

        // The header section ends at the first empty line. Searching from the
        // request line's CRLF means a request with no headers ends there too.
        size_t headers_end = request.find("\r\n\r\n", request_line_end);
        if (headers_end == string_view::npos) {
            return unexpected(ParseError::BadRequest);
        }

        string_view header_lines = request.substr(request_line_end + 2, headers_end - request_line_end);

        expected<Headers, ParseError> headers = parse_headers(header_lines);
        if (!headers) {
            return unexpected(headers.error());
        }

        r.headers = std::move(*headers);

        optional<string> result = r.headers.get("Content-Length");
        if (!result) {
            return r;
        }

        optional<size_t> content_length = parse_ulong(*result);
        if (!content_length) {
            return unexpected(ParseError::BadRequest);
        }

        string_view body = request.substr(headers_end + 4, *content_length);
        if (body.size() != *content_length) {
            return unexpected(ParseError::BadRequest);
        }

        r.body = body;

        return r;
    }
}
