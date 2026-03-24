#pragma once

#include "game_extra_data.h"
#include "game_session.h"
#include "game_map.h"

namespace model {

class Game {
public:
    using Maps = std::vector<Map>;
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using GameSessionIdHasher = util::TaggedHasher<GameSession::Id>;
    using GameSessionIdToValue = std::unordered_map<GameSession::Id, GameSession, GameSessionIdHasher>;
    using MapIdToGameSession = std::unordered_map<Map::Id, std::vector<const GameSession*>, MapIdHasher>;

    Game(std::shared_ptr<extra_data::GameExtraData> game_extra_data)
        : game_extra_data_(std::move(game_extra_data))
    {
        auto config = game_extra_data_->GetLootGeneratorConfig();
        // К этому моменту данные для инициализации LootGenerator уже доступны
        // (см. json_loader::LoadGame -> LoadLootGeneratorConfig)
        loot_generator_ = std::make_shared<loot_gen::LootGenerator>(config.period, config.probability);
    }

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const;

    Map* FindMap(const Map::Id& id) noexcept;

    const app_geom::Speed2D& GetSpeedForMap(const Map::Id &map_id) {
        return game_extra_data_->GetSpeedForMap(map_id);
    }

    const std::shared_ptr<extra_data::GameExtraData> GetGameExtraData() const {
        return game_extra_data_;
    }

    // Finds GameSession for selected Map, creates new GameSession if needed
    [[nodiscard]] std::pair<GameSession::Id, bool /*new_created*/> RequestGameSession(const Map::Id &map_id, boost::asio::io_context& ioc);

    GameSession* FindGameSession(const GameSession::Id& session_id);

    const GameSession* FindGameSession(const GameSession::Id& session_id) const;

    void UpdateAllGameSessions(std::chrono::milliseconds time_delta_ms);

    void SetCreateBots(bool enable) {
        create_bots_ = enable;
    }

    std::shared_ptr<loot_gen::LootGenerator> GetLootGenerator() {
        return loot_generator_;
    }

    const GameSessionIdToValue& GetSessions() const {
        return session_id_to_value_;
    }

    void AddRestoredSession(model::GameSession&& session);

    // Enable or disable dog retirement due to idle timeout.
    // When disabled, dogs will never be retired, and players persist indefinitely.
    void SetEnableRetirement(bool enable) {
        enable_retirement_ = enable;
    }

    bool IsRetirementEnabled() const {
        return enable_retirement_;
    }

private:
    // origin for Maps data
    std::vector<Map> maps_;
    // for quick search
    MapIdToIndex map_id_to_index_;

    // GameExtraData
    std::shared_ptr<extra_data::GameExtraData> game_extra_data_;

    // origin for GameSession data
    std::uint32_t next_session_id_ = 1;
    GameSessionIdToValue session_id_to_value_;

    // GameSession to Map index
    MapIdToGameSession map_id_to_session_;

    // LootGenerator for Loot processing
    std::shared_ptr<loot_gen::LootGenerator> loot_generator_ = nullptr;

    // if bots needed
    bool create_bots_ = false;
    // if remote database (enabled by default) used - retirement also used
    bool enable_retirement_ = true;   // enabled by default
};

}  // namespace model
