#include "http/utils.hpp"

#include <algorithm>
#include <cctype>

using namespace std;

namespace http {
    bool case_insensitive_equals(string_view a, string_view b) {
        return ranges::equal(a, b, [](char x, char y) {
            return tolower(static_cast<unsigned char>(x)) == tolower(static_cast<unsigned char>(y));
        });
    }

    string_view trim(string_view s) {
        size_t first = s.find_first_not_of(ows);
        if (first == string_view::npos) {
            return "";
        }

        size_t last = s.find_last_not_of(ows);
        return s.substr(first, last - first + 1);
    }

    optional<pair<string_view, string_view>> split_once(string_view s, char delim) {
        size_t pos = s.find(delim);
        if (pos == string_view::npos) {
            return nullopt;
        }

        return pair{s.substr(0, pos), s.substr(pos + 1)};
    }
}
