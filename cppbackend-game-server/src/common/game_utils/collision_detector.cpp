#include "collision_detector.h"

#include <algorithm>
#include <cassert>

namespace collision_detector {

CollectionResult TryCollectPoint(app_geom::Position2D a, app_geom::Position2D b, app_geom::Position2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}


std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;
    size_t gatherers_count = provider.GatherersCount();
    size_t items_count = provider.ItemsCount();

    for (size_t g = 0; g < gatherers_count; ++g) {
        Gatherer gatherer = provider.GetGatherer(g);
        // Skip if the gatherer didn't move
        if (gatherer.start_pos == gatherer.end_pos) {
            continue;
        }

        double total_radius = gatherer.width; // will add item width later
        for (size_t i = 0; i < items_count; ++i) {
            Item item = provider.GetItem(i);
            double combined = total_radius + item.width;

            CollectionResult res = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);
            if (res.proj_ratio >= 0.0 && res.proj_ratio <= 1.0 && res.sq_distance <= combined * combined) {
                events.push_back({i, g, res.sq_distance, res.proj_ratio});
            }
        }
    }

    // Sort by time (chronological order)
    std::sort(events.begin(), events.end(),
              [](const GatheringEvent& a, const GatheringEvent& b) {
                  return a.time < b.time;
              });
    return events;
}

}  // namespace collision_detector
