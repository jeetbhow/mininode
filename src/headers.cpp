#include "http/headers.hpp"
#include "http/utils.hpp"

#include <ranges>
#include <string>
#include <string_view>

using namespace std;

namespace http {
    optional<string> Headers::get(string_view name) const {
        vector<string> values = get_all(name);
        if (values.empty()) {
            return nullopt;
        }

        return ranges::to<string>(values | views::join_with(", "sv));
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
        fields.emplace_back(std::move(name), std::move(value));
    }

    void Headers::remove(string_view name) {
        erase_if(fields, [name](const pair<string, string>& p) {
            return case_insensitive_equals(name, p.first);
        });
    }
}
