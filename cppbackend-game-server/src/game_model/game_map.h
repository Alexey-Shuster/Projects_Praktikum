#pragma once

#include <random>
#include <stdexcept>
#include <string>

#include "road_engine/road_engine.h"
#include "../common/tagged.h"

namespace model {

class Building {
public:
    explicit Building(model_geom::Rectangle2D bounds) noexcept
        : bounds_{bounds} {
    }

    [[nodiscard]] const model_geom::Rectangle2D& GetBounds() const noexcept {
        return bounds_;
    }

private:
    model_geom::Rectangle2D bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, model_geom::Point2D position, model_geom::Offset2D offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    [[nodiscard]] const Id& GetId() const noexcept {
        return id_;
    }

    [[nodiscard]] model_geom::Point2D GetPosition() const noexcept {
        return position_;
    }

    [[nodiscard]] model_geom::Offset2D GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    model_geom::Point2D position_;
    model_geom::Offset2D offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    void BuildRoadIndex() {
        road_engine_.BuildIndex(roads_);
    }

    const RoadEngine& GetRoadEngine() const { return road_engine_; }

    const Road& GetZeroRoad() const {
        return *roads_.begin();
    }

    void SetDefaultSpeed(app_geom::Speed2D speed) { default_map_speed_ = speed; }
    void SetDefaultCapacity(int capacity) { default_bag_capacity_ = capacity; }

    const app_geom::Speed2D& GetDefaultSpeed() const {
        return (default_map_speed_ != app_geom::Speed2D::Zero()) ?
                           default_map_speed_ : throw std::runtime_error("Default speed is Zero."); }

    int GetDefaultCapacity() const {
        return (default_bag_capacity_ != 0) ?
                           default_bag_capacity_ : throw std::runtime_error("Default capacity is Zero."); }

    [[nodiscard]] app_geom::Position2D GetRandomPoint() const {
        if (roads_.empty()) {
            throw std::runtime_error("No roads on map to generate random point");
        }

        // Random device and generator (static to avoid re-seeding on every call)
        static std::random_device rd;
        static std::mt19937 gen(rd());

        // Choose a random road
        std::uniform_int_distribution<size_t> road_dist(0, roads_.size() - 1);
        const Road& road = roads_[road_dist(gen)];

        // Get start and end points
        model_geom::Point2D start = road.GetStart();
        model_geom::Point2D end = road.GetEnd();

        // Random interpolation factor t in [0, 1]
        std::uniform_real_distribution<double> t_dist(0.0, 1.0);
        double t = t_dist(gen);

        // Linear interpolation (coordinates are integer, but we return floating point)
        double x = start.x + t * (end.x - start.x);
        double y = start.y + t * (end.y - start.y);

        return app_geom::Position2D{x, y};
    }

private:
    using OfficeIdHasher = util::TaggedHasher<Office::Id>;
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, OfficeIdHasher>;

    Id id_;
    std::string name_;
    app_geom::Speed2D default_map_speed_ = app_geom::Speed2D::Zero();
    int default_bag_capacity_{0};
    Roads roads_;
    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    RoadEngine road_engine_;
};

} // namespace model
