#pragma once

#include "geometry.h"

#include <algorithm>
#include <vector>

namespace collision_detector {

struct CollectionResult {
    [[nodiscard]] bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }

    // квадрат расстояния до точки
    double sq_distance;

    // доля пройденного отрезка
    double proj_ratio;
};

// Движемся из точки a в точку b и пытаемся подобрать точку c.
CollectionResult TryCollectPoint(app_geom::Position2D a, app_geom::Position2D b, app_geom::Position2D c);

struct Item {
    app_geom::Position2D position;
    double width;
};

struct Gatherer {
    app_geom::Position2D start_pos;
    app_geom::Position2D end_pos;
    double width;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;

public:
    [[nodiscard]] virtual size_t ItemsCount() const = 0;
    [[nodiscard]] virtual Item GetItem(size_t idx) const = 0;
    [[nodiscard]] virtual size_t GatherersCount() const = 0;
    [[nodiscard]] virtual Gatherer GetGatherer(size_t idx) const = 0;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

// ItemGatherer implements ItemGathererProvider using vectors
class ItemGatherer : public ItemGathererProvider {
public:
    void AddItem(const Item &item) {
        items_.push_back(item);
    }

    void AddGatherer(const Gatherer &gatherer) {
        gatherers_.push_back(gatherer);
    }

    [[nodiscard]] size_t ItemsCount() const override {
        return items_.size();
    }
    [[nodiscard]] Item GetItem(size_t idx) const override {
        return items_.at(idx);
    }
    [[nodiscard]] size_t GatherersCount() const override {
        return gatherers_.size();
    }
    [[nodiscard]] Gatherer GetGatherer(size_t idx) const override {
        return gatherers_.at(idx);
    }

private:
    std::vector<Item> items_;
    std::vector<Gatherer> gatherers_;
};

// Helper to sort events by time, then gatherer_id, then item_id for deterministic comparison
static void SortEvents(std::vector<GatheringEvent>& events) {
    std::sort(events.begin(), events.end(),
              [](const GatheringEvent& a, const GatheringEvent& b) {
                  if (a.time != b.time) {
                      return a.time < b.time;
                  };
                  if (a.gatherer_id != b.gatherer_id) {
                      return a.gatherer_id < b.gatherer_id;
                  };
                  return a.item_id < b.item_id;
              });
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector
