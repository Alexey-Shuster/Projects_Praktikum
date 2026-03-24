#pragma once

#include <boost/json/string_view.hpp>
#include <filesystem>
#include <optional>

#include "../game_app/token.h"
#include "game_utils/geometry.h"
#include "../http_server/http_response.h"

namespace utils {

    // Проверка, что путь находится внутри базового каталога
    bool IsSubPath(const std::filesystem::path& path, const std::filesystem::path& base);

    // Helper function to convert std::string_view to JSON::string_view
    boost::json::string_view ToJsonStringView(std::string_view sv);

    double CalculateDistance2D(const app_geom::Position2D& a, const app_geom::Position2D& b);

    model_geom::Point2D Position2DToPoint2D (app_geom::Position2D pos);

    app_geom::Position2D Point2DToPosition2D (model_geom::Point2D point);

    std::string Direction2DToString(const app_geom::Direction2D &dir);

    std::optional<app_geom::Direction2D> StringToDirection2D(std::string_view move_value);

    app_geom::Speed2D Speed2DFromDirection2D(app_geom::Direction2D dir, app_geom::Speed2D default_speed);

    app_geom::Direction2D Direction2DFromIndex(size_t index);

    double GetRandomNumber(double num1, double num2, std::mt19937& rng);

    bool IsDiffInsideBounds(double num1, double num2, double bounds);

    bool IsWithinTolerance(double value, double reference, double tolerance = common_values::DOUBLE_ABS_TOLERANCE);

    namespace http {

    // Декодирование URL
    const std::string DecodeUrl(const std::string& url);

    // Получение MIME-типа по расширению файла
    const std::string_view GetMimeType(const std::filesystem::path& file_path);

    // Convenience function for Authorization header parsing
    std::optional<app::Token> ExtractToken(const http_handler::StringRequest& req);

    // Helper function to convert vector of HTTP methods to comma-separated string
    std::string MethodsToString(const std::vector<boost::beast::http::verb>& methods);

} // namespace http

} // namespace utils
