#pragma once

#include "api_router.h"
#include "http_response.h"
#include "../game_model/game_model.h"
#include "../game_app/application.h"

namespace http_handler {

// API REQUEST HANDLER CLASS
class ApiHandler {
public:
    explicit ApiHandler(app::Application& app)
        : app_(app)
        , router_(std::make_unique<ApiRouter>(ApiRouter{}))
    {
        SetupRoutes();
    }

    static bool IsApiRequest(const StringRequest& req);
    StringResponse HandleApiRequest(StringRequest&& req) const;

private:
    app::Application& app_;
    model::Game& game_ = app_.GetGame();
    std::unique_ptr<ApiRouter> router_;

    void SetupRoutes();

    [[nodiscard]] StringResponse HandleGetMaps(const RequestContext& ctx) const;
    [[nodiscard]] StringResponse HandleGetMapById(const RequestContext& ctx) const;
    [[nodiscard]] StringResponse HandleGameJoin(const RequestContext& ctx) const;
    [[nodiscard]] StringResponse HandleGamePlayers(const RequestContext& ctx) const;
    [[nodiscard]] StringResponse HandleGameState(const RequestContext& ctx) const;
    [[nodiscard]] StringResponse HandleGamePlayerAction(const RequestContext& ctx) const;
    [[nodiscard]] StringResponse HandleGameTick(const RequestContext& ctx) const;
    [[nodiscard]] StringResponse HandleGameRecords(const RequestContext& ctx) const;

    // Универсальный метод парсинга JSON
    bool ParseJsonRequest(const StringRequest& req,
                          const std::function<bool(const json::object&)> &validator,
                          json::object& parsed_json) const;

    // Специализированные методы
    bool ParseGameJoinRequest(const StringRequest& req,
                              std::string& user_name,
                              std::string& map_id) const;

    bool ParsePlayerActionRequest(const StringRequest& req,
                                  std::string& move_value) const;

    bool ParseGameTickRequest(const StringRequest& req,
                              int& time_delta) const;

    bool ParseGameRecordsRequest(const StringRequest& req,
                                int& offset, int& limit) const;
};

} // namespace http_handler
