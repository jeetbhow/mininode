#include "http/http.hpp"
#include "http/utils.hpp"

#include <algorithm>
#include <cctype>
#include <expected>
#include <ranges>
#include <utility>

using namespace std;

namespace http {
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

    optional<string> Request::query(const string& name) const {
        auto it = query_params.find(name);

        if (it == query_params.end()) {
            return nullopt;
        }

        return it->second;
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

    // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
    //         "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA (RFC 9110 §5.6.2)
    static bool is_tchar(char c) {
        return isalnum(static_cast<unsigned char>(c)) || "!#$%&'*+-.^_`|~"sv.contains(c);
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

            optional<pair<string_view, string_view>> field = split_once(line, ':');
            if (!field) {
                return unexpected(ParseError::BadRequest);
            }

            string_view name = field->first;
            string_view value = trim(field->second);

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

    // Decodes `%XX` escapes in a URL component, e.g. "a%20b" -> "a b". Fails
    // with BadRequest if an escape is truncated or isn't two hex digits.
    static expected<string, ParseError> decode_percent(string_view s) {
        string result;
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

            optional<unsigned char> byte = parse_number<unsigned char>(s.substr(i + 1, 2), 16);
            if (!byte) {
                return unexpected(ParseError::BadRequest);
            }

            result += *byte;
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

            // A key without '=' (e.g. "?flag") has an empty value, as in Express.
            auto [raw_key, raw_val] = split_once(param, '=').value_or(pair{param, ""sv});

            expected<string, ParseError> key = decode_query_component(raw_key);
            expected<string, ParseError> val = decode_query_component(raw_val);

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

    namespace {
        // A request line with its method and version parsed.
        struct RequestLine {
            Method method;
            string_view target;
            Version version;
        };

        // A request target split into its path and decoded query parameters.
        struct Target {
            string_view path;
            unordered_map<string, string> query_params;
        };
    }

    static expected<RequestLine, ParseError> parse_request_line(string_view line) {
        vector<string_view> parts;
        for (auto part : line | views::split(' ')) {
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

        return RequestLine{*method, target, *version};
    }

    static expected<Target, ParseError> parse_target(string_view target) {
        // A target without a '?' is all path and has an empty query.
        string_view path = target;
        string_view query;
        if (optional<pair<string_view, string_view>> split = split_once(target, '?')) {
            path = split->first;
            query = split->second;
        }

        expected<unordered_map<string, string>, ParseError> query_params = parse_query_params(query);
        if (!query_params) {
            return unexpected(query_params.error());
        }

        return Target{path, std::move(*query_params)};
    }

    // `rest` is everything after the empty line that ends the header section.
    static expected<string, ParseError> parse_body(string_view rest, const Headers& headers) {
        optional<string> content_length_header = headers.get("Content-Length");
        if (!content_length_header) {
            return string();
        }

        optional<size_t> content_length = parse_number<size_t>(*content_length_header);
        if (!content_length) {
            return unexpected(ParseError::BadRequest);
        }

        string_view body = rest.substr(0, *content_length);
        if (body.size() != *content_length) {
            return unexpected(ParseError::BadRequest);
        }

        return string(body);
    }

    expected<Request, ParseError> parse_request(string_view request) {
        size_t request_line_end = request.find("\r\n");
        if (request_line_end == string_view::npos) {
            return unexpected(ParseError::BadRequest);
        }

        expected<RequestLine, ParseError> line = parse_request_line(request.substr(0, request_line_end));
        if (!line) {
            return unexpected(line.error());
        }

        expected<Target, ParseError> target = parse_target(line->target);
        if (!target) {
            return unexpected(target.error());
        }

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

        expected<string, ParseError> body = parse_body(request.substr(headers_end + 4), *headers);
        if (!body) {
            return unexpected(body.error());
        }

        Request r{};
        r.method = line->method;
        r.version = line->version;
        r.path = target->path;
        r.query_params = std::move(target->query_params);
        r.headers = std::move(*headers);
        r.body = std::move(*body);

        return r;
    }
}
