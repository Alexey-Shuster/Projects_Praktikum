#pragma once

#include <compare>  // Required for <=> "Spaceship Operator"
#include <iomanip>
#include <sstream>
#include <string>

namespace model_geom {

    using Dimension = int;
    using Coord = Dimension;

    struct Point2D {
        Coord x{0}, y{0};
        constexpr Point2D() = default;
        constexpr Point2D(Coord x, Coord y)
            : x(x)
            , y(y)
        {}
        auto operator<=>(const Point2D&) const = default;
        static constexpr Point2D Zero() { return {0, 0}; }

        // Convert Point to string representation
        [[nodiscard]] std::string ToString() const {
            std::ostringstream oss;
            oss << "(" << x << ", " << y << ")";
            return oss.str();
        }
    };

    inline std::string PointToString(const Point2D& point) {
        return point.ToString();
    }

    struct Size2D {
        Dimension width, height;
    };

    struct Rectangle2D {
        Point2D position{};
        Size2D size{};
    };

    struct Offset2D {
        Dimension dx, dy;
    };

} // namespace model_geom

namespace app_geom {

    using Value = double;
    using Coord = Value;

    struct Vec2D {
        Coord x{0.0}, y{0.0};
        Vec2D() = default;
        Vec2D(Coord x, Coord y)
            : x(x)
            , y(y) {
        }

        Vec2D& operator*=(double scale) {
            x *= scale;
            y *= scale;
            return *this;
        }

        auto operator<=>(const Vec2D&) const = default;
    };

    inline Vec2D operator*(Vec2D lhs, double rhs) {
        return lhs *= rhs;
    }

    inline Vec2D operator*(double lhs, Vec2D rhs) {
        return rhs *= lhs;
    }

    struct Position2D {
        Coord x{0.0}, y{0.0};
        constexpr Position2D() = default;
        constexpr Position2D(Coord x, Coord y)
            : x(x)
            , y(y)
        {}
        auto operator<=>(const Position2D&) const = default;
        Position2D& operator+=(const Vec2D& rhs) {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }
        static constexpr Position2D Zero() { return {0.0, 0.0}; }

        // Convert Position to string representation with fixed precision
        [[nodiscard]] std::string ToString() const {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6)
                << "(" << x << ", " << y << ")";
            return oss.str();
        }
    };

    struct Position2DHasher {
        std::size_t operator()(const Position2D& p) const noexcept {
            // Combine hashes of x and y using a simple XOR + shift
            std::size_t h1 = std::hash<double>{}(p.x);
            std::size_t h2 = std::hash<double>{}(p.y);
            return h1 ^ (h2 << 1);
        }
    };

    inline std::string PositionToString(const Position2D& position) {
        return position.ToString();
    }

    inline Position2D operator+(Position2D lhs, const Vec2D& rhs) {
        return lhs += rhs;
    }

    inline Position2D operator+(const Vec2D& lhs, Position2D rhs) {
        return rhs += lhs;
    }

    struct Rectangle2DTwoPoints {
        Position2D top_left;
        Position2D bottom_right;
    };

    struct Speed2D {
        Value x{0.0}, y{0.0};
        auto operator<=>(const Speed2D&) const = default;
        static constexpr Speed2D Zero() { return {0.0, 0.0}; }
    };

    // 0=STOP, 1=LEFT, 2=RIGHT, 3=UP, 4=DOWN
    enum class Direction2D {
        STOP = 0,   // = api-request {"move": ""}
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

} // namespace app_geom
