#pragma once

#include "request_handler.h"    // boost #includes
#include "../common/boost_logger.h"

namespace server_logging {

template <typename Handler>
class LoggingRequestHandler {
public:
    explicit LoggingRequestHandler(Handler handler)
        : handler_(std::move(handler))
    {
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(boost::asio::ip::tcp::endpoint endpoint, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req, Send&& send) {
        // Log request
        boost_logger::LogRequestReceived(
            endpoint.address().to_string(),
            std::string(req.target()),
            std::string(req.method_string())
            );

        auto start_time = std::chrono::steady_clock::now();

        try {
            // Call the actual handler
            handler_(std::move(endpoint), std::move(req),
                     [send = std::forward<Send>(send), start_time]
                     (auto&& response) mutable {
                         // Calculate response time
                         auto end_time = std::chrono::steady_clock::now();
                         auto response_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                                     end_time - start_time)
                                                     .count();

                         // Extract content type
                         std::string content_type = "";
                         if (response.find(boost::beast::http::field::content_type) != response.end()) {
                             content_type = std::string(response[boost::beast::http::field::content_type]);
                         }

                         // Log response
                         boost_logger::LogResponseSent(
                             response_time_ms,
                             response.result_int(),
                             content_type
                             );

                         // Send response
                         send(std::forward<decltype(response)>(response));
                     });
        } catch (std::exception& ex) {
            boost_logger::LogError(EXIT_FAILURE, ex.what(), boost_logger::error_locations::handle_request);
        }
    }

private:
    Handler handler_;
};

} // namespace server_logging
