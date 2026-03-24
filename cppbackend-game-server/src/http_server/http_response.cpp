#include "http_response.h"

namespace http_handler {

// RESPONSE BUILDER IMPLEMENTATION
namespace response {

Builder Builder::From(const StringRequest& req) {
    Builder builder;
    builder.response_.keep_alive(req.keep_alive());
    builder.response_.version(req.version());
    return builder;
}

Builder Builder::Json(const StringRequest& req) {
    return From(req).WithContentType(ContentType::APPLICATION_JSON);
}

Builder Builder::Error(const StringRequest& req) {
    return Json(req);  // Error responses are also JSON
}

// Modify existing response
Builder Builder::Modify(const StringResponse &response) {
    Builder builder;
    builder.response_ = std::move(response);
    return builder;
}

Builder& Builder::WithStatus(http::status status) {
    response_.result(status);
    return *this;
}

Builder& Builder::WithBody(std::string_view body) {
    response_.body() = std::string(body);
    response_.content_length(body.size());
    return *this;
}

Builder& Builder::WithJson(const json::value& json) {
    response_.body() = json::serialize(json);
    response_.content_length(response_.body().size());
    response_.set(http::field::content_type, std::string(ContentType::APPLICATION_JSON));
    return *this;
}

Builder& Builder::WithError(std::string_view code, std::string_view message) {
    json::object error_obj;
    error_obj[json_fields::CODE] = std::string(code);
    error_obj[json_fields::MESSAGE] = std::string(message);

    response_.body() = json::serialize(error_obj);
    response_.content_length(response_.body().size());
    response_.set(http::field::content_type, std::string(ContentType::APPLICATION_JSON));
    return *this;
}

Builder& Builder::WithContentType(std::string_view content_type) {
    response_.set(http::field::content_type, std::string(content_type));
    return *this;
}

Builder& Builder::WithKeepAlive(bool keep_alive) {
    response_.keep_alive(keep_alive);
    return *this;
}

Builder& Builder::WithVersion(unsigned version) {
    response_.version(version);
    return *this;
}

Builder& Builder::WithCacheControl(std::string_view cache_control) {
    response_.set(http::field::cache_control, std::string(cache_control));
    return *this;
}

Builder& Builder::WithAllow(std::string_view allow) {
    response_.set(http::field::allow, std::string(allow));
    return *this;
}

StringResponse Builder::Build() const {
    return response_;
}

StringResponse Builder::MakeJson(const StringRequest& req, const json::value& json, http::status status) {
    return Json(req)
    .WithStatus(status)
        .WithJson(json)
        .Build();
}

StringResponse Builder::MakeError(const StringRequest& req, http::status status,
                                  std::string_view code, std::string_view message) {
    return Error(req)
    .WithStatus(status)
        .WithError(code, message)
        .Build();
}

StringResponse Builder::MakeText(const StringRequest& req, std::string_view body, http::status status) {
    return From(req)
    .WithStatus(status)
        .WithBody(body)
        .Build();
}

} // namespace response

} // namespace http_handler
