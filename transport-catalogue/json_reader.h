#pragma once

#include "json.h"
#include "json_builder.h"
#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <cassert>
#include <sstream>

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace json_reader {

class JSONReader {
public:
    explicit JSONReader(json::Document data_input, TransportCatalogue& catalogue, renderer::MapRenderer& renderer)
        : data_input_(data_input), catalogue_(catalogue), renderer_(renderer), router_(nullptr) {};

    const json::Document GetData() const;

    void ApplyCommandsBase();
    json::Document PrintStat() const;
    //Debug
    void PrintRouteRequestStat(bool only_general_data = true);

private:
    json::Document data_input_;
    TransportCatalogue& catalogue_;
    renderer::MapRenderer& renderer_;
    std::unique_ptr<transport_router::TransportRouter> router_;
    transport_router::RoutingSettings routing_settings_;
    std::unordered_map<std::pair<std::string_view, std::string_view>, size_t, HashStringViewPair> route_request_count_;     //Debug

    using StopName = std::string_view;
    using StopDistances = std::unordered_map<std::string_view, int>;

    void ApplyCommandsBaseForStop(const json::Dict& request_data, std::unordered_map<StopName, StopDistances>& stops_distances);
    void ApplyCommandsBaseForBus(const json::Dict& request_data);
    void ApplyCommandsBaseForBusNoRoundtrip(std::vector<std::string_view>& stops_on_current_route);

    void ApplyCommandsBaseForStopsDistances(const std::unordered_map<StopName, StopDistances>& stops_distances);
    void ApplyCommandsBaseForBusStops(Route& route, const std::vector<std::string_view>& stops_for_route);

    json::Dict ApplyCommandsStatForStop(const json::Dict& request_data) const;
    json::Dict ApplyCommandsStatForBus(const json::Dict& request_data) const;
    json::Dict ApplyCommandsStatForRoute(const json::Dict& request_data) const;
    json::Dict ApplyCommandsStatForRouteMap(const json::Dict& request_data) const;
    json::Dict DictErrorMessage(int request_id) const;

    void ApplyCommandsRenderSettings();
    svg::Point ApplyCommandsRenderSettingsPoint(const json::Array& input) const;
    svg::Rgb ApplyCommandsRenderSettingsRgb(const json::Array& input) const;
    svg::Rgba ApplyCommandsRenderSettingsRgba(const json::Array& input) const;

    const transport_router::RoutingSettings ApplyCommandsRoutingSettings();
    json::Array ProcessRouteData(std::vector<const transport_router::GraphEdge*> edges) const;
};

} // namespace json_reader
