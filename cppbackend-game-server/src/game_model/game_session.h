#pragma once

#include <map>
#include <ranges>
#include <string>
#include <boost/signals2.hpp>

#include "dog.h"
#include "game_map.h"
#include "../common/game_utils/collision_detector.h"
#include "../common/game_utils/loot_generator.h"
#include "../common/tagged.h"
#include "../game_bots/bot_manager.h"

namespace model {

    constexpr static inline size_t MAX_PLAYERS_ON_MAP = common_values::MAX_PLAYERS_ON_MAP;

class GameSession {
public:
    using Id = util::Tagged<std::uint32_t, GameSession>;
    using DogDeletedSignal =  boost::signals2::signal<void(uint32_t dog_id, uint32_t dog_score)>;

    GameSession(Id id, std::string name, const Map* map,
                boost::asio::io_context &ioc,
                std::shared_ptr<loot_gen::LootGenerator> loot_generator,
                std::shared_ptr<extra_data::GameExtraData> game_extra_data,
                bool enable_retirement = true) noexcept
        : id_(id)
        , name_(std::move(name))
        , map_(map)
        , ioc_(ioc)
        , strand_(boost::asio::make_strand(ioc_))
        , game_extra_data_(std::move(game_extra_data))
        , loot_generator_(std::move(loot_generator))
        , bot_manager_(map)
        , enable_retirement_(enable_retirement)
    {}

    GameSession(const GameSession&) = delete;
    GameSession& operator=(const GameSession&) = delete;

    GameSession(GameSession&&) = default;
    GameSession& operator=(GameSession&&) = delete;

    [[nodiscard]] const Id& GetId() const noexcept {
        return id_;
    }

    [[nodiscard]] std::string_view GetName() const noexcept { return
        name_; }

    // Creates new Dog if needed
    [[nodiscard]] Dog* RequestDog(std::uint32_t dog_id, std::string_view dog_name, bool restored = false);

    void AddRestoredDog(model::Dog&& dog) {
        dogs_.emplace(dog.GetId(), std::move(dog));
    }

    [[nodiscard]] const Map* GetMap() const noexcept {
        return map_;
    }

    [[nodiscard]] Dog* FindDog(std::uint32_t dog_id) noexcept;

    [[nodiscard]] const Dog* FindDog(std::uint32_t dog_id) const noexcept;

    [[nodiscard]] const std::map<std::uint32_t, Dog>& GetDogs() const noexcept {
        return dogs_;
    }

    void UpdateGameState(std::chrono::milliseconds time_delta_ms);

    http_server::Strand& GetStrand() noexcept {
        return strand_;
    }

    [[nodiscard]] const loot::LootStorage& GetLootStorage() const {
        return loot_storage_;
    }

    [[nodiscard]] loot::LootStorage& GetLootStorage() {
        return loot_storage_;
    }

    [[nodiscard]] const std::map<int, const loot::LootObject*> GetLootNotCollected() const;

    [[nodiscard]] loot::LootObject* FindLootByID(int loot_object_id) {
        return loot_storage_.FindLootByID(loot_object_id);
    }

    [[nodiscard]] const loot::LootObject* FindLootByID(int loot_object_id) const {
        return loot_storage_.FindLootByID(loot_object_id);
    }

    void CreateBots() {
        bot_manager_.CreateBots();
    }

    [[nodiscard]] const std::map<uint32_t, Dog>& GetBots() const noexcept {
        return bot_manager_.GetBots();
    }

    [[nodiscard]] const std::shared_ptr<extra_data::GameExtraData> GetGameExtraData() const {
        return game_extra_data_;
    }

    DogDeletedSignal& GetDogDeletedSignal() {
        return on_dog_deleted_;
    }

private:
    Id id_;
    std::string name_;
    const Map* map_ = nullptr;

    boost::asio::io_context &ioc_;
    // for better performance with many GameSession's
    http_server::Strand strand_;

    // original Dog data
    std::map<std::uint32_t, Dog> dogs_;

    // GameExtraData
    std::shared_ptr<extra_data::GameExtraData> game_extra_data_;
    // LootGenerator for Loot processing
    std::shared_ptr<loot_gen::LootGenerator> loot_generator_;
    // original Loot data
    loot::LootStorage loot_storage_;

    BotManager bot_manager_;
    DogDeletedSignal on_dog_deleted_;

    bool enable_retirement_ = true;

    void ProcessCollisions(std::chrono::milliseconds /*time_delta_ms*/,
                          const std::vector<app_geom::Position2D>& start_positions,
                          const std::vector<Dog*>& dog_ptrs);

    // Dogs retirement process
    void UpdateDogsIdleTime(std::chrono::milliseconds time_delta_ms);

    BotWorldState BuildBotWorldState() const;

    void RetireDog(std::uint32_t dog_id);

    void ReleaseRetiredBag(Dog* dog);
};

} // namespace model
