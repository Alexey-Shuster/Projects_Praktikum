#pragma once

#include <cmath>
#include <iostream>

namespace geo {

struct Coordinates {
    double lat;     //Широта
    double lng;     //Долгота
    bool operator==(const Coordinates& other) const {
        return lat == other.lat && lng == other.lng;
    }
    bool operator!=(const Coordinates& other) const {
        return !(*this == other);
    }
    friend std::ostream& operator << (std::ostream& out, const Coordinates& item);
};

std::ostream& operator << (std::ostream& out, const Coordinates& item);

double ComputeDistance(Coordinates from, Coordinates to);

} //namespace geo
