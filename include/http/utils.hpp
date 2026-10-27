#pragma once

#include <charconv>
#include <optional>
#include <string_view>
#include <system_error>
#include <utility>

namespace http {
    /**
     * @brief Optional whitespace: the characters HTTP allows around field
     *        values and other tokens.
     *
     * OWS = *( SP / HTAB ) (RFC 9110 §5.6.3)
     */
    inline constexpr std::string_view ows = " \t";

    /**
     * @brief Compares two strings, ignoring ASCII case.
     *
     * Only the letters A-Z and a-z are folded, so non-ASCII bytes must match
     * exactly. Strings of different lengths are never equal. Used for header
     * field names, which are case-insensitive (RFC 9110 §5.1).
     *
     * @param a The first string, e.g. "Content-Type".
     * @param b The second string, e.g. "content-type".
     * @return `true` if the strings are equal apart from ASCII case.
     */
    bool case_insensitive_equals(std::string_view a, std::string_view b);

    /**
     * @brief Removes leading and trailing whitespace.
     *
     * Only spaces and tabs (`ows`) are removed. CR, LF and other whitespace
     * are kept, so a stray line ending is still visible to the caller.
     *
     * @param s The string to trim, e.g. "  text/html\t".
     * @return A view into `s` without the surrounding spaces and tabs, e.g.
     *         "text/html", or an empty view if `s` is all spaces and tabs.
     *         The view refers to the same characters as `s`, so it must not
     *         outlive them.
     */
    std::string_view trim(std::string_view s);

    /**
     * @brief Splits a string at the first occurrence of a delimiter.
     *
     * Only the first `delim` splits; later ones stay in the second part, e.g.
     * splitting "a=b=c" at '=' gives "a" and "b=c". The delimiter itself is
     * in neither part. Either part may be empty, e.g. "=b" gives "" and "b".
     *
     * @param s     The string to split.
     * @param delim The character to split at.
     * @return The parts before and after `delim`, as views into `s`, or
     *         `std::nullopt` if `s` doesn't contain `delim`.
     */
    std::optional<std::pair<std::string_view, std::string_view>> split_once(std::string_view s, char delim);

    /**
     * @brief Parses an entire string as an integer.
     *
     * Stricter than `std::stoi` and friends: the whole of `s` must be digits,
     * so leading or trailing whitespace, a leading '+', and prefixes such as
     * "0x" are rejected. A leading '-' is accepted only when `T` is signed.
     * In bases above 10, letters may be upper- or lowercase, e.g. "ff" and
     * "FF" both parse as 255 in base 16.
     *
     * @tparam T The integer type to parse into, e.g. `size_t`.
     * @param s    The digits to parse, e.g. "42".
     * @param base The base, from 2 to 36. Defaults to 10.
     * @return The parsed value, or `std::nullopt` if `s` is empty, contains a
     *         character that isn't a digit in `base`, or the value doesn't
     *         fit in `T`.
     */
    template <typename T>
    std::optional<T> parse_number(std::string_view s, int base = 10) {
        T result{};
        const char* end = s.data() + s.size();
        auto [ptr, ec] = std::from_chars(s.data(), end, result, base);

        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }

        return result;
    }
}
