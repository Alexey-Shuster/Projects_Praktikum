#include "loot_storage.h"

#include "../common/utils.h"

namespace loot {

    void LootStorage::GenerateLoots(size_t loot_count,
                                 const std::vector<model::Road>& roads,
                                 const std::vector<extra_data::LootData>& loot_types,
                                 std::function<double()> random_gen) {
        if (loot_count <= 0 || roads.empty() || loot_types.empty()) return;

        auto& rng = GetRandomEngine();
        std::uniform_real_distribution<double> unit_dist(0.0, 1.0);
        auto rand_double = random_gen ? random_gen : [&]() { return unit_dist(rng); };

        for (int i = 0; i < loot_count; ++i) {
            // Random road
            std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
            const auto& road = roads[road_dist(rng)];

            // Random point on road (ensure min <= max for uniform_real_distribution)
            app_geom::Position2D pos;
            if (road.IsHorizontal()) {
                pos = {utils::GetRandomNumber(road.GetStart().x, road.GetEnd().x, rng),
                            utils::Point2DToPosition2D(road.GetStart()).y};
            } else {
                pos = {utils::Point2DToPosition2D(road.GetStart()).x,
                        utils::GetRandomNumber(road.GetStart().y, road.GetEnd().y, rng)};
            }

            // Random loot type (by index)
            std::uniform_int_distribution<int> type_dist(0, loot_types.size() - 1);
            int type_idx = type_dist(rng);

            LootObject obj;
            obj.object_id = next_loot_object_id_++;
            obj.pos = pos;
            obj.loot_data_ptr = &loot_types[type_idx]; // указатель на описание

            loots_.emplace(obj.object_id, std::move(obj));
        }
    }

} // namespace loot