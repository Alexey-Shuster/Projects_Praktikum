#pragma once

#include "router.h"
#include "transport_catalogue.h"
#include <cstdint>
#include <memory>

namespace transport_router {

struct RoutingSettings {
    uint16_t bus_wait_time; // input in minutes
    double bus_velocity;    // input in km/h
};

enum class VertexType {
    WAIT,
    STOP
};

struct GraphVertex {
    graph::VertexId id;
    std::string_view name;  // same as Stop name
    VertexType type;
};

struct GraphEdge
{
    const GraphVertex* from;
    const GraphVertex* to;
    double distance_weight;     // weight from Route-start to every Stop on Route
    std::string_view bus;
    size_t span_count = 0;      // Stop spans: GraphVertex* from - GraphVertex* to
};

struct RouteData {
    double route_weight;
    std::vector<const GraphEdge*> edges;
};

class TransportRouter {
public:
    TransportRouter(TransportCatalogue& catalogue, const RoutingSettings routing_settings)
        : catalogue_(catalogue), routing_settings_(routing_settings), graph_(), graph_router_(nullptr)
    {
        InitializeGraphAndCreateRouter();
    };

    const std::optional<RouteData> GetRouteData(std::string_view stop_from, std::string_view stop_to) const;

private:
    TransportCatalogue& catalogue_;
    RoutingSettings routing_settings_;
    graph::DirectedWeightedGraph<double> graph_;
    std::vector<GraphVertex> vertex_data_;
    std::unordered_map<graph::EdgeId, GraphEdge> route_edge_data_;    // excluding edge connections Wait-Stop (& Stop-Wait)
    std::unordered_map<std::string_view, size_t> stop_vertex_wait_;   // WAIT-vertices for Stop
    std::unordered_map<std::string_view, size_t> stop_vertex_;        // STOP-vertices for Stop
    std::unique_ptr<graph::Router<double>> graph_router_;
    bool graph_initialized = false;

    const std::string WAIT_BUS = "Wait";
    const double ZERO_WEIGHT = 0.0;

    void InitializeGraphAndCreateRouter();

    // Create: WAIT & STOP vertices, edge connections Wait-Stop (& Stop-Wait)
    void FillGraphVertices();
    void FillGraphEdges();
    void FillGraphEdgesRoundRoute(std::vector<const Stop*>::const_iterator stop_iter,
                                  std::vector<const Stop*>::const_iterator stop_iter_end, std::string_view bus, bool round_route_begin);
    void FillGraphEdgesTwoWayRoute(std::vector<const Stop*>::const_iterator stop_iter,
                                   std::vector<const Stop*>::const_iterator stop_iter_end, std::string_view bus, size_t segment_count);

    graph::VertexId SetVertexForGraph(std::string_view stop_name, VertexType vertex_type);
    void SetRouteEdgeForGraph(graph::VertexId from, graph::VertexId to, double distance_weight, std::string_view bus, size_t span_count = 0);
    double CalculateRouteSpanWeight(std::string_view stop_name_from, std::string_view stop_name_to);

    std::vector<const GraphEdge*> ProceesRouteDataItems (const std::vector<graph::EdgeId>& edges) const;
};

} // namespace transport_router
