#include "transport_catalogue.h"

void TransportCatalogue::AddStop(const Stop& stop) {
    stops_[stop.name] = stop;
}

void TransportCatalogue::AddRoute(Route& route) {
    auto route_length_curv = ComputeRouteLengthAndCurvature(route);
    route.route_length = route_length_curv.first;
    route.curvature = route_length_curv.second;
    routes_[route.id] = route;

    AddBusForStop(route);
}

std::pair<TransportCatalogue::RouteLength, TransportCatalogue::RouteCurvature> TransportCatalogue::ComputeRouteLengthAndCurvature (const Route &route) const {
    double distance_geo = 0;
    int distance_real = 0;
    auto previous_stop = route.description.front();
    bool first_iteration = true;
    for (const auto& item : route.description) {
        if (first_iteration) {
            first_iteration = false;
            continue;
        }
        auto current_stop = GetStop(item->name);

        distance_geo += ComputeDistance(previous_stop->position, current_stop->position);
        distance_real += GetRealDistanceForStop(previous_stop->name, current_stop->name);
        previous_stop = current_stop;
    }
    return {distance_real, distance_real / distance_geo};
}

const Stop* TransportCatalogue::GetStop(const std::string_view name) const {
    auto iter = stops_.find(std::string(name));
    if (iter != stops_.end()) {
        return &iter->second;
    }
    return nullptr;
}

const Route* TransportCatalogue::GetRoute(const std::string_view id) const {
    auto iter = routes_.find(std::string(id));
    if (iter != routes_.end()) {
        return &iter->second;
    }
    return nullptr;
}

const Route* TransportCatalogue::GetRouteStat(const std::string_view id) const {
    return GetRoute(id);
}

const Stop* TransportCatalogue::GetStopStat(const std::string_view name) const {
    return GetStop(name);
}

void TransportCatalogue::SetRealDistanceForStop(const std::string_view stop_name_start, const std::string_view stop_name_end, int distance)
{
    const std::string_view& stop_from_name = GetStop(stop_name_start)->name;
    const std::string_view& stop_to_name = GetStop(stop_name_end)->name;
    // cout << "SetRealDistanceForStop->" << stop_from_name << "/" << stop_to_name << "/" << distance << endl;
    auto stop_pair = std::make_pair(stop_from_name, stop_to_name);
    auto stop_pair_reverse = std::make_pair(stop_to_name, stop_from_name);
    stops_real_dist_[stop_pair] = distance;                     //forward direction always saved
    stops_real_dist_.insert({stop_pair_reverse, distance});     //backward direction saved only if value not found
}

int TransportCatalogue::GetRealDistanceForStop(const std::string_view stop_start, const std::string_view stop_end) const
{
    auto iter = stops_real_dist_.find({stop_start, stop_end});
    if (iter == stops_real_dist_.end()) {
        throw std::logic_error("TransportCatalogue::GetRealDistanceForStop: NO Distance between Stops exists");
    }
    return iter->second;
}

const std::set<std::string_view> TransportCatalogue::GetRouteList() const
{
    std::set<std::string_view> result;
    for (const auto& [route_id, route_value] : routes_) {
        if (route_value.stops_num > 0) {
            result.emplace(route_value.id);
        }
    }
    return result;
}

const std::set<std::string_view> TransportCatalogue::GetStopList() const
{
    std::set<std::string_view> result;
    for (const auto& [stop_id, stop_value] : stops_) {
        if (stop_value.buses.size() > 0) {
            result.emplace(stop_value.name);
        }
    }
    return result;
}

void TransportCatalogue::PrintCatalogue() const {
    for (const auto& stop : stops_) {
        std::cout << "Stop in DataBase->" << stop.second << std::endl;
    }
    for (const auto& route : routes_) {
        std::cout << "Route in DataBase->" << route.second << std::endl;
    }
}

void TransportCatalogue::PrintCatalogueStat() const
{
    auto routes_with_stops = GetRouteList().size();
    auto stops_list = GetStopList();
    size_t stops_multi_route = 0;
    for (const auto& item : stops_list) {
        (GetStop(item)->buses.size() > 1) ? ++stops_multi_route : 0;
        // if (GetStop(item)->buses.size() > 1) {
        //     ++stops_multi_route;
        // }
    }
    std::cout << "TransportCatalogue::PrintCatalogueStat: Routes_with_stops: " << routes_with_stops << std::endl;
    std::cout << "TransportCatalogue::PrintCatalogueStat: Stops_with_buses: " << stops_list.size() << ", where multi-stops: " << stops_multi_route << std::endl;
}

void TransportCatalogue::AddBusForStop(const Route &route)
{
    const std::string& bus_id = GetRoute(route.id)->id;
    std::unordered_set<std::string_view> unique_stops{};
    for (const auto& stop : route.description) {
        if (unique_stops.find(stop->name) != unique_stops.end()) {continue;}
        stops_.at(stop->name).buses.insert(bus_id);             //adding buses to Stop
        unique_stops.insert(stop->name);                        //to avoid extra loops for already processed Stop
    }
}
