#pragma once

#include <string>   // for string literals

namespace common_values {

    constexpr static inline unsigned VERSION = 11;
    constexpr static inline double DEFAULT_DOG_SPEED = 1.0;
    constexpr static inline int DEFAULT_BAG_CAPACITY = 3;
    constexpr static inline int NO_AUTO_TICK = 0;
    constexpr static inline double DOUBLE_ABS_TOLERANCE = 1e-10;
    constexpr static inline double ROAD_WIDTH_HALF = 0.4;
    constexpr static inline double COLLISION_WIDTH_OBJECT = 0.0;
    constexpr static inline double COLLISION_WIDTH_PLAYER = 0.6;
    constexpr static inline double COLLISION_WIDTH_OFFICE = 0.5;
    constexpr static inline size_t DEFAULT_LOOT_CONFIG_PERIOD = 100;
    constexpr static inline double DEFAULT_LOOT_CONFIG_PROBABILITY = 0.5;
    constexpr static inline size_t DOG_BOT_START_ID = 10001;     // big enough value to diff from real Player
    constexpr static inline size_t MAX_PLAYERS_ON_MAP = 12;
    constexpr static inline double BOT_DIR_CHANGE_PROBABILITY = DEFAULT_DOG_SPEED / 10.0;
    constexpr static inline size_t SEC_TO_MSEC = 1000;
    constexpr static inline size_t MIN_TO_SEC = 60;
    constexpr static inline size_t DEFAULT_DOG_RETIREMENT_TIME_SEC = 1*MIN_TO_SEC;    // default 1 minute
    constexpr static inline size_t DB_MAX_ITEMS_TO_GET = 100;

} // namespace common_values

// Константы для JSON полей
namespace json_fields {

    // Общие поля
    constexpr static inline const char* ID = "id";
    constexpr static inline const char* NAME = "name";
    constexpr static inline const char* CODE = "code";
    constexpr static inline const char* MESSAGE = "message";
    constexpr static inline const char* INFO_LOG = "Info Log";
    constexpr static inline const char* USER_NAME = "userName";
    constexpr static inline const char* MAP_ID = "mapId";
    constexpr static inline const char* AUTH_TOKEN = "authToken";
    constexpr static inline const char* PLAYER_ID = "playerId";
    constexpr static inline const char* NO_CACHE = "no-cache";
    constexpr static inline const char* DEFAULT_DOG_SPEED = "defaultDogSpeed";
    constexpr static inline const char* DOG_SPEED = "dogSpeed";
    constexpr static inline const char* MOVE = "move";
    constexpr static inline const char* TIME_DELTA = "timeDelta";
    constexpr static inline const char* LOOT_GENERATOR_CONFIG = "lootGeneratorConfig";
    constexpr static inline const char* PERIOD = "period";
    constexpr static inline const char* PROBABILITY = "probability";
    constexpr static inline const char* DEFAULT_BAG_CAPACITY = "defaultBagCapacity";
    constexpr static inline const char* BAG_CAPACITY = "bagCapacity";
    constexpr static inline const char* DOG_RETIREMENT_TIME = "dogRetirementTime";
    constexpr static inline const char* GAME_RECORDS_OFFSET = "start";
    constexpr static inline const char* GAME_RECORDS_MAX_ITEMS = "maxItems";

    // Поля дорог
    constexpr static inline const char* X0 = "x0";
    constexpr static inline const char* Y0 = "y0";
    constexpr static inline const char* X1 = "x1";
    constexpr static inline const char* Y1 = "y1";

    // Поля зданий
    constexpr static inline const char* X = "x";
    constexpr static inline const char* Y = "y";
    constexpr static inline const char* W = "w";
    constexpr static inline const char* H = "h";

    // Поля офисов
    constexpr static inline const char* OFFSET_X = "offsetX";
    constexpr static inline const char* OFFSET_Y = "offsetY";

    // Поля игроков
    constexpr static inline const char* POS = "pos";
    constexpr static inline const char* SPEED = "speed";
    constexpr static inline const char* DIR = "dir";
    constexpr static inline const char* BAG = "bag";
    constexpr static inline const char* SCORE = "score";
    constexpr static inline const char* PLAYERS = "players";

    // Ключи массивов
    constexpr static inline const char* ROADS = "roads";
    constexpr static inline const char* BUILDINGS = "buildings";
    constexpr static inline const char* OFFICES = "offices";
    constexpr static inline const char* MAPS = "maps";
    constexpr static inline const char* LOOT_TYPES = "lootTypes";

    // Поля для лута (lootTypes)
    constexpr static inline const char* FILE = "file";        //file - reserved, but json_fields::FILE - Ok
    constexpr static inline const char* TYPE = "type";
    constexpr static inline const char* ROTATION = "rotation";
    constexpr static inline const char* COLOR = "color";
    constexpr static inline const char* SCALE = "scale";
    constexpr static inline const char* LOST_OBJECTS = "lostObjects";
    constexpr static inline const char* VALUE = "value";

} // namespace json_fields

namespace http_handler {

using namespace std::literals;

// Константы для Content-Type
struct ContentType {
    ContentType() = delete;
    constexpr inline static std::string_view EMPTY = ""sv;
    constexpr inline static std::string_view TEXT_HTML = "text/html"sv;
    constexpr inline static std::string_view TEXT_CSS = "text/css"sv;
    constexpr inline static std::string_view TEXT_PLAIN = "text/plain"sv;
    constexpr inline static std::string_view TEXT_JAVASCRIPT = "text/javascript"sv;
    constexpr inline static std::string_view APPLICATION_JSON = "application/json"sv;
    constexpr inline static std::string_view APPLICATION_XML = "application/xml"sv;
    constexpr inline static std::string_view IMAGE_PNG = "image/png"sv;
    constexpr inline static std::string_view IMAGE_JPEG = "image/jpeg"sv;
    constexpr inline static std::string_view IMAGE_GIF = "image/gif"sv;
    constexpr inline static std::string_view IMAGE_BMP = "image/bmp"sv;
    constexpr inline static std::string_view IMAGE_ICO = "image/vnd.microsoft.icon"sv;
    constexpr inline static std::string_view IMAGE_TIFF = "image/tiff"sv;
    constexpr inline static std::string_view IMAGE_SVG = "image/svg+xml"sv;
    constexpr inline static std::string_view AUDIO_MPEG = "audio/mpeg"sv;
    constexpr inline static std::string_view APPLICATION_OCTET_STREAM = "application/octet-stream"sv;
};

// Коды ошибок API
namespace error_codes {
    constexpr inline static std::string_view MAP_NOT_FOUND = "mapNotFound"sv;
    constexpr inline static std::string_view BAD_REQUEST = "badRequest"sv;
    constexpr inline static std::string_view INVALID_METHOD = "invalidMethod"sv;
    constexpr inline static std::string_view INVALID_URI = "invalidURI"sv;
    constexpr inline static std::string_view INTERNAL_ERROR = "internalError"sv;
    constexpr inline static std::string_view EXCEPTION_CAUGHT = "exceptionCaught"sv;
    constexpr inline static std::string_view INVALID_ARGUMENT = "invalidArgument"sv;
    constexpr inline static std::string_view INVALID_TOKEN = "invalidToken"sv;
    constexpr inline static std::string_view UNKNOWN_TOKEN = "unknownToken"sv;
    constexpr inline static std::string_view ACTION_PARSE_ERROR = "actionParseError"sv;
}

// Сообщения об ошибках API
namespace error_messages {
    constexpr inline static std::string_view MAP_NOT_FOUND = "Map not found"sv;
    constexpr inline static std::string_view BAD_REQUEST = "Bad request"sv;
    constexpr inline static std::string_view INVALID_METHOD = "Invalid method"sv;
    constexpr inline static std::string_view INVALID_URI = "Invalid URI"sv;
    constexpr inline static std::string_view INVALID_PATH = "Path is outside root directory"sv;
    constexpr inline static std::string_view INVALID_SOURCE = "Source not found"sv;
    constexpr inline static std::string_view INVALID_ENDPOINT = "Invalid endpoint"sv;
    constexpr inline static std::string_view SOURCE_FAIL = "Failed processing requested resource"sv;
    constexpr inline static std::string_view METHOD_EXPECTED = "Only method expected: "sv;
    constexpr inline static std::string_view UNKNOWN_ERROR = "Unknown error"sv;
    constexpr inline static std::string_view JOIN_GAME_PARSE = "Join game request parse error"sv;
    constexpr inline static std::string_view INVALID_NAME = "Invalid name"sv;
    constexpr inline static std::string_view INVALID_TOKEN = "Authorization header is missing"sv;
    constexpr inline static std::string_view UNKNOWN_TOKEN = "Player token has not been found"sv;
    constexpr inline static std::string_view INVALID_CONTENT_TYPE = "Invalid Content Type"sv;
    constexpr inline static std::string_view ACTION_PARSE_ERROR = "Failed to parse action value"sv;
    constexpr inline static std::string_view INVALID_ACTION_VALUE = "Invalid action-move value"sv;
    constexpr inline static std::string_view TICK_PARSE_ERROR = "Failed to parse tick value"sv;
    constexpr inline static std::string_view INVALID_TICK_VALUE = "Invalid tick value"sv;
}

// Пути API
namespace api_paths {
    constexpr inline static std::string_view API_PREFIX = "/api/"sv;
    constexpr inline static std::string_view MAPS_BASE = "/api/v1/maps"sv;
    constexpr inline static std::string_view SLASH = "/"sv;
    constexpr inline static std::string_view COLON = ":"sv;
    constexpr inline static std::string_view ID = ":id"sv;
    constexpr inline static std::string_view INDEX_HTML = "index.html"sv;
    constexpr inline static std::string_view GAME_BASE = "/api/v1/game"sv;
    constexpr inline static std::string_view GAME_JOIN = "/api/v1/game/join"sv;
    constexpr inline static std::string_view GAME_PLAYERS = "/api/v1/game/players"sv;
    constexpr inline static std::string_view GAME_STATE = "/api/v1/game/state"sv;
    constexpr inline static std::string_view PLAYER_ACTION = "/api/v1/game/player/action"sv;
    constexpr inline static std::string_view BEARER = "Bearer "sv;
    constexpr inline static std::string_view GAME_TICK = "/api/v1/game/tick"sv;
    constexpr inline static std::string_view GAME_RECORDS = "/api/v1/game/records"sv;
}

// HTTP методы
namespace http_methods {
    constexpr inline static std::string_view ALLOW_METHODS = "GET, HEAD, POST(join game only)"sv;
    constexpr inline static std::string_view GET = "GET"sv;
    constexpr inline static std::string_view HEAD = "HEAD"sv;
    constexpr inline static std::string_view GET_HEAD = "GET, HEAD"sv;
    constexpr inline static std::string_view POST = "POST"sv;
}

} // namespace http_handler

namespace app {

namespace move_values {
    constexpr inline static const char* LEFT = "L";
    constexpr inline static const char* RIGHT = "R";
    constexpr inline static const char* UP = "U";
    constexpr inline static const char* DOWN = "D";
    constexpr inline static const char* STOP = "";  // empty string = S
}

} // namespace app
