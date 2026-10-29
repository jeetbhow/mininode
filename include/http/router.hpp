#pragma once

#include "http/http.hpp"

#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <optional>

/**
 * GET /hello/:param/world
 * GET /hello/my/world
 */

// Test fixture that is allowed to inspect the router's trie.
class RouterTest;

namespace http {
    struct Route {
        std::vector<Method> methods;
        std::string path;

        // Return true if the route has the method associated with it.
        bool has_method(Method method);

        // Remove a method from the route.
        void remove_method(Method method);
    };

    /**
     * @brief A single node in the router's trie.
     * 
     */
    struct RouteNode {
        RouteNode() = default;
        RouteNode(const std::string& v, RouteNode* p = nullptr) : value(v), parent(p) {};

        std::string value;

        // The node this one hangs off of. Null for the root.
        RouteNode* parent = nullptr;
        std::vector<std::unique_ptr<RouteNode>> children{};
        std::optional<Route> route;
        
        // Get a child node.
        RouteNode* get_child(std::string_view key);

        // Remove a child node
        void remove_child(std::string_view val);
    };

    /**
     * @brief Matches targets to routes. 
     * 
     */
    class Router {
    public:
        Router() = default;

        // Add a route. Invalid paths are ignored.
        void add(Method method, std::string_view path);

        // Remove a route if it exists. If the path is invalid then reject it.
        void remove(Method method, std::string_view path);

        // Check if a route exists. Invalid paths return false.
        bool contains(Method method, std::string_view path);

        // Match a target against a route. Invalid targets return false.
        bool match(Method method, std::string_view target);

        // Return the number of routes.
        size_t size() { return size_; }

    private:
        friend class ::RouterTest;

        RouteNode* find(std::string_view path, bool insert = false);

        /// The root of the trie. All potential routes branch off of this node.
        std::unique_ptr<RouteNode> root_;
        size_t size_ = 0;
    };
}
