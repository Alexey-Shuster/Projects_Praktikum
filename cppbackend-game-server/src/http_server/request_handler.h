#pragma once

#include <filesystem>
#include <memory>

#include "api_handler.h"
#include "../game_app/application.h"
#include "http_response.h"
#include "../common/ticker.h"

namespace http_handler {

/**
 * @brief Main HTTP request handler that dispatches requests to appropriate sub-handlers
 *
 * This class is the entry point for all HTTP requests to the game server:
 * 1. Determines whether a request is for the API or static files
 * 2. Routes API requests through ApiHandler with strand synchronization
 * 3. Serves static files directly from the filesystem
 * 4. Manages the game ticker for automatic game state updates
 *
 * The handler is designed to work with Boost.Beast and ensures thread-safe
 * processing of API requests through an explicit boost::asio::strand<net::io_context::executor_type>.
 */
class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
public:
    /**
     * @brief Construct a new Request Handler
     * @param app Reference to the main application layer (contains game state and players)
     * @param ioc Boost.Asio io_context for asynchronous operations
     *
     * Initializes:
     * - API handler for routing game endpoints
     * - Strand for thread-safe API processing
     * - Ticker for automatic game ticks (if enabled in command-line args)
     */
    explicit RequestHandler(app::Application& app, net::io_context& ioc)
    : app_(app)                 // Application layer (game logic + players)
    , ioc_(ioc)                 // Boost.Asio io_context
    , game_(app_.GetGame())     // Game model
    , static_files_root_(app_.GetCmdArgs().www_root)    // Root for static files
    , api_strand_{net::make_strand(ioc_)}   // Strand for serializing API requests
    , api_handler_{app_}                    // API endpoint router
    {
        // Initialize ticker - required if auto-tick is enabled
        ticker_ = std::make_shared<tick::Ticker>(api_strand_, std::chrono::milliseconds(app_.GetCmdArgs().tick_period),
                                                [this](std::chrono::milliseconds delta)
                                                {
                                                    this->app_.Tick(delta);
                                                }
                                                );
    }

    // Disable copying (handlers are move-only)
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    /**
     * @brief Main request processing operator (called by Boost.Beast)
     * @tparam Body HTTP body type
     * @tparam Allocator Allocator type
     * @tparam Send Callback type for sending response
     * @param req The incoming HTTP request
     * @param send Function to call with the response
     *
     * This is the core request handling logic:
     * 1. Start the ticker if auto-tick is enabled (idempotent - Start() checks if already running)
     * 2. Check if request is for API (starts with /api/v1)
     * 3. If API: dispatch through strand to ApiHandler for thread safety
     * 4. If static file: handle directly in this thread
     * 5. Catch and handle any exceptions, returning appropriate error responses
     */
    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

        // Start ticker if auto-tick is enabled (safe to call multiple times)
        if (app_.GetCmdArgs().tick_period != common_values::NO_AUTO_TICK) {
            ticker_->Start();
        }

        try {
            // Check if this is an API request (starts with /api)
            if (ApiHandler::IsApiRequest(req)) {
                // API requests must be processed sequentially through the strand
                // to avoid race conditions on game state
                auto handle = [self = this->shared_from_this(),
                               send = std::forward<decltype(send)>(send),
                               req = std::forward<decltype(req)>(req)]() mutable
                                {
                                    try {
                                        // Verify we're running inside the strand (debug assertion)
                                        assert(self->api_strand_.running_in_this_thread());

                                        // Process API request and send response
                                        return send(self->api_handler_.HandleApiRequest(std::move(req)));
                                    } catch (std::exception& e) {
                                        // Handle specific exceptions from API handler
                                        send(
                                            response::Builder::MakeError(req, http::status::internal_server_error,
                                                                        error_codes::EXCEPTION_CAUGHT,
                                                                        e.what())
                                            );
                                    } catch (...) {
                                        // Catch-all for non-std::exception errors
                                        send(response::InternalServerError(std::move(req)));
                                    }
                                };
                // Dispatch the handler to run in the strand
                return net::dispatch(api_strand_, std::move(handle));
            }

            // Non-API request: serve static file directly
            return send(HandleFileRequest(std::move(req)));
        }
        catch (std::exception& e) {
            // Handle exceptions from the top-level logic
            send(
                response::Builder::MakeError(req, http::status::internal_server_error,
                                             error_codes::EXCEPTION_CAUGHT,
                                             e.what())
                );
        }
        catch (...) {
            // Catch-all for unknown errors
            boost_logger::LogError(EXIT_FAILURE,
                                std::string(http_handler::error_messages::UNKNOWN_ERROR),
                                "RequestHandler::operator(): Unknown exception caught");
            send(response::InternalServerError(std::move(req)));
        }
    }

private:
    app::Application& app_;         ///< Reference to main application (game logic + players)
    net::io_context& ioc_;          ///< Boost.Asio context for async ops
    model::Game& game_;             ///< Game model (maps, sessions)
    std::filesystem::path static_files_root_;   ///< Root directory for static files
    http_server::Strand api_strand_;            ///< Strand to serialize API requests
    ApiHandler api_handler_;                    ///< API endpoint router
    std::shared_ptr<tick::Ticker> ticker_;      ///< Periodic timer for game updates


    /**
     * @brief Debug helper to print request details to console
     * @param req Request to dump
     *
     * Prints: HTTP method, target path, and all headers.
     */
    void DumpRequest(const StringRequest& req) const;

    /**
     * @brief Handle requests for static files (CSS, JS, HTML, images)
     * @param req The HTTP request
     * @return Response with file contents or error
     *
     * Security features:
     * - Path traversal prevention (checks that resolved path is within www_root)
     * - URL decoding of percent-encoded characters
     * - MIME type detection based on file extension
     * - Directory index support (serves index.html)
     */
    StringResponse HandleFileRequest(const StringRequest& req) const;
};

}  // namespace http_handler
