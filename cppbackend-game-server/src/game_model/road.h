#pragma once

#include "../common/game_utils/geometry.h"

namespace model {

    class Road {
        struct HorizontalTag {
            explicit HorizontalTag() = default;
        };

        struct VerticalTag {
            explicit VerticalTag() = default;
        };

    public:
        constexpr static HorizontalTag HORIZONTAL{};
        constexpr static VerticalTag VERTICAL{};

        Road(HorizontalTag, model_geom::Point2D start, model_geom::Coord end_x) noexcept
            : start_{start}
        , end_{end_x, start.y} {
        }

        Road(VerticalTag, model_geom::Point2D start, model_geom::Coord end_y) noexcept
            : start_{start}
        , end_{start.x, end_y} {
        }

        bool IsHorizontal() const noexcept {
            return start_.y == end_.y;
        }

        bool IsVertical() const noexcept {
            return start_.x == end_.x;
        }

        model_geom::Point2D GetStart() const noexcept {
            return start_;
        }

        model_geom::Point2D GetEnd() const noexcept {
            return end_;
        }

    private:
        model_geom::Point2D start_;
        model_geom::Point2D end_;
    };

} // namespace model