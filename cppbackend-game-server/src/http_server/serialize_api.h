#pragma once

#include <boost/json.hpp>
#include "../game_app/players.h"
#include "../game_model/game_model.h"

namespace app_geom {        // namespace app_geom used here for ADL lookup when calling tag_invoke

    constexpr static inline double DOUBLE_JSON_PRECISION = 100.0;  // for two digits after dot

    // tag_invoke overload, which is automatically called when app_geom::Position2D serialized
    // possible usage: boost::json::value_from(app_geom::Position2D)
    static void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Position2D const& pos) {
        auto fix = [](double v) {
            // Round up to 2 digits before making json::array
            return std::round(v * DOUBLE_JSON_PRECISION) / DOUBLE_JSON_PRECISION;
        };

        jv = { fix(pos.x), fix(pos.y) };
    }

} // namespace app_geom

namespace serialize_api {

namespace json = boost::json;

    json::value SerializeMapBrief(const model::Map& map);
    json::value SerializeMapFull(const model::Map& map, const extra_data::GameExtraData* extra_data);
    json::object SerializePlayersAll(const std::map<uint32_t, const app::Player*>& players);
    json::object SerializeGameState(const model::GameSession& session);
    json::object SerializeGamePlayers(const std::map<std::uint32_t, const app::Player*>& players);
    json::object SerializeGamePlayers(const model::GameSession& session);

} // namespace serialize_api
