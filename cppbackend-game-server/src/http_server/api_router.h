#pragma once

#include <optional>
#include <functional>
#include <unordered_map>

#include "http_response.h"
#include "../game_app/token.h"

namespace http_handler {

    /**
     * @brief Context passed to all request handlers
     *
     * Contains the original request, authentication token (if present),
     * and any path parameters extracted from dynamic routes.
     */
    struct RequestContext {
        const StringRequest& req;               ///< Original HTTP request (read-only)
        std::optional<app::Token> token;        ///< Bearer token if provided in Authorization header
        std::unordered_map<std::string, std::string> path_params;       ///< Extracted from URL placeholders (e.g., {"id": "map1"})
    };

    /**
     * @brief Tree-based HTTP router with support for path parameters
     *
     * Features:
     * - O(L) lookup time (L = number of path segments), independent of total routes
     * - Supports static routes (exact match) and dynamic routes with :placeholders
     * - Automatic path parameter extraction into RequestContext
     * - Method validation, authentication checks, content-type verification
     *
     * Route patterns:
     * - Static:   "/api/v1/maps"           → exact match
     * - Dynamic:  "/api/v1/maps/:id"       → matches "/api/v1/maps/map1", extracts {"id": "map1"}
     *
     * The colon (:) is used only in route definitions; real requests contain plain slashes.
     */
    class ApiRouter {
    public:
        /// Handler function type: takes RequestContext and request data (body or query string)
        using HandlerFunc = std::function<StringResponse(
            const RequestContext&,
            const std::string_view /* body or query */
            )>;

        /**
         * @brief Configuration for a single endpoint
         *
         * Default values: requires_auth = false, content_type = EMPTY, auto_tick_enabled = false
         */
        struct EndpointConfig {
            std::vector<http::verb> allowed_methods;        ///< HTTP methods allowed (empty = any method)
            bool requires_auth = false;                     ///< If true, valid token must be present
            std::string content_type = std::string(ContentType::EMPTY);     ///< Required Content-Type header
            HandlerFunc handler;                            ///< Function to handle the request
            bool auto_tick_enabled = false;                 ///< Special flag for /api/v1/game/tick when auto-tick is active
        };

        /**
         * @brief Register a route with its configuration
         * @param path Route pattern (e.g., "/api/v1/maps/:id")
         * @param config Endpoint configuration including handler and constraints
         */
        void AddRoute(std::string_view path, EndpointConfig config);

        /**
         * @brief Route an incoming request to the appropriate handler
         * @param req The HTTP request
         * @param ctx Request context (token will be used; path_params will be filled)
         * @return Optional response if route found and all checks pass, nullopt otherwise
         *
         * Performs in order:
         * 1. Path matching (static or dynamic)
         * 2. HTTP method validation
         * 3. Authentication check if required
         * 4. Content-Type header validation if specified
         * 5. Auto-tick endpoint blocking if enabled
         * 6. Handler invocation with extracted path parameters
         */
        std::optional<StringResponse> Route(const StringRequest& req,
                                            RequestContext& ctx) const;

    private:
        /**
         * @brief Node in the routing tree
         *
         * Each node can have:
         * - Multiple static children (exact path segments)
         * - At most one parameter child (placeholder like :id)
         * - An EndpointConfig if this node represents a complete route
         */
        struct TreeNode {
            std::unordered_map<std::string, std::unique_ptr<TreeNode>> children;  ///< Static segment children
            std::unique_ptr<TreeNode> param_child;                                 ///< Parameter placeholder child (e.g., :id)
            std::string param_name;                                                ///< Name of the parameter (without colon)
            EndpointConfig config;                                                  ///< Only set at leaf nodes (complete routes)
        };

        std::unique_ptr<TreeNode> root_ = std::make_unique<TreeNode>();     ///< Root of the routing tree

        /**
         * @brief Split a path into segments
         * @param path Full URL path (e.g., "/api/v1/maps/map1")
         * @return Vector of segments ["api", "v1", "maps", "map1"]
         *
         * Leading slash is removed, empty segments are skipped.
         */
        static std::vector<std::string> SplitPath(std::string_view path);

        /**
         * @brief Recursively insert a route into the tree
         * @param segments Path segments from SplitPath
         * @param config Endpoint configuration to store at leaf
         * @param idx Current segment index
         * @param node Current tree node
         * @return true if insertion successful
         */
        bool InsertRoute(const std::vector<std::string>& segments,
                         const EndpointConfig& config,
                         size_t idx, TreeNode* node);

        /**
         * @brief Recursively match a request path against the tree
         * @param segments Request path segments
         * @param params Output map for extracted path parameters
         * @param config Output pointer to matched endpoint configuration
         * @param idx Current segment index
         * @param node Current tree node
         * @return true if a matching route was found
         *
         * Matching strategy:
         * 1. Try exact static segment match first
         * 2. If that fails, try parameter placeholder (if exists)
         * 3. Backtrack if deeper matching fails
         */
        bool MatchRoute(const std::vector<std::string>& segments,
                        std::unordered_map<std::string, std::string>& params,
                        const EndpointConfig*& config,
                        size_t idx, TreeNode* node) const;
    };

} // namespace http_handler
