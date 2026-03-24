#pragma once

#include <random>

#include "game_extra_data.h"
#include "../common/game_utils/geometry.h"

namespace loot {

    using LootObjectId = uint32_t;
    using LootObjectType = uint32_t;
    using Score = uint32_t;

    struct LootObject {
        LootObjectId object_id{0};
        app_geom::Position2D pos{app_geom::Position2D::Zero()};
        const extra_data::LootData* loot_data_ptr{nullptr};
        bool collected{false};  // true if already picked up
    };

    class LootStorage {
    public:
        void GenerateLoots(size_t loot_count,
                           const std::vector<model::Road>& roads,
                           const std::vector<extra_data::LootData>& loot_types,
                           std::function<double()> random_gen = nullptr);

        [[nodiscard]] const std::map<int, LootObject>& GetLootObjects() const { return loots_; }

        void RemoveLoot(int loot_object_id) {
            loots_.erase(loot_object_id);
        }

        LootObject* FindLootByID(int loot_object_id) {
            if (auto it = loots_.find(loot_object_id); it != loots_.end()) {
                return &it->second;
            }
            return nullptr;
        }

        [[nodiscard]] const LootObject* FindLootByID(int loot_object_id) const {
            if (auto it = loots_.find(loot_object_id); it != loots_.end()) {
                return &it->second;
            }
            return nullptr;
        }

        [[nodiscard]] int GetNextId() const {
            return next_loot_object_id_;
        }

        void SetNextId(int id) {
            next_loot_object_id_ = id;
        }

        void Clear() {
            loots_.clear();
        }

        loot::LootObject* AddLootObject(LootObject&& obj) {
            if (auto it = loots_.emplace(obj.object_id, std::move(obj)); it.second) {
                return &it.first->second;
            }
            return nullptr;
        }

    private:
        int next_loot_object_id_ = 1;       // unique ID generator
        std::map<int, LootObject> loots_;

        static std::mt19937& GetRandomEngine() {
            thread_local std::mt19937 engine(std::random_device{}());
            return engine;
        }
    };

} // namespace loot
