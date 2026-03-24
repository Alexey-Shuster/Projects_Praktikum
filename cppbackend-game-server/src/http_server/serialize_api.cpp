#include "serialize_api.h"
#include "../common/constants.h"

namespace serialize_api {

json::value SerializeRoad(const model::Road &road) {
    json::object road_obj;
    road_obj[json_fields::X0] = road.GetStart().x;
    road_obj[json_fields::Y0] = road.GetStart().y;

    if (road.IsHorizontal()) {
        road_obj[json_fields::X1] = road.GetEnd().x;
    } else {
        road_obj[json_fields::Y1] = road.GetEnd().y;
    }

    return road_obj;
}

json::value SerializeBuilding(const model::Building &building) {
    json::object building_obj;
    const auto& bounds = building.GetBounds();

    building_obj[json_fields::X] = bounds.position.x;
    building_obj[json_fields::Y] = bounds.position.y;
    building_obj[json_fields::W] = bounds.size.width;
    building_obj[json_fields::H] = bounds.size.height;

    return building_obj;
}

json::value SerializeOffice(const model::Office &office) {
    json::object office_obj;
    const auto& offset = office.GetOffset();
    const auto& position = office.GetPosition();

    office_obj[json_fields::ID] = std::string(*office.GetId());
    office_obj[json_fields::X] = position.x;
    office_obj[json_fields::Y] = position.y;
    office_obj[json_fields::OFFSET_X] = offset.dx;
    office_obj[json_fields::OFFSET_Y] = offset.dy;

    return office_obj;
}

json::object SerializeLoot(const extra_data::LootData& loot) {
    json::object obj;
    if (loot.name.empty() || loot.file.empty() || loot.type.empty() || loot.scale == 0.0) {
        throw std::invalid_argument("SerializeLoot: Invalid Loot");
    }
    obj[json_fields::NAME] = loot.name;
    obj[json_fields::FILE] = loot.file;
    obj[json_fields::TYPE] = loot.type;
    if (loot.rotation.has_value()) {
        obj[json_fields::ROTATION] = loot.rotation.value();
    }
    if (loot.color.has_value()) {
        obj[json_fields::COLOR] = loot.color.value();
    }
    obj[json_fields::SCALE] = loot.scale;
    obj[json_fields::VALUE] = loot.value;

    return obj;
}

json::array SerializeLootTypes(const std::vector<extra_data::LootData>& loots) {
    json::array arr;
    for (const auto& loot : loots) {
        arr.push_back(SerializeLoot(loot));
    }
    return arr;
}

json::value SerializeMapBrief(const model::Map &map) {
    json::object map_obj;
    map_obj[json_fields::ID] = std::string(*map.GetId());
    map_obj[json_fields::NAME] = map.GetName();
    return map_obj;
}

json::value SerializeMapFull(const model::Map &map, const extra_data::GameExtraData* extra_data) {
    json::object map_obj;
    map_obj[json_fields::ID] = std::string(*map.GetId());
    map_obj[json_fields::NAME] = map.GetName();

    json::array roads_array;
    for (const auto& road : map.GetRoads()) {
        roads_array.push_back(SerializeRoad(road));
    }
    map_obj[json_fields::ROADS] = std::move(roads_array);

    json::array buildings_array;
    for (const auto& building : map.GetBuildings()) {
        buildings_array.push_back(SerializeBuilding(building));
    }
    map_obj[json_fields::BUILDINGS] = std::move(buildings_array);

    json::array offices_array;
    for (const auto& office : map.GetOffices()) {
        offices_array.push_back(SerializeOffice(office));
    }
    map_obj[json_fields::OFFICES] = std::move(offices_array);

    const auto& loots = extra_data->GetLootTypes(map.GetId());
    if (!loots.empty()) {
        map_obj[json_fields::LOOT_TYPES] = SerializeLootTypes(loots);
    }

    return map_obj;
}

json::object SerializePlayersAll(const std::map<uint32_t, const app::Player*> &players) {
    json::object players_state_json;

    // Get all players
    for (const auto& [player_id, player_ptr] : players) {
        json::object player_state;

        // Get player's position
        const auto& pos = player_ptr->GetPosition();
        json::array pos_array;
        pos_array.push_back(pos.x);
        pos_array.push_back(pos.y);
        player_state[json_fields::POS] = std::move(pos_array);

        // Get player's speed
        const auto& speed = player_ptr->GetSpeed();
        json::array speed_array;
        speed_array.push_back(speed.x);
        speed_array.push_back(speed.y);
        player_state[json_fields::SPEED] = std::move(speed_array);

        // Get player's direction and convert to string
        player_state[json_fields::DIR] = utils::Direction2DToString(player_ptr->GetDirection());

        players_state_json[std::to_string(player_id)] = std::move(player_state);
    }

    // Create final response JSON
    json::object response_json;
    response_json[json_fields::PLAYERS] = std::move(players_state_json);

    return response_json;
}

json::object SerializeGameStatePlayerState(const model::Dog& dog) {
    json::object player_state;

    // player_state[json_fields::POS] = json::array{dog.GetPosition().x, dog.GetPosition().y};
    player_state[json_fields::POS] = json::value_from(dog.GetPosition());   // for two digits precision
    player_state[json_fields::SPEED] = json::array{dog.GetSpeed().x, dog.GetSpeed().y};
    player_state[json_fields::DIR] = utils::Direction2DToString(dog.GetDirection());

    json::array player_bag;
    for (const auto& item : dog.GetBag()) {
        json::object loot;
        loot[json_fields::ID] = item->object_id;
        loot[json_fields::TYPE] = item->loot_data_ptr->type_id;
        player_bag.push_back(loot);
    }
    player_state[json_fields::BAG] = std::move(player_bag);
    player_state[json_fields::SCORE] = dog.GetScore();

    return player_state;
}

json::object SerializeGameStatePlayers(const model::GameSession& session) {
    json::object players_json;

    for (const auto& [dog_id, dog] : session.GetDogs()) {
        players_json[std::to_string(dog_id)] = std::move(SerializeGameStatePlayerState(dog));
    }
    for (const auto& [dog_id, dog] : session.GetBots()) {
        players_json[std::to_string(dog_id)] = std::move(SerializeGameStatePlayerState(dog));
    }

    return players_json;
}

json::object SerializeGameStateLostObjects(const model::GameSession& session) {
    json::object lost_objects_json;

    for (const auto& [loot_id, loot] : session.GetLootNotCollected()) {
        json::object loot_obj;
        loot_obj[json_fields::TYPE] = loot->loot_data_ptr->type_id;
        // loot_obj[json_fields::POS] = json::array{loot.pos.x, loot.pos.y};
        loot_obj[json_fields::POS] = json::value_from(loot->pos);    // for two digits precision
        lost_objects_json[std::to_string(loot_id)] = std::move(loot_obj);
    }

    return lost_objects_json;
}

json::object SerializeGameState(const model::GameSession& session) {
    json::object result;
    result[json_fields::PLAYERS] = SerializeGameStatePlayers(session);
    result[json_fields::LOST_OBJECTS] = SerializeGameStateLostObjects(session);
    return result;
}

json::object SerializeGamePlayers(const std::map<std::uint32_t, const app::Player*>& players) {
    json::object players_json;

    for (const auto& [player_id, player_ptr] : players) {
        json::object player_info;
        player_info[json_fields::NAME] = player_ptr->GetName();
        players_json[std::to_string(player_id)] = std::move(player_info);
    }

    return players_json;
}

json::object SerializeGamePlayers(const model::GameSession& session) {
    json::object players_json;

    // players
    for (const auto& dog : session.GetDogs() | std::views::values) {
        json::object dog_info;
        dog_info[json_fields::NAME] = dog.GetName();
        players_json[std::to_string(dog.GetId())] = std::move(dog_info);
    }
    // bots
    for (const auto& bot : session.GetBots() | std::views::values) {
        json::object bot_info;
        bot_info[json_fields::NAME] = bot.GetName();
        players_json[std::to_string(bot.GetId())] = std::move(bot_info);
    }

    return players_json;
}
} // namespace serialize_api
