#include "transport_router.h"

namespace transport_router {

void TransportRouter::InitializeGraphAndCreateRouter()
{
    size_t vertex_num = catalogue_.GetStopList().size() * 2;
    graph_ = graph::DirectedWeightedGraph<double>(vertex_num);
    vertex_data_.reserve(vertex_num);
    FillGraphVertices();
    FillGraphEdges();
    graph_initialized = true;
    graph_router_ = std::make_unique<graph::Router<double>>(graph_);
}

void TransportRouter::FillGraphVertices()
{
    for (const auto& stop : catalogue_.GetStopList()) {
        const auto& stop_name = catalogue_.GetStop(stop)->name;
        // Create wait vertex (waiting at stop)
        auto wait_vertex_id = SetVertexForGraph(stop_name, VertexType::WAIT);
        // Create stop vertex (being on bus)
        auto stop_vertex_id = SetVertexForGraph(stop_name, VertexType::STOP);

        // Add edge wait vertex - stop vertex (waiting at Stop to board bus)
        SetRouteEdgeForGraph(wait_vertex_id, stop_vertex_id, CalculateRouteSpanWeight(WAIT_BUS, ""), WAIT_BUS);
    }
}

void TransportRouter::FillGraphEdges()
{
    for (const auto& route_name : catalogue_.GetRouteList()) {
        const auto route = catalogue_.GetRoute(route_name);
        const auto& stops = route->description;
        auto stop_iter_route = stops.begin();
        auto stop_iter_end = stops.begin() + route->route_one_way_size -1;   // End if Two-Way Route
        if (route->is_roundtrip) {
            stop_iter_end = stops.end() -1;         // End if Round-Route
        }
        bool round_route_begin = false;
        size_t two_way_route_segment_count = 0;
        while (stop_iter_route != stop_iter_end) {
            if (route->is_roundtrip) {
                stop_iter_route == stops.begin() ? round_route_begin = true : round_route_begin = false;
                FillGraphEdgesRoundRoute(stop_iter_route, stop_iter_end, route->id, round_route_begin);
            } else {
                FillGraphEdgesTwoWayRoute(stop_iter_route, stop_iter_end, route->id, two_way_route_segment_count);
                ++two_way_route_segment_count;
            }
            ++stop_iter_route;
        }
    }
}

void TransportRouter::FillGraphEdgesRoundRoute(std::vector<const Stop*>::const_iterator stop_iter,
                                               std::vector<const Stop*>::const_iterator stop_iter_end, std::string_view bus, bool round_route_begin)
{
    auto stop_iter_circle_end = round_route_begin ? stop_iter_end : stop_iter_end +1;
    size_t span_count = 0;
    double distance_weight = 0;
    auto stop_iter_begin = stop_iter;
    auto stop_iter_next = stop_iter_begin +1;
    while (stop_iter_next != stop_iter_circle_end) {
        // auto span_weight = CalculateRouteSpanWeight((*stop_iter_previous)->name, (*stop_iter)->name);
        // if (span_weight > 0) {
            distance_weight += CalculateRouteSpanWeight((*stop_iter)->name, (*stop_iter_next)->name);
            ++span_count;
        // }
        SetRouteEdgeForGraph(stop_vertex_[(*stop_iter_begin)->name], stop_vertex_wait_[(*stop_iter_next)->name], distance_weight, bus, span_count);
        ++stop_iter_next;
        ++stop_iter;
    }

}

void TransportRouter::FillGraphEdgesTwoWayRoute(std::vector<const Stop*>::const_iterator stop_iter,
                                                std::vector<const Stop*>::const_iterator stop_iter_end, std::string_view bus, size_t segment_count)
{
    auto left_half_begin = stop_iter;
    auto left_half_next_stop = stop_iter +1;
    auto right_half_begin = stop_iter_end + segment_count;
    auto right_half_next_stop = right_half_begin +1;
    size_t span_count = 0;
    double distance_weight_left_half = 0;
    double distance_weight_right_half = 0;
    while (stop_iter != stop_iter_end) {
        ++span_count;
        distance_weight_left_half += CalculateRouteSpanWeight((*stop_iter)->name, (*left_half_next_stop)->name);
        SetRouteEdgeForGraph(stop_vertex_[(*left_half_begin)->name], stop_vertex_wait_[(*left_half_next_stop)->name], distance_weight_left_half, bus, span_count);
        distance_weight_right_half += CalculateRouteSpanWeight((*(right_half_next_stop-1))->name, (*right_half_next_stop)->name);
        SetRouteEdgeForGraph(stop_vertex_[(*right_half_begin)->name], stop_vertex_wait_[(*right_half_next_stop)->name], distance_weight_right_half, bus, span_count);
        ++left_half_next_stop;
        ++right_half_next_stop;
        ++stop_iter;
    }
}

const std::optional<RouteData> TransportRouter::GetRouteData(std::string_view stop_from, std::string_view stop_to) const
{
    if (!graph_initialized) {
        throw std::logic_error("TransportRouter::GetRouteData: Graph NOT initialized! - NO Route possible!");
    }
    auto vertex_from = stop_vertex_wait_.at(stop_from);
    auto vertex_to = stop_vertex_wait_.at(stop_to);
    auto route_data = graph_router_->BuildRoute(vertex_from, vertex_to);
    if (!route_data) {
        return std::nullopt;
    }
    return RouteData{route_data->weight, ProceesRouteDataItems(route_data->edges)};
}

graph::VertexId TransportRouter::SetVertexForGraph(std::string_view stop_name, VertexType vertex_type)
{
    GraphVertex vertex;
    vertex.id = vertex_data_.size();
    vertex.name = stop_name;
    vertex.type = vertex_type;
    vertex_data_.emplace_back(vertex);
    if (vertex_type == VertexType::STOP) {
        stop_vertex_[stop_name] = vertex.id;
    } else if (vertex_type == VertexType::WAIT) {
        stop_vertex_wait_[stop_name] = vertex.id;
    }
    return vertex_data_.back().id;
}

void TransportRouter::SetRouteEdgeForGraph(graph::VertexId from, graph::VertexId to, double distance_weight, std::string_view bus, size_t span_count)
{
    route_edge_data_[graph_.AddEdge({from, to, distance_weight})] = {&vertex_data_[from], &vertex_data_[to], distance_weight, bus, span_count};
}

double TransportRouter::CalculateRouteSpanWeight(std::string_view stop_name_from, std::string_view stop_name_to)
{
    if (stop_name_from == WAIT_BUS) {
        return 1.0 * routing_settings_.bus_wait_time;
    }
    static const double weight_coef = 1.0/1000*60;      // real distance between Stops in meters, bus_velocity in km/h
    return weight_coef * catalogue_.GetRealDistanceForStop(stop_name_from, stop_name_to) / routing_settings_.bus_velocity;
}

std::vector<const GraphEdge*> TransportRouter::ProceesRouteDataItems(const std::vector<graph::EdgeId>& edges) const
{
    std::vector<const GraphEdge*> result;
    for (const auto& item : edges) {
        auto item_data = route_edge_data_.find(item);
        if (item_data == route_edge_data_.end()) {
            throw std::logic_error("TransportRouter::ProceesRouteDataItems: NO Edge found");
            }
        result.emplace_back(&item_data->second);
    }
    return result;
}

} // namespace transport_router
