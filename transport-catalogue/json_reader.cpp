#include "json_reader.h"

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace json_reader {

const json::Document JSONReader::GetData() const {
    return data_input_;
}

void JSONReader::ApplyCommandsBase() {
    const auto& base_requests = data_input_.GetRoot().AsMap().at("base_requests");
    if (base_requests.AsArray().empty()) {
        return;
    }
    std::unordered_map<StopName, StopDistances> stops_distances;            // temp object for Stops distances processing

    for (const auto& base_request : base_requests.AsArray()) {              // process only Stops in first loop
        auto& request_data = base_request.AsMap();
        if (request_data.at("type").IsString()) {
            if (request_data.at("type").AsString() == "Stop") {             // processing Base request for Stop
                ApplyCommandsBaseForStop(request_data, stops_distances);
            }
        }
    }

    ApplyCommandsBaseForStopsDistances(stops_distances);                    // process Stops distances

    for (const auto& base_request : base_requests.AsArray()) {              // process Buses in second loop
        auto& request_data = base_request.AsMap();
        if (request_data.at("type").IsString()) {
            if (request_data.at("type").AsString() == "Bus") {              // processing Base request for Bus
                ApplyCommandsBaseForBus(request_data);
            }
        }
    }

    ApplyCommandsRenderSettings();
    ApplyCommandsRoutingSettings();
    router_ = std::make_unique<transport_router::TransportRouter>(catalogue_, ApplyCommandsRoutingSettings());
}

void JSONReader::ApplyCommandsRenderSettings()
{
    renderer::RenderSettings render_settings;

    const auto& input_settings = data_input_.GetRoot().AsMap().at("render_settings");
    assert(!input_settings.AsMap().empty());
    for (const auto& [name, value]: input_settings.AsMap()) {
        if (name == "width" && value.IsDouble()) {
            render_settings.width = value.AsDouble();
        } else if (name == "height" && value.IsDouble()) {
            render_settings.height = value.AsDouble();
        } else if (name == "padding" && value.IsDouble()) {
            render_settings.padding = value.AsDouble();
        } else if (name == "line_width" && value.IsDouble()) {
            render_settings.line_width = value.AsDouble();
        } else if (name == "stop_radius" && value.IsDouble()) {
            render_settings.stop_radius = value.AsDouble();
        }

        if (name == "bus_label_font_size" && value.IsInt()) {
            render_settings.bus_label_font_size = value.AsInt();
        } else if (name == "bus_label_offset" && value.IsArray() && (value.AsArray().size() == 2)) {
            render_settings.bus_label_offset = ApplyCommandsRenderSettingsPoint(value.AsArray());
        } else if (name == "stop_label_font_size" && value.IsInt()) {
            render_settings.stop_label_font_size = value.AsInt();
        } else if (name == "stop_label_offset" && value.IsArray() && (value.AsArray().size() == 2)) {
            render_settings.stop_label_offset = ApplyCommandsRenderSettingsPoint(value.AsArray());
        }

        if (name == "underlayer_color" && (value.IsArray() || value.IsString())) {
            if (value.IsString()) {
                render_settings.underlayer_color = value.AsString();
            } else if (value.IsArray() && (value.AsArray().size() == size_t{3})) {
                render_settings.underlayer_color = ApplyCommandsRenderSettingsRgb(value.AsArray());
            } else if (value.IsArray() && (value.AsArray().size() == size_t{4})) {
                render_settings.underlayer_color = ApplyCommandsRenderSettingsRgba(value.AsArray());
            }
        } else if (name == "underlayer_width" && value.IsDouble()) {
            render_settings.underlayer_width = value.AsDouble();
        } else if (name == "color_palette" && value.IsArray() && !value.AsArray().empty()) {
            for (const auto& item : value.AsArray()) {
                // std::cout << "item in color_palette->" << item << std::endl;
                if (item.IsString()) {
                    render_settings.color_palette.emplace_back(item.AsString());
                } else if (item.IsArray() && (item.AsArray().size() == size_t{3})) {
                    render_settings.color_palette.emplace_back(ApplyCommandsRenderSettingsRgb(item.AsArray()));
                } else if (item.IsArray() && (item.AsArray().size() == size_t{4})) {
                    render_settings.color_palette.emplace_back(ApplyCommandsRenderSettingsRgba(item.AsArray()));
                }
            }
        }
    }
    renderer_.SetRenderSettings(render_settings);
}

void JSONReader::ApplyCommandsBaseForStop(const json::Dict& request_data, std::unordered_map<StopName, StopDistances>& stops_distances) {
    if (request_data.at("name").IsString() && !request_data.at("name").AsString().empty()) {
        Stop stop;
        stop.name = request_data.at("name").AsString();
        stop.position.lat = request_data.at("latitude").AsDouble();
        stop.position.lng = request_data.at("longitude").AsDouble();
        catalogue_.AddStop(stop);
        if (request_data.at("road_distances").IsMap() && !request_data.at("road_distances").AsMap().empty()) {    // if road_distances exist and not empty
            StopDistances temp;
            for (const auto& road_item : request_data.at("road_distances").AsMap()) {
                // std::cout << "road_item:[" << road_item.first << "/" << road_item.second.AsInt() << "]" << std::endl;
                temp.emplace(road_item.first, road_item.second.AsInt());
            }
            stops_distances.emplace(request_data.at("name").AsString(), temp);         // saves StopDistances for current StopName for further processing
        }
        // std::cout << "Stop in ApplyCommandsBase:" << stop << std::endl;
    }
}

void JSONReader::ApplyCommandsBaseForBus(const json::Dict &request_data)
{
    if (request_data.at("name").IsString() && !request_data.at("name").AsString().empty()) {
        Route route;
        route.id = request_data.at("name").AsString();
        route.is_roundtrip = request_data.at("is_roundtrip").AsBool();
        if (request_data.at("stops").IsArray() && !request_data.at("stops").AsArray().empty()) {
            std::vector<std::string_view> stops_on_current_route;           // temp storage for stops on current route
            for (const auto& stop : request_data.at("stops").AsArray()) {
                stops_on_current_route.emplace_back(stop.AsString());
            }
            route.route_one_way_size = stops_on_current_route.size();
            route.route_start = catalogue_.GetStop(request_data.at("stops").AsArray().front().AsString())->name;
            route.route_end = route.route_start;
            if (!route.is_roundtrip) {          // complete Route if NOT roundtrip
                route.route_end = catalogue_.GetStop(request_data.at("stops").AsArray().back().AsString())->name;
                ApplyCommandsBaseForBusNoRoundtrip(stops_on_current_route);
            }
            route.stops_num = stops_on_current_route.size();
            // std::cout << "Route in ApplyCommandsBase:" << route << std::endl;

            ApplyCommandsBaseForBusStops(route, stops_on_current_route);
        }
    }
}

void JSONReader::ApplyCommandsBaseForBusNoRoundtrip(std::vector<std::string_view>& stops_on_current_route) {
    bool first_iteration = true;
    std::vector<std::string_view> stops_temp_reverse;
    for (auto it = stops_on_current_route.rbegin(); it != stops_on_current_route.rend(); ++it) {
        if (first_iteration) {
            first_iteration = false;
            continue;
        }
        stops_temp_reverse.emplace_back(*it);
    }
    stops_on_current_route.insert(stops_on_current_route.end(), stops_temp_reverse.begin(), stops_temp_reverse.end());
}

void JSONReader::ApplyCommandsBaseForStopsDistances(const std::unordered_map<StopName, StopDistances>& stops_distances) {
    for (const auto& [stop_name, stop_dist] : stops_distances) {
        for (const auto& [destination_name, destination_distance] : stop_dist) {
            catalogue_.SetRealDistanceForStop(stop_name, destination_name, destination_distance);
        }
    }
}

void JSONReader::ApplyCommandsBaseForBusStops(Route& route, const std::vector<std::string_view>& stops_for_route) {
    std::unordered_set<std::string_view> unique_stops{};
    for (const auto& stop : stops_for_route) {
        unique_stops.insert(stop);
        route.description.push_back(catalogue_.GetStopStat(stop));
    }
    route.unique_stops_num = unique_stops.size();
    catalogue_.AddRoute(route);
}

json::Document JSONReader::PrintStat() const {
    const auto& stat_requests = data_input_.GetRoot().AsMap().at("stat_requests");
    json::Array output;

    if (stat_requests.AsArray().empty()) {
        return json::Document(json::Builder{}.Value(output).Build());
    }

    for (const auto&  stat_request : stat_requests.AsArray()) {
        auto& request_data = stat_request.AsMap();
        if (request_data.at("type").AsString() == "Stop") {             // processing Stat request for Stop
            if (ApplyCommandsStatForStop(request_data).empty()) {       // if Stop name NOT valid - skip request
                continue;
            }
            output.emplace_back(ApplyCommandsStatForStop(request_data));
        } else
        if (request_data.at("type").AsString() == "Bus") {              // processing Stat request for Bus
            if (ApplyCommandsStatForBus(request_data).empty()) {        // if Bus name NOT valid - skip request
                continue;
            }
            output.emplace_back(ApplyCommandsStatForBus(request_data));
        } else
        if (request_data.at("type").AsString() == "Route") {            // processing Stat request for Route
            if (ApplyCommandsStatForRoute(request_data).empty()) {      // if Route name NOT valid - skip request
                continue;
            }
            output.emplace_back(ApplyCommandsStatForRoute(request_data));
        } else
        if (request_data.at("type").AsString() == "Map") {              // processing Stat request for Map
            output.emplace_back(ApplyCommandsStatForRouteMap(request_data));
        }
    }

    return json::Document(json::Builder{}.Value(output).Build());
}

json::Dict JSONReader::ApplyCommandsStatForStop(const json::Dict& request_data) const
{
    auto request_id = request_data.at("id").AsInt();
    if (!request_data.at("name").IsString()) {          // if Stop name NOT string - return empty Dict
        return {};
    }
    auto stop = catalogue_.GetStopStat(request_data.at("name").AsString());
    if (stop == nullptr) {
        return DictErrorMessage(request_id);
    }
    json::Array buses_for_output;       // if no buses for Stop - print empty Array
    for (const auto bus : stop->buses) {
        buses_for_output.emplace_back((std::string(bus)));
    }
    json::Dict result{{"request_id", json::Builder{}.Value(request_id).Build()}};
    result.emplace("buses", json::Builder{}.Value(buses_for_output).Build());

    return result;
}

json::Dict JSONReader::ApplyCommandsStatForBus(const json::Dict& request_data) const
{
    auto request_id = request_data.at("id").AsInt();
    if (!request_data.at("name").IsString()) {          // if Route name NOT string - return empty Dict
        return {};
    }
    auto route = catalogue_.GetRouteStat(request_data.at("name").AsString());
    if (route == nullptr) {
        return DictErrorMessage(request_id);
    }
    json::Dict result{{"request_id", json::Builder{}.Value(request_id).Build()}};
    result.emplace("curvature", json::Builder{}.Value(route->curvature).Build());
    result.emplace("route_length", json::Builder{}.Value(route->route_length).Build());
    result.emplace("stop_count", json::Builder{}.Value(static_cast<int>(route->stops_num)).Build());
    result.emplace("unique_stop_count", json::Builder{}.Value(static_cast<int>(route->unique_stops_num)).Build());

    return result;
}

json::Dict JSONReader::ApplyCommandsStatForRoute(const json::Dict &request_data) const
{
    auto request_id = request_data.at("id").AsInt();
    if (!request_data.at("from").IsString() || !request_data.at("to").IsString()) {          // if Stop (from OR to) name NOT string - return empty Dict
        return {};
    }
    // return {};   //Debug
    auto stop_from = request_data.at("from").AsString();
    auto stop_to = request_data.at("to").AsString();

    auto stop_list = catalogue_.GetStopList();
    if (stop_list.find(stop_from) == stop_list.end() || stop_list.find(stop_to) == stop_list.end()) {
        return DictErrorMessage(request_id);
    }

    auto route_data = router_->GetRouteData(stop_from, stop_to);
    if (!route_data) {
        return DictErrorMessage(request_id);
    }
    json::Dict result{{"request_id", json::Builder{}.Value(request_id).Build()}};
    result.emplace("total_time", json::Builder{}.Value(route_data->route_weight).Build());
    result.emplace("items", ProcessRouteData(route_data->edges));

    return result;
}

json::Dict JSONReader::ApplyCommandsStatForRouteMap(const json::Dict &request_data) const
{
    auto request_id = request_data.at("id").AsInt();
    json::Dict result{{"request_id", json::Builder{}.Value(request_id).Build()}};
    std::ostringstream map_as_string;
    renderer_.RenderMap().Render(map_as_string);
    result.emplace("map", json::Builder{}.Value(map_as_string.str()).Build());
    return result;
}

json::Dict JSONReader::DictErrorMessage(int request_id) const {
    static const std::string NOT_FOUND = "not found";
    return json::Dict{{"request_id", json::Builder{}.Value(request_id).Build()}, {"error_message", json::Builder{}.Value(NOT_FOUND).Build()}};
}

void JSONReader::PrintRouteRequestStat(bool only_general_data)
{
    // for Debug
    size_t request_count = 0;
    size_t request_count_route = 0;
    const auto& stat_requests = data_input_.GetRoot().AsMap().at("stat_requests");
    request_count = stat_requests.AsArray().size();
    for (const auto&  stat_request : stat_requests.AsArray()) {
        auto& request_data = stat_request.AsMap();
        if (request_data.at("type").AsString() == "Route") {
            ++request_count_route;
            auto stop_from = request_data.at("from").AsString();
            auto stop_to = request_data.at("to").AsString();
            route_request_count_[std::make_pair(catalogue_.GetStop(stop_from)->name, catalogue_.GetStop(stop_to)->name)]++;
        }
    }

    catalogue_.PrintCatalogueStat();

    size_t repeat_request_count_1 = 0;
    size_t repeat_request_count_5 = 0;
    for (const auto& [route, count] : route_request_count_) {
        (count > 1) ? ++repeat_request_count_1 : 0;
        (count > 5) ? ++repeat_request_count_5 : 0;
        if (!only_general_data) {
            std::cout << "JSONReader::PrintRouteRequestInfo: from[" << route.first << "] to [" << route.second << "] count: " << count << std::endl;
        }
    }
    std::cout << "JSONReader::PrintRouteRequestStat: Request count [" << request_count << "], where Route requests [" << request_count_route
              << "] -> Repeated requests [" << repeat_request_count_1 << "]" << ", where requests repeated > 5 [" << repeat_request_count_5 << "]" << std::endl;
    // for Debug
}

svg::Point JSONReader::ApplyCommandsRenderSettingsPoint(const json::Array& input) const
{
    assert(input.size() == size_t{2});
    return {input.front().AsDouble(), input.back().AsDouble()};
}

svg::Rgb JSONReader::ApplyCommandsRenderSettingsRgb(const json::Array &input) const
{
    assert(input.size() == size_t{3});
    return {static_cast<uint8_t>(input[0].AsInt()), static_cast<uint8_t>(input[1].AsInt()), static_cast<uint8_t>(input[2].AsInt())};
}

svg::Rgba JSONReader::ApplyCommandsRenderSettingsRgba(const json::Array &input) const
{
    assert(input.size() == size_t{4});
    return {static_cast<uint8_t>(input[0].AsInt()), static_cast<uint8_t>(input[1].AsInt()), static_cast<uint8_t>(input[2].AsInt()), input[3].AsDouble()};
}

const transport_router::RoutingSettings JSONReader::ApplyCommandsRoutingSettings()
{
    transport_router::RoutingSettings routing_settings{};

    const auto& input_settings = data_input_.GetRoot().AsMap().at("routing_settings");
    assert(!input_settings.AsMap().empty());
    for (const auto& [name, value]: input_settings.AsMap()) {
        if (name == "bus_wait_time" && value.IsInt()) {
            routing_settings.bus_wait_time = value.AsInt();
        } else if (name == "bus_velocity" && value.IsDouble()) {
            routing_settings.bus_velocity = value.AsDouble();
        }
    }
    return routing_settings;
}

json::Array JSONReader::ProcessRouteData(std::vector<const transport_router::GraphEdge*> edges) const
{
    json::Array result;
    for (const auto& item : edges) {
        const auto& [vertex_from, vertex_to, weight, bus, span_count] = *item;

        json::Dict item_json;
        if (vertex_from->type == transport_router::VertexType::WAIT) {
            item_json.emplace("type", json::Builder{}.Value("Wait").Build());
            item_json.emplace("stop_name", json::Builder{}.Value(std::string(vertex_from->name)).Build());
        } else {
            item_json.emplace("type", json::Builder{}.Value("Bus").Build());
            item_json.emplace("bus", json::Builder{}.Value(std::string(bus)).Build());
            item_json.emplace("span_count", json::Builder{}.Value(static_cast<int>(span_count)).Build());
        }
        item_json.emplace("time", json::Builder{}.Value(weight).Build());
        result.emplace_back(item_json);
    }
    return result;
}

} // namespace json_reader
