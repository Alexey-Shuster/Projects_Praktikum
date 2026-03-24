#include "dog.h"

namespace model {

    void Dog::SetDirection(app_geom::Direction2D dir) {
        speed_= utils::Speed2DFromDirection2D(dir, map_->GetDefaultSpeed());
        if (speed_ != app_geom::Speed2D::Zero()) {
            dir_ = dir;
        }
    }

    void Dog::Move(std::chrono::milliseconds time_delta_ms) {
        auto time_delta = std::chrono::duration<double>(time_delta_ms).count();

        // If speed is zero, no movement
        if (speed_ == app_geom::Speed2D::Zero()) {
            return;
        }

        if (!road_to_move_) {
            std::string error_message = "Dog::Move: road_to_move = Nullptr";
            boost_logger::LogInfo(error_message);
            throw std::runtime_error(error_message);
        }
        // Save current road in case target unreachable
        auto current_road = road_to_move_;

        // Calculate target position based on speed and time
        app_geom::Position2D target = {
            pos_.x + speed_.x * time_delta,
            pos_.y + speed_.y * time_delta
        };

        // Check if road at starting position still applicable for this move
        if (!road_to_move_ || !map_->GetRoadEngine().IsPositionOnRoad(target, road_to_move_)) {
            // Find road for movement
            road_to_move_ = map_->GetRoadEngine().ChooseRoadForMovement(pos_, target, dir_);
        }

        // If no road exists for target (unreachable target)
        if (!road_to_move_) {
            // Clamp movement to road bounds (one road per move)
            pos_ = map_->GetRoadEngine().ClampPositionToRoadBounds(target, current_road);
            speed_ = app_geom::Speed2D::Zero();
            // Keep current road
            road_to_move_ = current_road;
        } else {
            pos_ = target;
        }
        // boost_logger::LogInfo("Dog::Move: " + name_ + " new Postion " + pos_.ToString());
    }

} // namespace model
