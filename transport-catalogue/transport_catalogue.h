#pragma once

#include "geo.h"

#include <unordered_set>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct Stop {
    std::string name{};                 // Stop name
    geo::Coordinates position;          // Stop coordinates
    std::set<std::string_view> buses{}; // Buses going through this Stop

    bool operator == (const Stop& other) const {
        return name == other.name &&
               position == other.position;
    }

    explicit operator bool() const {
        return !name.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    friend std::ostream& operator << (std::ostream& out, const Stop& item) {
        if (item.buses.empty()) {
            out << "Stop " << item.name << ": no buses";
        } else {
            out << "Stop " << item.name << ", buses:";
            for (const auto& item : item.buses) {
                out << " " << item;
            }
        }
        return out;
    }
};

struct Route {
    std::string id{};                       // Route id
    std::vector<const Stop*> description{}; // Route description
    size_t stops_num = 0;                   // Stops on route
    size_t unique_stops_num = 0;            // Unique Stops on route
    int route_length = 0;                   // Route real distance - computed with Stops distances - input in meters
    double curvature = 0;                   // Route curvature = route_length_real / route_length_geo
    bool is_roundtrip;                      // Route is roundtrip or not
    std::string_view route_start{};         // start Stop of Route - need for Renderer
    std::string_view route_end{};           // end Stop of Route (if NOT roundtrip = one-way end, else = start Stop) - need for
    size_t route_one_way_size;              // if NOT roundtrip = one-way size, else = all stops - no need for look-up while Graph edge creation

    bool operator == (const Route& other) const {
        return id == other.id &&
               description == other.description;
    }

    explicit operator bool() const {
        return !id.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    friend std::ostream& operator << (std::ostream& out, const Route& item) {
        out << "Bus " << item.id << ": " << item.stops_num << " stops on route, " << item.unique_stops_num << " unique stops, "
            << item.route_length << " route length, " << item.curvature << " curvature";
        return out;
    }
};

struct HashStringViewPair {
    size_t operator()(const std::pair<std::string_view, std::string_view>& p) const {
        int num = 37;
        return (std::hash<std::string_view>{}(p.first) +
                (std::hash<std::string_view>{}(p.second) * num));
    }
};


class TransportCatalogue {
public:
    void AddStop (const Stop& stop);
    void AddRoute (Route& route);

    const Stop* GetStop (const std::string_view name) const;
    const Route* GetRoute (const std::string_view id) const;

    const Route* GetRouteStat (const std::string_view id) const;
    const Stop* GetStopStat (const std::string_view name) const;

    void SetRealDistanceForStop (const std::string_view stop_name_start, const std::string_view stop_name_end, int distance);
    int GetRealDistanceForStop (const std::string_view stop_start, const std::string_view stop_end) const;

    //returns Routes if it contains any Stops
    const std::set<std::string_view> GetRouteList() const;
    //returns Stops if any Route contains it (any bus go through)
    const std::set<std::string_view> GetStopList() const;

    void PrintCatalogue() const;
    void PrintCatalogueStat() const;

private:
    std::unordered_map<std::string, Stop> stops_;
    std::unordered_map<std::string, Route> routes_;

    using StopNameStart = std::string_view;
    using StopNameEnd = std::string_view;
    std::unordered_map<std::pair<StopNameStart, StopNameEnd>, int, HashStringViewPair> stops_real_dist_;
    using RouteLength = int;
    using RouteCurvature = double;
    std::pair<RouteLength, RouteCurvature> ComputeRouteLengthAndCurvature (const Route& route) const;
    void AddBusForStop(const Route& route);
};
