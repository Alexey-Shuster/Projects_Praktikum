#include "api_router.h"
#include "../common/utils.h"

namespace http_handler {

    /**
     * @brief Register a new route in the trie
     *
     * Splits the path into segments and inserts them recursively.
     * If any segment starts with ':', it's treated as a parameter placeholder.
     *
     * Example: "/api/v1/maps/:id" becomes segments ["api", "v1", "maps", ":id"]
     */
    void ApiRouter::AddRoute(std::string_view path, EndpointConfig config)
    {
        auto segments = SplitPath(path);
        InsertRoute(segments, config, 0, root_.get());
    }

    /**
     * @brief Main routing logic for incoming requests
     *
     * Steps:
     * 1. Extract path without query parameters
     * 2. Split path into segments
     * 3. Match against trie, collecting parameters if any
     * 4. Validate HTTP method against allowed methods
     * 5. Check authentication if required
     * 6. Verify Content-Type header if specified
     * 7. Block /api/v1/game/tick if auto-tick is enabled
     * 8. Execute handler with extracted parameters and request data
     */
    std::optional<StringResponse> ApiRouter::Route(
        const StringRequest& req,
        RequestContext& ctx) const
    {
        // Step 1: Extract clean path (remove query string)
        std::string target = std::string(req.target());
        auto query_pos = target.find('?');
        std::string path = (query_pos != std::string::npos)
                               ? target.substr(0, query_pos)
                               : target;

        // Step 2-3: Match route and collect parameters
        auto segments = SplitPath(path);
        const EndpointConfig* config = nullptr;
        std::unordered_map<std::string, std::string> params;

        if (!MatchRoute(segments, params, config, 0, root_.get()))
            return std::nullopt;

        // Step 4: Validate HTTP method
        if (!config->allowed_methods.empty()) {
            auto it = std::find(config->allowed_methods.begin(),
                                config->allowed_methods.end(),
                                req.method());
            if (it == config->allowed_methods.end()) {
                std::string allowed = utils::http::MethodsToString(config->allowed_methods);
                return response::Builder::Modify(response::MethodNotAllowed(req))
                                                .WithAllow(allowed)
                                                .Build();
            }
        }

        // Step 5: Check authentication requirement
        if (config->requires_auth && !ctx.token) {
            return response::Builder::MakeError(req, http::status::unauthorized,
                                                error_codes::INVALID_TOKEN,
                                                error_messages::INVALID_TOKEN);
        }

        // Step 6: Validate Content-Type if specified
        if (!config->content_type.empty()) {
            auto content_type = req.find(http::field::content_type);
            if (content_type == req.end() ||
                std::string(content_type->value()) != config->content_type) {
                return response::Builder::MakeError(req, http::status::bad_request,
                                                    error_codes::INVALID_ARGUMENT,
                                                    error_messages::INVALID_CONTENT_TYPE);
            }
        }

        // Step 7: Block auto-tick endpoint if auto-tick is enabled
        if (config->auto_tick_enabled) {
            return response::Builder::MakeError(req, http::status::bad_request,
                                                error_codes::BAD_REQUEST,
                                                error_messages::INVALID_ENDPOINT);
        }

        // Step 8: Prepare for handler execution
        // Move extracted parameters into the original context
        ctx.path_params = std::move(params);

        // Determine request data: either query string or request body
        std::string_view data = (query_pos != std::string::npos)
                                    ? std::string_view(target).substr(query_pos + 1)
                                    : req.body();

        // Invoke the handler
        return config->handler(ctx, data);
    }

    /**
     * @brief Split URL path into segments
     *
     * Examples:
     * - "/api/v1/maps"      → ["api", "v1", "maps"]
     * - "api/v1/maps/"      → ["api", "v1", "maps"]
     * - "/"                 → [] (empty vector)
     */
    std::vector<std::string> ApiRouter::SplitPath(std::string_view path)
    {
        std::vector<std::string> tokens;
        std::string_view remaining = path;

        // Remove leading slash if present
        if (!remaining.empty() && remaining.starts_with(api_paths::SLASH))
            remaining.remove_prefix(1);

        // Split by '/'
        while (!remaining.empty()) {
            auto pos = remaining.find(api_paths::SLASH);
            if (pos == std::string_view::npos) {
                tokens.emplace_back(remaining);
                break;
            }
            tokens.emplace_back(remaining.substr(0, pos));
            remaining.remove_prefix(pos + 1);
        }
        return tokens;
    }

    /**
     * @brief Recursively insert route segments into the trie
     *
     * Algorithm:
     * - At leaf (idx == segments.size()): store the config
     * - For current segment:
     *   - If it starts with ':' → create/use param_child
     *   - Otherwise → create/use static child in children map
     */
    bool ApiRouter::InsertRoute(const std::vector<std::string>& segments,
                            const EndpointConfig& config,
                            size_t idx, TreeNode* node)
    {
        // Base case: reached the end of the path
        if (idx == segments.size()) {
            node->config = std::move(config);
            return true;
        }

        const std::string& seg = segments[idx];

        // Parameter placeholder (starts with ':')
        if (!seg.empty() && seg.starts_with(api_paths::COLON)) {
            if (!node->param_child) {
                node->param_child = std::make_unique<TreeNode>();
                node->param_name = seg.substr(1); // store name without ':'
            }
            return InsertRoute(segments, config, idx + 1, node->param_child.get());
        }
        // Static segment
        auto& child = node->children[seg];
        if (!child)
            child = std::make_unique<TreeNode>();
        return InsertRoute(segments, config, idx + 1, child.get());
    }

    /**
     * @brief Recursively match request segments against the trie
     *
     * Matching priority:
     * 1. Try exact static match first (most specific)
     * 2. If that fails, try parameter placeholder (if exists)
     * 3. Backtrack if a chosen path fails later
     *
     * This ensures that static routes take precedence over parameterized ones
     * when both could match (e.g., "/api/v1/maps" vs "/api/v1/maps/:id").
     */
    bool ApiRouter::MatchRoute(const std::vector<std::string>& segments,
                           std::unordered_map<std::string, std::string>& params,
                           const EndpointConfig*& config,
                           size_t idx, TreeNode* node) const
    {
        // Base case: reached the end of the request path
        if (idx == segments.size()) {
            if (node->config.handler) {     // Valid endpoint
                config = &node->config;
                return true;
            }
            return false;       // Path ended but no handler registered
        }

        const std::string& seg = segments[idx];

        // 1. Try static child first (most specific)
        auto it = node->children.find(seg);
        if (it != node->children.end()) {
            if (MatchRoute(segments, params, config, idx + 1, it->second.get()))
                return true;
        }

        // 2. Try parameter child (if static didn't match or failed deeper)
        if (node->param_child) {
            params[node->param_name] = seg;
            if (MatchRoute(segments, params, config, idx + 1, node->param_child.get()))
                return true;
            // Backtrack: remove parameter if this path ultimately fails
            params.erase(node->param_name);
        }

        return false;   // No match found
    }

} // namespace http_handler
