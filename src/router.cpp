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
        // Paths are absolute and have no empty segments ("/a//b", "/a/").
        if (!path.starts_with('/') || path.find("//") != string_view::npos
            || (path.size() > 1 && path.ends_with('/'))) {
            return;
        }

        if (!root_) {
            root_ = make_unique<RouteNode>("/");
        }

        RouteNode* node = find(path, true);
        if (!node->route) {
            node->route = Route{{}, string(path)};
        }

        if (!node->route->has_method(method)) {
            node->route->methods.push_back(method);
            size_++;
        }
    }

    bool Router::contains(Method method, string_view path) {
        if (!path.starts_with('/') || path.find("//") != string_view::npos
            || (path.size() > 1 && path.ends_with('/')) || !root_) {
            return false;
        }

        // find() stops at the last node it reached, so confirm it is the requested path.
        RouteNode* node = find(path);
        return node->route
            && node->route->path == path
            && node->route->has_method(method);
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
        if (!contains(method, path)) {
            return;
        }

        RouteNode* node = find(path);
        node->route->remove_method(method);
        size_--;

        if (!node->route->methods.empty()) {
            return;
        }
        
        node->route.reset();

        // Prune nodes that no longer lead to a route, walking up towards the root.
        while (node->parent && !node->route && node->children.empty()) {
            RouteNode* parent = node->parent;
            parent->remove_child(node->value);
            node = parent;
        }
    }
    
    bool Router::match(Method method, string_view target) {
        if (!target.starts_with('/') || target.find("//") != string_view::npos
            || (target.size() > 1 && target.ends_with('/')) || !root_) {
            return false;
        }

        RouteNode* result = match(root_.get(), target.substr(1));
        return result && result->route && result->route->has_method(method);
    }

    RouteNode* Router::match(RouteNode* node, string_view target) {
        if (!node) {
            return nullptr;
        }

        if (target.empty()) {
            return node;
        }

        size_t pos = target.find('/');

        string_view path_segment = target.substr(0, pos);
        RouteNode* child = node->get_child(path_segment);
        string_view remain = pos == string_view::npos ? string_view{} : target.substr(pos + 1);

        RouteNode* result = match(child, remain);
        return result;
    }


    RouteNode* Router::find(string_view path, bool insert) {
        path = path.substr(1);
    
        RouteNode* curr = root_.get();

        for (auto part : path | ranges::views::split('/')) {
            string_view path_segment(part.begin(), part.end());
            if (path_segment.empty()) {
                continue;
            }

            RouteNode* next = curr->get_child(path_segment);
            if (!next) {
                if (!insert) {
                    return curr;
                }

                next = curr->children
                    .emplace_back(make_unique<RouteNode>(string(path_segment), curr))
                    .get();
            }
            curr = next;
        }

        return curr;
    }
}