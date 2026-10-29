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

    void Route::remove_method(Method method) {
        auto it = ranges::find(methods, method);
        if (it != methods.end()) {
            methods.erase(it);
        }
    }

    void RouteNode::remove_child(string_view val) {
        auto it = ranges::find_if(children, [val](unique_ptr<RouteNode>& node) {
            return node->value == val;
        });

        if (it != children.end()) {
            children.erase(it);
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

    void Router::add(Method method, string_view path) {
        if (!path.starts_with('/')) {
            return;
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
                return;
            }
            curr->route->methods.push_back(method);
        } else {
            curr->route = Route{vector<Method>{ method }, string(path)};
        }

        ++size_;
    }

    bool Router::contains(Method method, string_view path) {
        if (!path.starts_with('/')) {
            return false;
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

    /**
     * @brief Remove a route from the router.
     *
     * If the path contains multiple methods, then only remove the designated one. Otherwise,
     * remove the entire node from the Trie.
     * 
     * @param method The method for the route.
     * @param path The path for the route.
     */
    void Router::remove(Method method, std::string_view path) {
        if (!path.starts_with('/')) {
            return;
        }

        path = path.substr(1);

        RouteNode* curr = &root_;
        RouteNode* prev = nullptr;
        for (const auto& subrange : path | views::split('/')) {
            string_view segment = string_view(subrange.begin(), subrange.end());
            
            RouteNode* child = curr->get_child(segment);
            if (!child) {
                return;
            }

            prev = curr;
            curr = child;
        }

        if (!curr->route || !curr->route->has_method(method)) {
            return;
        }

        curr->route->remove_method(method);        
        --size_;

        if (curr->route->methods.empty()) {
            curr->route = nullopt;
        }

        if (!curr->route && curr->children.empty()) {
            prev->remove_child(curr->value);
        }
    }

    bool Router::match(Method method, std::string_view target) {
        // TODO implement
        return false;
    }
}