#include "map_renderer.h"

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршртутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace renderer {

void MapRenderer::SetRenderSettings(RenderSettings& settings)
{
    if (settings.padding > std::min(settings.width, settings.height)/2) {
        settings.padding = std::min(settings.width, settings.height)/2;
    }
    assert(settings.padding >= 0);
    render_settings_ = std::move(settings);
}

void MapRenderer::PrintSettings() const
{
    std::cout << render_settings_ << std::endl;
}

const RenderSettings &MapRenderer::GetSettings() const
{
    return render_settings_;
}

svg::Document MapRenderer::RenderMap() const
{
    svg::Document result;

    const double WIDTH = render_settings_.width;
    const double HEIGHT = render_settings_.height;
    const double PADDING = render_settings_.padding;

    auto vector_projector = CoordinatesVectorProjector();
    const SphereProjector sphere_projector{vector_projector.begin(), vector_projector.end(), WIDTH, HEIGHT, PADDING};

    ProjectCatalogueValidRoutes(result, sphere_projector);
    ProjectCatalogueValidRoutesNames(result, sphere_projector);
    ProjectCatalogueValidStops(result, sphere_projector);
    ProjectCatalogueValidStopsNames(result, sphere_projector);

    return result;
}

const std::vector<geo::Coordinates> MapRenderer::CoordinatesVectorProjector() const
{
    std::vector<geo::Coordinates> result;
    for (auto route : catalogue_.GetRouteList()) {
        for (auto stop : catalogue_.GetRoute(route)->description) {
            result.emplace_back(stop->position);
        }
    }
    return result;
}

void MapRenderer::ProjectCatalogueValidRoutes(svg::Document& result, const SphereProjector& projector) const
{
    size_t num_fill_color = 0;      // number to choose color in loop from color_palette (if number_colors < number_routes)
    const size_t color_palette_size = render_settings_.color_palette.size();
    for (auto route : catalogue_.GetRouteList()) {
        if (num_fill_color >= color_palette_size) {
            num_fill_color = 0;
        }
        auto route_rendered = ProjectCatalogueRoute(catalogue_.GetRoute(route), projector, num_fill_color);
        result.AddPtr(std::make_unique<svg::Polyline>(std::move(route_rendered)));
        ++num_fill_color;
    }
}

svg::Polyline MapRenderer::ProjectCatalogueRoute(const Route* route, const SphereProjector& projector, size_t num_fill_color) const
{
    svg::Polyline result;
    result.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
    result.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    result.SetStrokeWidth(render_settings_.line_width);
    result.SetFillColor("none");
    result.SetStrokeColor(render_settings_.color_palette[num_fill_color]);

    for (auto stop : route->description) {
        result.AddPoint(projector(stop->position));
    }
    return result;
}

void MapRenderer::ProjectCatalogueValidRoutesNames(svg::Document& result, const SphereProjector& projector) const
{
    size_t num_fill_color = 0;      // number to choose color in loop from color_palette (if number_colors < number_routes)
    const size_t color_palette_size = render_settings_.color_palette.size();
    for (auto route : catalogue_.GetRouteList()) {
        if (num_fill_color >= color_palette_size) {
            num_fill_color = 0;
        }
        auto route_ptr = catalogue_.GetRoute(route);
        auto route_name = route_ptr->id;

        // start Stop for every Route is always rendered
        auto start_stop = catalogue_.GetStop(route_ptr->route_start);
        auto route_underlayer_rendered = ProjectCatalogueRouteName(start_stop, route_name, projector, num_fill_color, true);    //first Text - underlayer
        auto route_name_rendered = ProjectCatalogueRouteName(start_stop, route_name, projector, num_fill_color);                //second Text - Route name
        result.AddPtr(std::make_unique<svg::Text>(std::move(route_underlayer_rendered)));
        result.AddPtr(std::make_unique<svg::Text>(std::move(route_name_rendered)));

        // end Stop for Route is rendered only if NOT roundtrip and (start_Stop_name != end_Stop_name)
        if (!route_ptr->is_roundtrip && (route_ptr->route_start != route_ptr->route_end)) {
            auto end_stop = catalogue_.GetStop(route_ptr->route_end);
            route_underlayer_rendered = ProjectCatalogueRouteName(end_stop, route_name, projector, num_fill_color, true);
            route_name_rendered = ProjectCatalogueRouteName(end_stop, route_name, projector, num_fill_color);
            result.AddPtr(std::make_unique<svg::Text>(std::move(route_underlayer_rendered)));
            result.AddPtr(std::make_unique<svg::Text>(std::move(route_name_rendered)));
        }

        ++num_fill_color;
    }
}

svg::Text MapRenderer::ProjectCatalogueRouteName(const Stop* stop, std::string_view route_name,
                                                 const SphereProjector& projector, size_t num_fill_color, bool is_underlayer) const
{
    svg::Text result;
    result.SetPosition(projector(stop->position));
    result.SetOffset(render_settings_.bus_label_offset);
    result.SetFontSize(render_settings_.bus_label_font_size);
    result.SetFontFamily("Verdana");
    result.SetFontWeight("bold");
    result.SetData(std::string(route_name));
    if (is_underlayer) {
        result.SetFillColor(render_settings_.underlayer_color);
        result.SetStrokeColor(render_settings_.underlayer_color);
        result.SetStrokeWidth(render_settings_.underlayer_width);
        result.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        result.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    }
    if (!is_underlayer) {
        result.SetFillColor(render_settings_.color_palette[num_fill_color]);
    }
    return result;
}

void MapRenderer::ProjectCatalogueValidStops(svg::Document& result, const SphereProjector& projector) const
{
    for (auto [stop_name, stop_value] : ProjectCatalogueGetSortedStops()) {
        auto stop_rendered = ProjectCatalogueStop(stop_value, projector);
        result.AddPtr(std::make_unique<svg::Circle>(stop_rendered));
    }
}

svg::Circle MapRenderer::ProjectCatalogueStop(const Stop* stop, const SphereProjector& projector) const
{
    svg::Circle result;
    result.SetCenter(projector(stop->position));
    result.SetRadius(render_settings_.stop_radius);
    result.SetFillColor("white");
    return result;
}

std::map<std::string_view, const Stop*> MapRenderer::ProjectCatalogueGetSortedStops() const
{
    std::map<std::string_view, const Stop*> stops_sorted_for_render;
    for (auto route : catalogue_.GetRouteList()) {
        for (auto stop : catalogue_.GetRoute(route)->description) {
            stops_sorted_for_render.emplace(stop->name, stop);
        }
    }
    return stops_sorted_for_render;
}

void MapRenderer::ProjectCatalogueValidStopsNames(svg::Document& result, const SphereProjector& projector) const
{
    for (auto [stop_name, stop_value] : ProjectCatalogueGetSortedStops()) {
        auto stop_name_underlayer_rendered = ProjectCatalogueStopName(stop_value, projector, true);
        auto stop_name_rendered = ProjectCatalogueStopName(stop_value, projector);
        result.AddPtr(std::make_unique<svg::Text>(stop_name_underlayer_rendered));
        result.AddPtr(std::make_unique<svg::Text>(stop_name_rendered));
    }
}

svg::Text MapRenderer::ProjectCatalogueStopName(const Stop *stop, const SphereProjector &projector, bool is_underlayer) const
{
    svg::Text result;
    result.SetPosition(projector(stop->position));
    result.SetOffset(render_settings_.stop_label_offset);
    result.SetFontSize(render_settings_.stop_label_font_size);
    result.SetFontFamily("Verdana");
    result.SetData(stop->name);
    if (is_underlayer) {
        result.SetFillColor(render_settings_.underlayer_color);
        result.SetStrokeColor(render_settings_.underlayer_color);
        result.SetStrokeWidth(render_settings_.underlayer_width);
        result.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        result.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    }
    if (!is_underlayer) {
        result.SetFillColor("black");
    }
    return result;
}

} // namespace renderer
