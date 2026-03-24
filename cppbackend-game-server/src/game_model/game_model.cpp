#include "game_model.h"

#include <ranges>
#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
    // Build road index for the map
    maps_.back().BuildRoadIndex();
}

const Map* Game::FindMap(const Map::Id &id) const {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

Map* Game::FindMap(const Map::Id &id) noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession* Game::FindGameSession(const GameSession::Id &session_id) {
    if (auto it = session_id_to_value_.find(session_id); it != session_id_to_value_.end())
    {
        return &it->second;
    }
    return nullptr;
}

const GameSession* Game::FindGameSession(const GameSession::Id &session_id) const {
    if (auto it = session_id_to_value_.find(session_id); it != session_id_to_value_.end())
    {
        return &it->second;
    }
    return nullptr;
}

void Game::UpdateAllGameSessions(std::chrono::milliseconds time_delta_ms) {
    for (auto &session: session_id_to_value_ | std::views::values) {
        session.UpdateGameState(time_delta_ms);
    }
}

void Game::AddRestoredSession(model::GameSession &&session) {
    auto id = session.GetId();

    // Ensure the ID is not already present (should not happen with valid save data)
    if (session_id_to_value_.contains(id)) {
        std::string error_msg = "Duplicate session ID during session restoration";
        boost_logger::LogError(EXIT_FAILURE, error_msg, "Game::AddRestoredSession");
        throw std::runtime_error(error_msg);
    }

    // Insert the session into the main container
    session_id_to_value_.emplace(id, std::move(session));

    // Update the map from map ID to sessions (for quick lookup)
    auto added_session_ptr = &session_id_to_value_.at(id);
    auto map_ptr = added_session_ptr->GetMap();  // get map pointer from inserted session
    map_id_to_session_[map_ptr->GetId()].push_back(added_session_ptr);

    // Update the session ID counter to avoid collisions with future new sessions
    if (*id >= next_session_id_) {
        next_session_id_ = *id + 1;
    }
}

std::pair<GameSession::Id, bool /*created*/> Game::RequestGameSession(const Map::Id &map_id, boost::asio::io_context &ioc) {
    if (const auto& session_it = map_id_to_session_.find(map_id);
        session_it != map_id_to_session_.end()
        && session_it->second.back()->GetDogs().size() < MAX_PLAYERS_ON_MAP) {
        return {session_it->second.back()->GetId(), false};
    }

    GameSession::Id session_tagg_id = GameSession::Id{next_session_id_++};
    std::string session_name = "Map[" + *map_id + "]:GameSession-" + std::to_string(*session_tagg_id);

    try {
        auto new_session = GameSession{session_tagg_id, std::move(session_name), FindMap(map_id),
                                        ioc, loot_generator_, game_extra_data_, enable_retirement_};
        session_id_to_value_.emplace(new_session.GetId(), std::move(new_session));

        // fill GameSession map index
        auto new_session_ptr = &session_id_to_value_.at(session_tagg_id);
        map_id_to_session_[map_id].emplace_back(new_session_ptr);

        // create bots if enabled
        if (create_bots_) {
            new_session_ptr->CreateBots();
        }

        return {new_session_ptr->GetId(), true};
    } catch(std::exception& e) {
        throw std::runtime_error("Game::RequestGameSession: Exception caught while creating GameSession: "
                                    + std::string(e.what()));
    } catch(...) {
        boost_logger::LogError(EXIT_FAILURE,
                        "Game::RequestGameSession: ",
                        "Unknown exception caught while creating GameSession ");
        throw;
    }
}

}  // namespace model
