/**
 * @file http.hpp
 * @author jeetbhowm (jeetbhowm@domain.com)
 * @brief Helpful functions and definitions for creating HTTP requests and responses.
 * @version 0.1
 * @date 2026-10-24
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 #pragma once

 #include <string>
 #include <vector>

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
     * Requests using any other version are rejected with
     * 505 HTTP Version Not Supported.
     */
    enum class Version {
        Http10,     ///< HTTP/1.0
        Http11      ///< HTTP/1.1
    };

    /**
     * @brief A parsed HTTP request.
     */
    struct Request {
        /**
         * @brief Header fields, in the order they appeared in the request.
         *
         * Stored as a `std::vector` rather than an `std::unordered_map` because headers can have duplicates.
         */
        std::vector<std::string> headers;

        /// The request method, e.g. GET.
        Method method;

        /// The protocol version from the request line.
        Version version;

        /// The raw request target, e.g. "/users/42?sort=asc".
        std::string target;

        /// The request body. Empty if the request has none.
        std::string body;
    };

    /**
     * @brief Parses a single HTTP request.
     *
     * @param request The raw bytes of one complete request: request line,
     *                header fields, and body.
     * @return The parsed request.
     */
    Request parse_request(const std::string& request);
}
