/**
 * @file http.hpp
 * @author jeetbhowm (jeetbhowm@domain.com)
 * @brief Helpful functions and definitions for reading/writing HTTP requests and responses.
 * @version 0.1
 * @date 2026-10-24
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <optional>

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
     * @brief HTTP protocol versions supported by the server.
     *
     * Requests using any other version are rejected with 505 HTTP Version Not Supported.
     */
    enum class Version {
        Http10,     ///< HTTP/1.0
        Http11      ///< HTTP/1.1
    };

    struct Headers {
        /**
         * @brief Header fields, in the order they appeared in the request.
         *
         * Stored as a `std::vector` rather than an `std::unordered_map` because headers can have duplicates.
         */
        std::vector<std::pair<std::string, std::string>> fields;

        /**
         * @brief Looks up the value of a header field.
         *
         * Names are compared case-insensitively. If the field appears more
         * than once, the values are joined with ", " in the order they
         * appeared, e.g. `Accept: a` and `Accept: b` give "a, b"
         * (RFC 9110 §5.3).
         *
         * Joining is wrong for fields whose values may contain commas, such
         * as `Set-Cookie`; use `get_all()` for those.
         *
         * @param name The field name, e.g. "Content-Type".
         * @return The field's value, or `std::nullopt` if no field has that name.
         */
        std::optional<std::string> get(std::string_view name) const;

        /**
         * @brief Looks up the value of a header field. Same as `get()`.
         *
         * Read-only: unlike `std::map::operator[]`, this never inserts a
         * field. Use `put()` to add one.
         *
         * @param name The field name, e.g. "Content-Type".
         * @return The field's value, or `std::nullopt` if no field has that name.
         */
        std::optional<std::string> operator[](std::string_view name) const { return get(name); }

        /**
         * @brief Looks up every value of a header field.
         *
         * Names are compared case-insensitively. Each occurrence is returned
         * as a separate value, without joining.
         *
         * @param name The field name, e.g. "Set-Cookie".
         * @return The field's values in the order they appeared, or an empty
         *         vector if no field has that name.
         */
        std::vector<std::string> get_all(std::string_view name) const;

        /**
         * @brief Appends a header field.
         *
         * Existing fields with the same name are kept, so this can add
         * duplicates (e.g. multiple `Set-Cookie` fields). To replace a field,
         * call `remove()` first.
         *
         * @param name  The field name.
         * @param value The field value.
         */
        void put(std::string name, std::string value);

        /**
         * @brief Removes every header field with the given name.
         *
         * Names are compared case-insensitively. Does nothing if no field has
         * that name.
         *
         * @param name The field name to remove.
         */
        void remove(std::string_view name);
    };

    /**
     * @brief A parsed HTTP request.
     */
    struct Request {
        Request() = default;

        /// The header fields.
        Headers headers;

        /// The request method, e.g. GET.
        Method method = Method::Get;

        /// The protocol version from the request line.
        Version version = Version::Http11;

        /// The raw request target, e.g. "/users/42?sort=asc".
        std::string target;

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
}
