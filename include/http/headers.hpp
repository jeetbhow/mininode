#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <optional>
#include <utility>

namespace http {
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
         * field. Use `add()` to insert one.
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
        void add(std::string name, std::string value);

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
}
