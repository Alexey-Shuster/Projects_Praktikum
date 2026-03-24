#pragma once

#include <chrono>
#include <optional>
#include <string>

#include "game_map.h"
#include "../common/constants.h"

namespace extra_data {

    using LootDataType = uint32_t;

    struct LootGeneratorConfig {
        std::chrono::milliseconds period{common_values::DEFAULT_LOOT_CONFIG_PERIOD};
        double probability{common_values::DEFAULT_LOOT_CONFIG_PROBABILITY};
    };

    struct LootData {
        model::Map::Id map_id{"NoMap"};
        LootDataType type_id{0};
        std::string name;
        std::string file;
        std::string type;
        std::optional<int> rotation;
        std::optional<std::string> color;
        double scale{0.1};
        int value{0};
    };

    class GameExtraData {
    public:
        using MapsSpeed = std::unordered_map<model::Map::Id, app_geom::Speed2D, util::TaggedHasher<model::Map::Id>>;
        using MapsBagCapacity = std::unordered_map<model::Map::Id, int, util::TaggedHasher<model::Map::Id>>;

        void AddMapsSpeed(MapsSpeed&& maps_speed) {
            maps_speed_ = std::move(maps_speed);
        }
        void AddMapsBagCapacity(MapsBagCapacity&& maps_bag_capacity) {
            maps_bag_capacity_ = std::move(maps_bag_capacity);
        }

        const app_geom::Speed2D& GetSpeedForMap(const model::Map::Id &map_id) {
            return maps_speed_[map_id];
        }

        void AddLootGeneratorConfig(LootGeneratorConfig&& loot_generator_config) {
            loot_generator_config_ = std::move(loot_generator_config);
        }

        void AddLootTypes(const model::Map::Id& id, std::vector<LootData>&& loots) {
            loots_for_map_[id] = std::move(loots);
        }

        const std::vector<LootData>& GetLootTypes(const model::Map::Id& id) const {
            static const std::vector<LootData> empty;
            auto it = loots_for_map_.find(id);
            return it != loots_for_map_.end() ? it->second : empty;
        }

        const LootGeneratorConfig& GetLootGeneratorConfig() {
            return loot_generator_config_;
        }

        void SetDogRetirementTime(std::chrono::seconds time) {
            dog_retirement_time_ = time;
        }

        std::chrono::seconds GetDogRetirementTime() const {
            return dog_retirement_time_;
        }

    private:
        MapsSpeed maps_speed_;
        MapsBagCapacity maps_bag_capacity_;
        LootGeneratorConfig loot_generator_config_;
        std::chrono::seconds dog_retirement_time_{common_values::DEFAULT_DOG_RETIREMENT_TIME_SEC};

        std::unordered_map<model::Map::Id, std::vector<LootData>, util::TaggedHasher<model::Map::Id>> loots_for_map_;
    };

} // namespace extra_data