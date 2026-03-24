#include "api_handler.h"
#include "../common/utils.h"
#include "serialize_api.h"

namespace http_handler {

void ApiHandler::SetupRoutes() {
    // ALL MAPS
    router_->AddRoute(api_paths::MAPS_BASE,
        {
                        .allowed_methods = {http::verb::get, http::verb::head},
                        .handler = [this](const RequestContext& ctx, std::string_view) {
                            return HandleGetMaps(ctx);}
                });

    // MAP BY ID
    std::string path = std::string(api_paths::MAPS_BASE)
                        + std::string(api_paths::SLASH)
                        + std::string(api_paths::ID);
    router_->AddRoute(path,
        {
                    .allowed_methods = {http::verb::get, http::verb::head},
                    .handler = [this](const RequestContext& ctx, std::string_view) {
                        return HandleGetMapById(ctx);}
                });

    // GAME JOIN
    router_->AddRoute(api_paths::GAME_JOIN,
        {
                    .allowed_methods = {http::verb::post},
                    .content_type = std::string(ContentType::APPLICATION_JSON),
                    .handler = [this](const RequestContext& ctx, std::string_view) {
                        return HandleGameJoin(ctx);}
                });

    // PLAYERS LIST
    router_->AddRoute(api_paths::GAME_PLAYERS,
        {
                    .allowed_methods = {http::verb::get, http::verb::head},
                    .requires_auth = true,
                    .handler = [this](const RequestContext& ctx, std::string_view) {
                        return HandleGamePlayers(ctx);}
                });

    // GAME STATE
    router_->AddRoute(api_paths::GAME_STATE,
        {
                    .allowed_methods = {http::verb::get, http::verb::head},
                    .requires_auth = true,
                    .handler = [this](const RequestContext& ctx, std::string_view) {
                        return HandleGameState(ctx);}
                });

    // PLAYER ACTION
    router_->AddRoute(api_paths::PLAYER_ACTION,
        {
                    .allowed_methods = {http::verb::post},
                    .requires_auth = true,
                    .content_type = std::string(ContentType::APPLICATION_JSON),
                    .handler = [this](const RequestContext& ctx, std::string_view) {
                        return HandleGamePlayerAction(ctx);}
                });

    // GAME TICK
    router_->AddRoute(api_paths::GAME_TICK,
        {
                    .allowed_methods = {http::verb::post},
                    .content_type = std::string(ContentType::APPLICATION_JSON),
                    .handler = [this](const RequestContext& ctx, std::string_view) {
                        return HandleGameTick(ctx);
                    },
                    .auto_tick_enabled = (app_.GetCmdArgs().tick_period != common_values::NO_AUTO_TICK)
                });

    // GAME RECORDS
    router_->AddRoute(api_paths::GAME_RECORDS,
        {
                    .allowed_methods = {http::verb::get, http::verb::head},
                    .content_type = std::string(ContentType::APPLICATION_JSON),
                    .handler = [this](const RequestContext& ctx, std::string_view) {
                        return HandleGameRecords(ctx);}
                });
}

bool ApiHandler::IsApiRequest(const StringRequest &req)
{
    if (req.target().starts_with(api_paths::API_PREFIX)) {
        return true;
    }
    return false;
}

StringResponse ApiHandler::HandleApiRequest(StringRequest&& req) const {
    RequestContext ctx{req, std::nullopt};

    // Extract token if present (for all requests)
    ctx.token = utils::http::ExtractToken(req);

    // Try to route via modular router
    if (auto response = router_->Route(req, ctx)) {
        return std::move(*response);
    }

    // No route matched
    return response::Builder::MakeError(req, http::status::bad_request,
                                        error_codes::BAD_REQUEST,
                                        error_messages::BAD_REQUEST);
}

StringResponse ApiHandler::HandleGetMaps(const RequestContext &ctx) const {
    json::array maps_array;
    for (const auto& map : game_.GetMaps()) {
        maps_array.push_back(serialize_api::SerializeMapBrief(map));
    }
    return response::Builder::MakeJson(ctx.req, maps_array);
}

StringResponse ApiHandler::HandleGetMapById(const RequestContext& ctx) const {
    auto it = ctx.path_params.find(json_fields::ID);
    if (it == ctx.path_params.end()) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::BAD_REQUEST,
                                            error_messages::BAD_REQUEST);
    }
    const std::string& map_id = it->second;

    const model::Map* map = game_.FindMap(model::Map::Id{map_id});
    if (!map) {
        return response::Builder::MakeError(ctx.req, http::status::not_found,
                                            error_codes::MAP_NOT_FOUND,
                                            error_messages::MAP_NOT_FOUND);
    }
    return response::Builder::MakeJson(ctx.req, serialize_api::SerializeMapFull(*map, game_.GetGameExtraData().get()));
}

StringResponse ApiHandler::HandleGameJoin(const RequestContext& ctx) const {
    // Parse request body
    std::string user_name;
    std::string map_id;

    if (!ParseGameJoinRequest(ctx.req, user_name, map_id)) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::INVALID_ARGUMENT, error_messages::JOIN_GAME_PARSE);
    }

    // Validate user name
    if (user_name.empty()) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::INVALID_ARGUMENT, error_messages::INVALID_NAME);
    }

    // Find map
    if (auto map_ptr = game_.FindMap(model::Map::Id{map_id}); !map_ptr) {
        return response::Builder::MakeError(ctx.req, http::status::not_found,
                                            error_codes::MAP_NOT_FOUND, error_messages::MAP_NOT_FOUND);
    }

    // Add player
    try {
        auto& player = app_.AddPlayer(user_name, map_id);

        // Get token for the player
        auto token = app_.GetPlayers().FindTokenByPlayer(player);
        if (!token) {
            return response::InternalServerError(ctx.req);
        }

        // Create response JSON
        json::object response_json;
        response_json[json_fields::AUTH_TOKEN] = **token;  // Convert Token to string
        response_json[json_fields::PLAYER_ID] = player.GetId();  // Get actual player ID

        return response::Builder::MakeJson(ctx.req, response_json);;

    } catch (const std::exception&) {
        return response::InternalServerError(ctx.req);
    }
}

StringResponse ApiHandler::HandleGamePlayers(const RequestContext& ctx) const
{
    // INVALID_TOKEN check done in ModularRouter::Route
    if (!ctx.token.has_value()) {
        throw std::runtime_error("Token required for current Handler.");
    }
    auto token = ctx.token.value();
    // Проверяем токен
    auto player = app_.GetPlayers().FindPlayerByToken(token);
    if (!player) {
        return response::Builder::MakeError(ctx.req, http::status::unauthorized,
                                            error_codes::UNKNOWN_TOKEN,
                                            error_messages::UNKNOWN_TOKEN);
    }

    return response::Builder::MakeJson(ctx.req, serialize_api::SerializeGamePlayers(*player->GetGameSession()));
}

StringResponse ApiHandler::HandleGameState(const RequestContext& ctx) const
{
    // INVALID_TOKEN check done in ModularRouter::Route
    if (!ctx.token.has_value()) {
        throw std::runtime_error("Token required for current API.");
    }
    auto token = ctx.token.value();
    // Проверяем токен
    auto player = app_.GetPlayers().FindPlayerByToken(token);
    if (!player) {
        return response::Builder::MakeError(ctx.req, http::status::unauthorized,
                                            error_codes::UNKNOWN_TOKEN,
                                            error_messages::UNKNOWN_TOKEN);
    }

    auto session = player->GetGameSession();
    if (!session) {
        return response::InternalServerError(ctx.req);
    }

    return response::Builder::MakeJson(ctx.req, serialize_api::SerializeGameState(*session));
}

StringResponse ApiHandler::HandleGamePlayerAction(const RequestContext& ctx) const
{

    // INVALID_TOKEN check done in ModularRouter::Route
    if (!ctx.token.has_value()) {
        throw std::runtime_error("Token required for current Handler.");
    }
    auto token = ctx.token.value();
    // Проверяем токен
    auto player = app_.GetPlayers().FindPlayerByToken(token);
    if (!player) {
        return response::Builder::MakeError(ctx.req, http::status::unauthorized,
                                            error_codes::UNKNOWN_TOKEN,
                                            error_messages::UNKNOWN_TOKEN);
    }

    // Парсим запрос
    std::string move_value;
    if (!ParsePlayerActionRequest(ctx.req, move_value)) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::INVALID_ARGUMENT,
                                            error_messages::ACTION_PARSE_ERROR);
    }

    auto direction_opt = utils::StringToDirection2D(move_value);
    // Проверяем допустимые значения move
    if (!direction_opt) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::INVALID_ARGUMENT,
                                            error_messages::INVALID_ACTION_VALUE);
    }

    // Обновляем направление игрока
    player->SetDirection(*direction_opt);
    // boost_logger::LogInfo("ApiHandler::HandleGamePlayerAction: Player id=" +
    //                         std::to_string(player->GetId()) + " SetDirection[" +
    //                         move_value == app::move_values::STOP ? "STOP" : move_value + "]");

    // Возвращаем успешный ответ
    return response::Builder::MakeJson(ctx.req, json::object{});
}

StringResponse ApiHandler::HandleGameTick(const RequestContext &ctx) const
{
    // Парсим запрос
    int time_delta_ms;
    if (!ParseGameTickRequest(ctx.req, time_delta_ms)) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::INVALID_ARGUMENT,
                                            error_messages::TICK_PARSE_ERROR);
    }

    if (time_delta_ms == 0) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::INVALID_ARGUMENT,
                                            error_messages::INVALID_TICK_VALUE);
    }

    // Update all game sessions
    app_.Tick(std::chrono::milliseconds(time_delta_ms));

    // Send response
    return response::Builder::MakeJson(ctx.req, json::object{});
}

StringResponse ApiHandler::HandleGameRecords(const RequestContext &ctx) const {
    int offset{}, limit{};
    // limit should not be more than common_values::DB_MAX_ITEMS_TO_GET
    if (!ParseGameRecordsRequest(ctx.req, offset, limit)) {
        return response::Builder::MakeError(ctx.req, http::status::bad_request,
                                            error_codes::INVALID_ARGUMENT,
                                            "Invalid Game Records query parameters");
    }

    auto scores = app_.GetPlayerScores(limit, offset);

    boost::json::array arr;
    for (const auto& score : scores) {
        boost::json::object obj;
        obj["name"] = score.name;
        obj["score"] = score.score;
        obj["playTime"] = score.play_time_sec;          // stored as double (seconds)
        arr.push_back(std::move(obj));
    }

    return response::Builder::MakeJson(ctx.req, arr);
}

/**
 * Parses the HTTP request body as JSON, validates that it is a JSON object,
 * optionally runs a user‑supplied validator, and stores the result.
 *
 * @param req          The incoming HTTP request containing the body to parse.
 * @param validator    A callable (std::function) that takes a const reference to
 *                     the parsed JSON object and returns true if the object is valid,
 *                     false otherwise. If this is empty (nullptr), validation is skipped.
 * @param parsed_json  [out] On success, holds the parsed JSON object.
 *                     Unchanged if parsing or validation fails.
 * @return true if:
 *         - The body was successfully parsed as JSON,
 *         - The parsed value is a JSON object,
 *         - The optional validator returns true (or no validator is provided).
 *         false otherwise (parse error, wrong type, or validator rejected the object).
 *
 * @note Exceptions during parsing or validation are caught, logged, and cause a false return.
 * @note The function is const and does not modify any member variables.
 */
bool ApiHandler::ParseJsonRequest(const StringRequest& req,
                                  const std::function<bool(const json::object&)> &validator,
                                  json::object& parsed_json) const {
    try {
        // Attempt to parse the entire request body as JSON.
        // json::parse will throw an exception (derived from std::exception) if:
        // - The body is empty or malformed,
        // - The JSON syntax is invalid,
        // - The input contains unexpected characters.
        // It returns a json::value that can hold any JSON type.
        json::value json_body = json::parse(req.body());

        // Verify that the top‑level JSON value is an object (i.e., {...}).
        // If it's an array, string, number, boolean, or null, we reject it.
        // This check prevents type mismatches later when we try to access members.
        if (!json_body.is_object()) {
            // Not a JSON object → return false without modifying parsed_json.
            // No error is logged here because the caller might expect a specific type;
            // logging could be added if desired.
            return false;
        }

        // Extract the JSON object from the value.
        // Since we already confirmed is_object(), as_object() is safe and returns a reference.
        // The assignment copies the object into parsed_json (may be expensive for large objects).
        parsed_json = json_body.as_object();

        // If a validator was provided (i.e., the std::function is not empty),
        // invoke it with the parsed object. The validator should perform semantic checks
        // (e.g., required fields, correct data types, business logic).
        // If the validator returns false, we treat the request as invalid.
        if (validator && !validator(parsed_json)) {
            // Validator rejected the object; no further processing.
            return false;
        }

        // All checks passed: parsed_json now holds a valid JSON object.
        return true;

    } catch (const std::exception& e) {
        // Catch any standard exception thrown during parsing or validation.
        // This includes:
        // - json::parse errors (syntax, incomplete input)
        // - Exceptions from the validator if it throws (though it should ideally not)
        // - Any other std::exception derived errors.
        // Log the error with a descriptive message and the function name.
        // EXIT_FAILURE is used here as a severity level or error code (adjust as needed).
        boost_logger::LogError(EXIT_FAILURE, e.what(), "ApiHandler::ParseJsonRequest");

        // Return false because we cannot produce a valid parsed_json.
        return false;
    }
    // Note: Non‑std exceptions (e.g., int, custom types not derived from std::exception)
    // would not be caught here and would propagate. In practice, json::parse and std::function
    // only throw std::exception derivatives, so this is acceptable.
}

bool ApiHandler::ParseGameJoinRequest(const StringRequest& req,
                                      std::string& user_name,
                                      std::string& map_id) const {
    json::object parsed_json;

    // Создаем валидатор для проверки обязательных полей
    auto validator = [&](const json::object& obj) -> bool {
        auto user_name_it = obj.find(json_fields::USER_NAME);
        auto map_id_it = obj.find(json_fields::MAP_ID);

        return user_name_it != obj.end() && map_id_it != obj.end();
    };

    if (!ParseJsonRequest(req, validator, parsed_json)) {
        return false;
    }

    // Извлекаем значения
    user_name = std::string(parsed_json.at(json_fields::USER_NAME).as_string().c_str());
    map_id = std::string(parsed_json.at(json_fields::MAP_ID).as_string().c_str());

    return true;
}

bool ApiHandler::ParsePlayerActionRequest(const StringRequest& req,
                                          std::string& move_value) const {
    json::object parsed_json;

    // Валидатор проверяет наличие поля move
    auto validator = [&](const json::object& obj) -> bool {
        auto move_it = obj.find(json_fields::MOVE);
        return move_it != obj.end();
    };

    if (!ParseJsonRequest(req, validator, parsed_json)) {
        return false;
    }

    // Извлекаем значение
    move_value = std::string(parsed_json.at(json_fields::MOVE).as_string().c_str());
    return true;
}

bool ApiHandler::ParseGameTickRequest(const StringRequest &req, int &time_delta) const
{
    json::object parsed_json;

    // Валидатор проверяет наличие поля time_delta
    auto validator = [&](const json::object& obj) -> bool {
        auto move_it = obj.find(json_fields::TIME_DELTA);
        return move_it != obj.end();
    };

    if (!ParseJsonRequest(req, validator, parsed_json)) {
        return false;
    }

    if (!parsed_json.at(json_fields::TIME_DELTA).is_number()) {
        return false;
    }

    // Извлекаем значение
    time_delta = parsed_json.at(json_fields::TIME_DELTA).to_number<int>();
    return true;
}

/**
 * Parses a request for game records, extracting pagination parameters from the query string.
 *
 * Expected API format: "/api/v1/game/records?start=0&maxItems=100"
 * Parameters:
 *   - "start":     zero‑based offset (default 0)
 *   - "maxItems":  maximum number of records to return (default DB_MAX_ITEMS_TO_GET)
 *
 * The function handles missing parameters, malformed values, and any order of keys.
 * It performs bounds checking on the parsed values.
 *
 * @param req         The incoming HTTP request containing the target URI.
 * @param offset      [out] Parsed offset, set to 0 if missing or invalid.
 * @param limit       [out] Parsed limit, set to DB_MAX_ITEMS_TO_GET if missing or invalid,
 *                    then clamped to [1, DB_MAX_ITEMS_TO_GET].
 * @return true if final offset and limit satisfy:
 *         offset >= 0 && limit > 0 && limit <= DB_MAX_ITEMS_TO_GET;
 *         false otherwise (i.e., invalid parameters that couldn't be defaulted to acceptable values).
 */
bool ApiHandler::ParseGameRecordsRequest(const StringRequest &req, int &offset, int &limit) const {
    // Set safe default values: offset 0, limit to the database maximum.
    offset = 0;
    limit = common_values::DB_MAX_ITEMS_TO_GET;

    // Extract the target URI as a string_view for efficient, non‑copying access.
    std::string_view target = req.target();

    // Locate the beginning of the query string (the '?' character).
    auto qpos = target.find('?');
    // If there is no '?', there is no query string → return true with defaults.
    if (qpos == std::string_view::npos) {
        return true;
    };

    // Extract the query part: everything after the '?'.
    // Example: "start=0&maxItems=100"
    std::string_view query = target.substr(qpos + 1);

    // Parse the query string by splitting on '&' characters.
    size_t pos = 0; // Current position in the query string.
    while (pos < query.size()) {
        // Find the next '&' starting from 'pos'.
        size_t amp = query.find('&', pos);

        // Extract the current parameter substring:
        // - If '&' is found, the parameter runs from 'pos' to just before the '&'.
        // - If no '&' is found (amp == npos), the parameter runs to the end of the query.
        std::string_view param = query.substr(pos, (amp == std::string_view::npos) ? amp : amp - pos);

        // Advance the position for the next iteration.
        // - If '&' was found, move past it (amp + 1).
        // - If no '&', we have processed the last parameter; set pos to the end.
        pos = (amp == std::string_view::npos) ? query.size() : amp + 1;

        // Each parameter should be of the form "key=value".
        // Find the '=' separator.
        size_t eq = param.find('=');
        if (eq == std::string_view::npos) {
            // Malformed parameter: no '='. Silently ignore (skip) it.
            continue;
        }

        // Split the parameter into key and value parts.
        std::string_view key = param.substr(0, eq);
        std::string_view value = param.substr(eq + 1);

        // Local helper lambda to parse an integer from a std::string_view.
        // Uses std::from_chars for fast, non‑throwing, locale‑independent conversion.
        // Returns true if the conversion succeeded (no overflow/underflow, at least one digit parsed),
        // and stores the result in 'out'. Note: this accepts partial conversions, e.g., "123abc" becomes 123.
        auto parse_int = [](std::string_view v, int& out) {
            auto [ptr, ec] = std::from_chars(v.data(), v.data() + v.size(), out);
            // Success if no error and at least one character was consumed.
            // (ec == std::errc{} means no error.)
            return ec == std::errc{};
        };

        // Match known parameter keys (using constants from json_fields).
        if (key == json_fields::GAME_RECORDS_OFFSET) {
            // Attempt to parse the offset value.
            // If parsing fails, leave offset unchanged (still the default or a previously set value).
            parse_int(value, offset);
        } else if (key == json_fields::GAME_RECORDS_MAX_ITEMS) {
            // Attempt to parse the limit value.
            parse_int(value, limit);
        }
        // Unknown keys are ignored (no error, no action).
    }

    // Final validation of parsed values before returning.
    // - offset must be non‑negative (pagination from beginning).
    // - limit must be positive and not exceed the database maximum.
    // If these conditions are not met, the request is considered invalid.
    return (offset >= 0 &&
            limit > 0 && limit <= common_values::DB_MAX_ITEMS_TO_GET);
}

} // namespace http_handler
