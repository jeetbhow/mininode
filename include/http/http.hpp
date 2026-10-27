/**
 * @file http.hpp
 * @author jeetbhowm (jeetbhowm@domain.com)
 * @brief Helpful functions and definitions for reading/writing HTTP requests and responses.
 * @version 0.1
 * @date 2026-10-24
 */

#pragma once

#include "headers.hpp"

#include <expected>
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
     *   malformed, `Content-Length` is invalid, or the body is shorter than
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

    /**
     * @brief Decodes `%XX` escapes in a URL component, e.g. "a%20b" -> "a b".
     *
     * @param s The encoded component, such as a query parameter's key or value.
     *
     * @return The decoded bytes, or `ParseError::BadRequest` if an escape is
     *         truncated or isn't two hex digits.
     */
    std::expected<std::string, ParseError> decode_percent(std::string_view s);
}
