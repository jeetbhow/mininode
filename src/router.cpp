#include "http/router.hpp"

#include <algorithm>
#include <memory>
#include <ranges>

using namespace std;

namespace http {
    bool Route::has_method(Method method) {
        auto it = ranges::find(methods, method);
        
        if (it == methods.end()) {
            return false;
        } else {
            return true;
        }
    }

    RouteNode* RouteNode::get_child(string_view key) {
        auto it = ranges::find_if(children, [key](unique_ptr<RouteNode>& node) {
            return node->value == key;
        });

        if (it == children.end()) {
            return nullptr;
        }

        return it->get();
    }

    RouterError Router::add(Method method, string_view path) {
        if (!path.starts_with('/')) {
            return RouterError::InvalidRoute;
        }

        path = path.substr(1);

        RouteNode* curr = &root_;        
        for (auto part : path | ranges::views::split('/')) {
            string path_segment = string(part.begin(), part.end());
            
            RouteNode* next = curr->get_child(path_segment);
            if (!next) {
                next = curr->children.emplace_back(make_unique<RouteNode>(path_segment)).get();
            }

            curr = next;
        }

        if (curr->route) {
            if (curr->route->has_method(method)) {
                return RouterError::Ok;
            }
            curr->route->methods.push_back(method);
        } else {
            curr->route = Route{vector<Method>{ method }, string(path)};
        }

        ++size_;
        return RouterError::Ok;
    }

    expected<bool, RouterError> Router::contains(Method method, string_view path) {
        if (!path.starts_with('/')) {
            return unexpected(RouterError::InvalidRoute);
        }

        path = path.substr(1);

        RouteNode* curr = &root_;        
        for (auto part : path | ranges::views::split('/')) {
            string_view path_segment(part.begin(), part.end());
            
            RouteNode* next = curr->get_child(path_segment);
            if (!next) {
                return false;
            }

            curr = next;
        }

        return curr->route && curr->route->has_method(method);
    }

    void Router::remove(std::string_view route) {
        // TODO: Implement
    }

    expected<bool, RouterError> Router::match(Method method, std::string_view target) {
        // TODO implement
        return false;
    }
}