#include <unordered_map>

#include "constants.h"
#include "utils.h"

namespace utils {

    bool IsSubPath(const std::filesystem::path &path, const std::filesystem::path &base) {
        auto norm_path = std::filesystem::weakly_canonical(path);
        auto norm_base = std::filesystem::weakly_canonical(base);

        auto path_it = norm_path.begin();
        auto base_it = norm_base.begin();

        while (base_it != norm_base.end()) {
            if (path_it == norm_path.end() || *path_it != *base_it) {
                return false;
            }
            ++path_it;
            ++base_it;
        }
        return true;
    }

    boost::json::string_view ToJsonStringView(std::string_view sv) {
        return boost::json::string_view(sv.data(), sv.size());
    }

    double CalculateDistance2D(const app_geom::Position2D &a, const app_geom::Position2D &b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    app_geom::Position2D Point2DToPosition2D(model_geom::Point2D point) {
        return {static_cast<app_geom::Coord>(point.x), static_cast<app_geom::Coord>(point.y)};
    }

    std::string Direction2DToString(const app_geom::Direction2D &dir) {
        std::string dir_str;
        switch (dir) {
            case app_geom::Direction2D::LEFT:
                dir_str = app::move_values::LEFT;
                break;
            case app_geom::Direction2D::RIGHT:
                dir_str = app::move_values::RIGHT;
                break;
            case app_geom::Direction2D::UP:
                dir_str = app::move_values::UP;
                break;
            case app_geom::Direction2D::DOWN:
                dir_str = app::move_values::DOWN;
                break;
            case app_geom::Direction2D::STOP:
                dir_str = app::move_values::STOP;
                break;
            default:
                dir_str = app::move_values::STOP;
                break;
        }
        return dir_str;
    }

    std::optional<app_geom::Direction2D> StringToDirection2D(std::string_view move_value) {
        if (move_value == app::move_values::LEFT) {
            return app_geom::Direction2D::LEFT;
        } else if (move_value == app::move_values::RIGHT) {
            return app_geom::Direction2D::RIGHT;
        } else if (move_value == app::move_values::UP) {
            return app_geom::Direction2D::UP;
        } else if (move_value == app::move_values::DOWN) {
            return app_geom::Direction2D::DOWN;
        } else if (move_value == app::move_values::STOP) {
            return app_geom::Direction2D::STOP;
        }

        return std::nullopt;
    }

    model_geom::Point2D Position2DToPoint2D(app_geom::Position2D pos) {
        return {static_cast<model_geom::Coord>(pos.x), static_cast<model_geom::Coord>(pos.y)};
    }

    app_geom::Speed2D Speed2DFromDirection2D(app_geom::Direction2D dir, app_geom::Speed2D default_speed) {
        app_geom::Speed2D result;
        switch(dir) {
            case app_geom::Direction2D::UP:    result = {0.0, -default_speed.y}; break;
            case app_geom::Direction2D::DOWN:  result = {0.0, default_speed.y}; break;
            case app_geom::Direction2D::LEFT:  result = {-default_speed.x, 0.0}; break;
            case app_geom::Direction2D::RIGHT: result = {default_speed.x, 0.0}; break;
            case app_geom::Direction2D::STOP:  result = app_geom::Speed2D::Zero(); break;
            default: throw std::runtime_error("utils::Speed2DFromDirection2D: Unknown Direction."); break;
        }
        return result;
    }

    app_geom::Direction2D Direction2DFromIndex(size_t index) {
        app_geom::Direction2D new_dir;
        switch (index) {
            case 0: new_dir = app_geom::Direction2D::LEFT; break;
            case 1: new_dir = app_geom::Direction2D::RIGHT; break;
            case 2: new_dir = app_geom::Direction2D::UP; break;
            case 3: new_dir = app_geom::Direction2D::DOWN; break;
            default: new_dir = app_geom::Direction2D::UP; break;
        }
        return new_dir;
    }

    double GetRandomNumber(double num1, double num2, std::mt19937 &rng) {
        if (std::abs(num1 - num2) < std::numeric_limits<double>::epsilon()) {
            return num1;
        }
        if (num1 > num2) std::swap(num1, num2);
        std::uniform_real_distribution<double> num_dist(num1, num2);
        return num_dist(rng);
    }

    bool IsDiffInsideBounds(double num1, double num2, double bounds) {
        return std::abs(num1 - num2) <= bounds + common_values::DOUBLE_ABS_TOLERANCE;
    }

    bool IsWithinTolerance(double value, double reference, double tolerance) {
        return std::abs(value - reference) <= tolerance;
    }

    namespace http {

const std::string DecodeUrl(const std::string &url) {
    std::string result;
    for (size_t i = 0; i < url.size(); ++i) {
        if (url[i] == '%' && i + 2 < url.size()) {
            int hex_value;
            std::stringstream ss;
            ss << std::hex << url.substr(i + 1, 2);
            ss >> hex_value;
            result += static_cast<char>(hex_value);
            i += 2;
        } else if (url[i] == '+') {
            result += ' ';
        } else {
            result += url[i];
        }
    }
    return result;
}

const std::string_view GetMimeType(const std::filesystem::path& file_path)  {
    // Статическая таблица соответствий расширение -> ContentType
    static const std::unordered_map<std::string, std::string_view> mime_types = {
        // HTML
        {".html", http_handler::ContentType::TEXT_HTML},
        {".htm", http_handler::ContentType::TEXT_HTML},

        // CSS
        {".css", http_handler::ContentType::TEXT_CSS},

        // Text
        {".txt", http_handler::ContentType::TEXT_PLAIN},

        // JavaScript
        {".js", http_handler::ContentType::TEXT_JAVASCRIPT},

        // JSON
        {".json", http_handler::ContentType::APPLICATION_JSON},

        // XML
        {".xml", http_handler::ContentType::APPLICATION_XML},

        // Images
        {".png", http_handler::ContentType::IMAGE_PNG},
        {".jpg", http_handler::ContentType::IMAGE_JPEG},
        {".jpe", http_handler::ContentType::IMAGE_JPEG},
        {".jpeg", http_handler::ContentType::IMAGE_JPEG},
        {".gif", http_handler::ContentType::IMAGE_GIF},
        {".bmp", http_handler::ContentType::IMAGE_BMP},
        {".ico", http_handler::ContentType::IMAGE_ICO},
        {".tiff", http_handler::ContentType::IMAGE_TIFF},
        {".tif", http_handler::ContentType::IMAGE_TIFF},
        {".svg", http_handler::ContentType::IMAGE_SVG},
        {".svgz", http_handler::ContentType::IMAGE_SVG},

        // Audio
        {".mp3", http_handler::ContentType::AUDIO_MPEG}
    };

    // Получаем расширение файла и приводим к нижнему регистру
    std::string extension = file_path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // Ищем расширение в таблице
    auto it = mime_types.find(extension);
    if (it != mime_types.end()) {
        return it->second;
    }

    // Расширение не найдено - возвращаем тип по умолчанию
    return http_handler::ContentType::APPLICATION_OCTET_STREAM;
}

std::optional<app::Token> ExtractToken(const http_handler::StringRequest &req) {
    auto auth_header = req.find(boost::beast::http::field::authorization);
    if (auth_header == req.end()) {
        return std::nullopt;
    }

    auto auth_value = auth_header->value();

    if (auth_value.size() <= http_handler::api_paths::BEARER.size() ||
        !auth_value.starts_with(http_handler::api_paths::BEARER)) {
        return std::nullopt;
    }

    return app::Token{std::string(auth_value.substr(http_handler::api_paths::BEARER.size()))};
}

std::string MethodsToString(const std::vector<boost::beast::http::verb> &methods) {
    if (methods.empty()) {
        return "";
    }

    std::string result;
    for (size_t i = 0; i < methods.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += to_string(methods[i]);
    }

    std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return std::toupper(c);
    });

    return result;
}

} // namespace http

} // namespace utils
