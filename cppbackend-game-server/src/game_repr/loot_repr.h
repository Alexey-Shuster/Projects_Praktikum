#pragma once

#include <boost/serialization/access.hpp>
#include <ranges>
#include <vector>
#include <unordered_map>

#include "../game_model/loot_storage.h"
#include "../game_model/game_extra_data.h"

namespace serialize_game_save {

    // Representation of a Loot object
    struct LootObjectRepr {
        loot::LootObjectId object_id{0};
        double pos_x = 0.0;
        double pos_y = 0.0;
        int loot_type_index{0};    // index into map's loot types vector
        bool collected = false;

    private:
        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned const /*version*/) {
            ar & object_id;
            ar & pos_x;
            ar & pos_y;
            ar & loot_type_index;
            ar & collected;
        }
    };

    class LootStorageRepr {
    public:
        LootStorageRepr() = default;

        explicit LootStorageRepr(const loot::LootStorage& storage,
                                  const model::Map::Id& map_id,
                                  const std::shared_ptr<extra_data::GameExtraData>& extra_data)
        {
            next_id_ = storage.GetNextId();
            const auto& loot_types = extra_data->GetLootTypes(map_id);

            for (const auto& obj : storage.GetLootObjects() | std::views::values) {
                LootObjectRepr repr;
                repr.object_id = obj.object_id;
                repr.pos_x = obj.pos.x;       // assume .x, .y members
                repr.pos_y = obj.pos.y;
                repr.collected = obj.collected;

                // Find index of loot_data_ptr in loot_types vector
                auto it = std::find_if(loot_types.begin(), loot_types.end(),
                                       [ptr = obj.loot_data_ptr](const extra_data::LootData& ld) {
                                           return &ld == ptr;
                                       });
                if (it != loot_types.end()) {
                    repr.loot_type_index = static_cast<int>(std::distance(loot_types.begin(), it));
                } else {
                    repr.loot_type_index = -1; // should not happen
                }
                loots_.push_back(repr);
            }
        }

        void Restore(loot::LootStorage& storage,
                     const std::vector<extra_data::LootData>& loot_types,
                     std::unordered_map<loot::LootObjectId, loot::LootObject*>& id_to_loot) const {
            storage.Clear();
            storage.SetNextId(next_id_);

            for (const auto& repr : loots_) {
                loot::LootObject obj;
                obj.object_id = repr.object_id;
                obj.pos = app_geom::Position2D{repr.pos_x, repr.pos_y};   // reconstruct
                obj.collected = repr.collected;
                if (repr.loot_type_index >= 0 && repr.loot_type_index < static_cast<int>(loot_types.size())) {
                    obj.loot_data_ptr = &loot_types[repr.loot_type_index];
                }
                auto* ptr = storage.AddLootObject(std::move(obj));
                id_to_loot[repr.object_id] = ptr;
            }
        }

    private:
        int next_id_ = 0;
        std::vector<LootObjectRepr> loots_;

        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned const /*version*/) {
            ar & next_id_;
            ar & loots_;
        }
    };

} // namespace serialize_game_save