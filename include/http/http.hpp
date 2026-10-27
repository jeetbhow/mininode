#pragma once

#include "headers.hpp"

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace http {
    /**
     * @brief HTTP request methods supported by the server.
     *
     * Requests using any other method are rejected with 501 Not Implemented.
     */
    enum class Method {
        Get,        ///< Retrieve a resource.
        Head,       ///< Like GET, but the response carries no body.
        Post,       ///< Submit data to a resource.
        Put,        ///< Replace a resource.
        Delete,     ///< Remove a resource.
        Patch,      ///< Partially modify a resource.
        Options     ///< Describe the communication options for a resource.
    };

    /**
     * @brief Returns the method's name as it appears in a request line.
     *
     * @param method The method to convert.
     * @return The uppercase method name, e.g. "GET".
     */
    std::string_view to_string(Method method);

    /**
     * @brief HTTP protocol versions supported by the server.
     *
     * Requests using any other version are rejected with 505 HTTP Version Not Supported.
     */
    enum class Version {
        Http10,     ///< HTTP/1.0
        Http11      ///< HTTP/1.1
    };

    /**
     * @brief A parsed HTTP request.
     */
    struct Request {
        Request() = default;

        /// The header fields.
        Headers headers;

        /// The query parameters, e.g. {"sort": "asc"} for "?sort=asc".
        std::unordered_map<std::string, std::string> query_params;

        /// The request method, e.g. GET.
        Method method = Method::Get;

        /// The protocol version from the request line.
        Version version = Version::Http11;

        /// The request path without the query string, e.g. "/users/42".
        std::string path;

        /// The request body. Empty if the request has none.
        std::string body;

        /**
         * @brief Looks up a query parameter by name.
         *
         * Names are case-sensitive and compared after percent-decoding. If
         * the key appears more than once, the last value wins, e.g.
         * "?q=a&q=b" gives "b".
         *
         * @param name The parameter's decoded name, e.g. "sort".
         * @return The decoded value, or `std::nullopt` if there's no such
         *         parameter. A key with an empty value or no `=`, e.g. "?q="
         *         or "?q", gives "".
         */
        std::optional<std::string> query(const std::string& name) const;

        /**
         * @brief Formats the request as pretty-printed JSON, e.g. for logs and
         *        test output.
         *
         * Not HTTP wire format. Indented by two spaces, e.g.
         *
         *     {
         *       "method": "GET",
         *       "path": "/users/42",
         *       "version": "HTTP/1.1",
         *       "query": {
         *         "sort": "asc"
         *       },
         *       "headers": [
         *         ["Host", "example.com"]
         *       ],
         *       "body": "hello"
         *     }
         *
         * `headers` is an array of `[name, value]` pairs in the order they
         * appeared, so duplicate fields are kept. Strings are escaped with
         * `json_quote`, so the output is only valid JSON if the request's
         * strings are valid UTF-8.
         *
         * @return The request as a JSON object.
         */
        std::string to_string() const;
    };

    /**
     * @brief Reasons a request can fail to parse.
     */
    enum class ParseError {
        BadRequest,             ///< Malformed request. Respond with 400 Bad Request.
        UnsupportedMethod,      ///< Method isn't one of `Method`. Respond with 501 Not Implemented.
        UnsupportedVersion      ///< Version isn't one of `Version`. Respond with 505 HTTP Version Not Supported.
    };

    /**
     * @brief Parses a single HTTP request.
     *
     * Fails with:
     * - `ParseError::BadRequest` if the request line or a header line is
     *   malformed, a query parameter has a bad `%XX` escape,
     *   `Content-Length` is invalid, or the body is shorter than
     *   `Content-Length` says;
     * - `ParseError::UnsupportedMethod` if the method isn't one of `Method`;
     * - `ParseError::UnsupportedVersion` if the version isn't one of `Version`.
     *
     * @param request The raw bytes of one complete request: request line,
     *                header fields, and body.
     *
     * @return The parsed request, or the reason it couldn't be parsed.
     */
    std::expected<Request, ParseError> parse_request(std::string_view request);
}
