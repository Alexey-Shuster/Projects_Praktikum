#pragma once

#include <cstdint>  // std::uint32_t

#include "loot_storage.h"
#include "../common/utils.h"

namespace model {

    class Dog {
    public:
        using Id = uint32_t;
        using Bag = std::vector<loot::LootObject*>;

        explicit Dog(std::uint32_t id, std::string name, const Map* map)
            : id_(id)
            , name_(std::move(name))
            , map_(map)
            , road_to_move_(&map_->GetZeroRoad())
        {
            pos_ = utils::Point2DToPosition2D(road_to_move_->GetStart());
        }

        Dog(const Dog&) = delete;
        Dog& operator=(const Dog&) = delete;

        Dog(Dog&&) = default;
        Dog& operator=(Dog&&) = default;

        [[nodiscard]] Id GetId() const noexcept {
            return id_;
        }

        [[nodiscard]] const std::string& GetName() const noexcept {
            return name_;
        }

        const Map* GetMap() const noexcept {
            return map_;
        }

        void SetPosition(const app_geom::Position2D& pos) noexcept {
            pos_ = pos;
        }

        void SetSpeed(const app_geom::Speed2D& speed) noexcept {
            speed_ = speed;
        }

        [[nodiscard]] app_geom::Position2D GetPosition() const noexcept {
            return pos_;
        }

        [[nodiscard]] app_geom::Speed2D GetSpeed() const noexcept {
            return speed_;
        }

        [[nodiscard]] app_geom::Direction2D GetDirection() const noexcept {
            return dir_;
        }

        [[nodiscard]] loot::Score GetScore() const noexcept {
            return score_;
        }

        [[nodiscard]] const Bag& GetBag() const noexcept {
            return bag_;
        }

        void SetDirection(app_geom::Direction2D dir);

        void Move(std::chrono::milliseconds time_delta_ms);

        void AddToBag(loot::LootObject* loot_data) {
            bag_.push_back(loot_data);
        }

        void ClearBag() {
            bag_.clear();
        }

        void AddScore(loot::Score delta) {
            score_ += delta;
        }

        void SetCurrentRoad(const Road* road) {
            road_to_move_ = road;
        }

        [[nodiscard]] const Road* GetCurrentRoad() const {
            return road_to_move_;
        }

        [[nodiscard]] std::chrono::milliseconds GetIdleTime() const noexcept {
            return idle_time_;
        }

        void UpdateIdleTime(std::chrono::milliseconds time_delta_ms) {
            idle_time_ += time_delta_ms;
        }

        void ResetIdleTime() {
            idle_time_ = std::chrono::milliseconds(0);
        }

    private:
        Id id_;             // same as for Player
        std::string name_;  // same as for Player
        const Map* map_ = nullptr;
        const Road* road_to_move_ = nullptr;
        app_geom::Position2D pos_{app_geom::Position2D::Zero()};
        app_geom::Speed2D speed_{app_geom::Speed2D::Zero()};
        app_geom::Direction2D dir_{app_geom::Direction2D::UP};
        Bag bag_{};
        loot::Score score_{0};
        std::chrono::milliseconds idle_time_{0};
    };

} // namespace model