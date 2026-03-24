#pragma once

#include <boost/json.hpp>

#include "http_server.h"    // boost includings
#include "../common/constants.h"

namespace http_handler {

namespace net = boost::asio;
namespace sys = boost::system;
namespace http = boost::beast::http;
namespace json = boost::json;

namespace fs = std::filesystem;

using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;

// RESPONSE BUILDER NAMESPACE (Public utility for creating responses)
namespace response {

// Main builder class for creating HTTP responses
class Builder {
public:
    // Factory methods for common cases
    static Builder From(const StringRequest& req);
    static Builder Json(const StringRequest& req);
    static Builder Error(const StringRequest& req);
    static Builder Modify(const StringResponse& response);

    // Fluent interface for configuration
    Builder& WithStatus(http::status status);
    Builder& WithBody(std::string_view body);
    Builder& WithJson(const json::value& json);
    Builder& WithError(std::string_view code, std::string_view message);
    Builder& WithContentType(std::string_view content_type);
    Builder& WithKeepAlive(bool keep_alive);
    Builder& WithVersion(unsigned version);
    Builder& WithCacheControl(std::string_view cache_control);
    Builder& WithAllow(std::string_view allow);

    // Build the response
    // If not explicitly set than default values:
    // http::status::ok, http-version = 11, http::field::content_type = "text/html", keep_alive = true,
    // http::field::cache_control = "no-cache"
    StringResponse Build() const;

    // Direct build methods (for common patterns)
    static StringResponse MakeJson(const StringRequest& req, const json::value& json, http::status status = http::status::ok);
    static StringResponse MakeError(const StringRequest& req, http::status status,
                                    std::string_view code, std::string_view message);
    static StringResponse MakeText(const StringRequest& req, std::string_view body,
                                   http::status status = http::status::ok);

private:
    // Internal state
    StringResponse response_ = [] {
        StringResponse r(http::status::ok, common_values::VERSION);
        r.set(http::field::content_type, std::string(ContentType::TEXT_HTML));
        r.keep_alive(true);
        r.set(http::field::cache_control, json_fields::NO_CACHE);
        return r;
    }();
};

// Convenience functions for common response types
inline StringResponse BadRequest(const StringRequest& req) {
    return Builder::MakeError(req, http::status::bad_request,
                              error_codes::BAD_REQUEST,
                              error_messages::BAD_REQUEST);
}

inline StringResponse MethodNotAllowed(const StringRequest& req) {
    return Builder::MakeError(req, http::status::method_not_allowed,
                              error_codes::INVALID_METHOD,
                              error_messages::INVALID_METHOD);
}

inline StringResponse InternalServerError(const StringRequest& req) {
    return Builder::MakeError(req, http::status::internal_server_error,
                              error_codes::INTERNAL_ERROR,
                              error_messages::SOURCE_FAIL);
}

} // namespace response

} // namespace http_handler
