#include <fstream>
#include <sstream>
#include <string>
#include <boost/json.hpp>

#include "json_loader.h"
#include "constants.h"

namespace json_loader {

namespace {

namespace json = boost::json;

// Helper function to load JSON from file
json::value LoadJson(const std::filesystem::path &json_path) {
    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open JSON file: " + json_path.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_content = buffer.str();

    if (json_content.empty()) {
        throw std::runtime_error("Loaded JSON is empty.");
    }

    try {
        return json::parse(json_content);
    } catch (const boost::system::system_error &e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
}

model::Road LoadRoad(const json::object &road_obj) {
    int x0 = json::value_to<int>(road_obj.at(json_fields::X0));
    int y0 = json::value_to<int>(road_obj.at(json_fields::Y0));

    model_geom::Point2D start_point{x0, y0};

    // Determine if road is horizontal or vertical
    if (auto x1_it = road_obj.if_contains(json_fields::X1))
    {
        // Horizontal road
        int x1 = json::value_to<int>(*x1_it);
        return model::Road(model::Road::HORIZONTAL, start_point, x1);
    } else if (auto y1_it = road_obj.if_contains(json_fields::Y1))
    {
        // Vertical road
        int y1 = json::value_to<int>(*y1_it);
        return model::Road(model::Road::VERTICAL, start_point, y1);
    }

    throw std::runtime_error("Invalid road JSON: must contain either 'x1' or 'y1'");
}

void LoadRoads(model::Map &map, const json::array &roads_array) {
    for (const auto& road_json : roads_array) {
        const json::object& road_obj = road_json.as_object();
        model::Road road = LoadRoad(road_obj);
        map.AddRoad(road);
    }
}

model::Building LoadBuilding(const json::object &building_obj) {
    int x = json::value_to<int>(building_obj.at(json_fields::X));
    int y = json::value_to<int>(building_obj.at(json_fields::Y));
    int w = json::value_to<int>(building_obj.at(json_fields::W));
    int h = json::value_to<int>(building_obj.at(json_fields::H));

    model_geom::Rectangle2D bounds = {{x, y}, {w, h}};
    return model::Building{bounds};
}

void LoadBuildings(model::Map &map, const json::array &buildings_array) {
    for (const auto& building_json : buildings_array) {
        const json::object& building_obj = building_json.as_object();
        model::Building building = LoadBuilding(building_obj);
        map.AddBuilding(building);
    }
}

model::Office LoadOffice(const json::object &office_obj) {
    std::string office_id_str = json::value_to<std::string>(office_obj.at(json_fields::ID));
    int x = json::value_to<int>(office_obj.at(json_fields::X));
    int y = json::value_to<int>(office_obj.at(json_fields::Y));
    int offsetX = json::value_to<int>(office_obj.at(json_fields::OFFSET_X));
    int offsetY = json::value_to<int>(office_obj.at(json_fields::OFFSET_Y));

    model::Office::Id office_id{std::move(office_id_str)};
    model_geom::Point2D position{x, y};
    model_geom::Offset2D offset{offsetX, offsetY};

    return model::Office{std::move(office_id), position, offset};
}

void LoadOffices(model::Map &map, const json::array &offices_array) {
    for (const auto& office_json : offices_array) {
        const json::object& office_obj = office_json.as_object();
        model::Office office = LoadOffice(office_obj);
        map.AddOffice(std::move(office));
    }
}

extra_data::LootData ParseLoot(const json::object& loot_obj)
{
    extra_data::LootData loot;

    if (auto it = loot_obj.if_contains(json_fields::NAME); it && it->is_string())
        loot.name = it->as_string();

    if (auto it = loot_obj.if_contains(json_fields::FILE); it && it->is_string())
        loot.file = it->as_string();

    if (auto it = loot_obj.if_contains(json_fields::TYPE); it && it->is_string())
        loot.type = it->as_string();

    if (auto it = loot_obj.if_contains(json_fields::ROTATION); it && it->is_number())
        loot.rotation = it->to_number<int>();

    if (auto it = loot_obj.if_contains(json_fields::COLOR); it && it->is_string())
        loot.color = it->as_string();

    if (auto it = loot_obj.if_contains(json_fields::SCALE); it && it->is_number())
        loot.scale = it->to_number<double>();

    if (auto it = loot_obj.if_contains(json_fields::VALUE); it && it->is_number())
        loot.value = it->to_number<int>();

    return loot;
}

void LoadLootTypes(extra_data::GameExtraData* extra_data,
                       const model::Map::Id& map_id,
                       const json::array& loot_array)
{
    std::vector<extra_data::LootData> loots;
    loots.reserve(loot_array.size());

    int loot_data_type = 0;
    for (const auto& item : loot_array) {
        if (!item.is_object()) continue;
        loots.push_back(ParseLoot(item.as_object()));
        loots.back().map_id = map_id;
        loots.back().type_id = loot_data_type++;
    }

    if (!loots.empty()) {
        extra_data->AddLootTypes(map_id, std::move(loots));
    }
}

model::Map LoadMap(const json::object &map_obj,
                       double default_speed,
                       int default_capacity,
                       extra_data::GameExtraData* extra_data) {
    // ID и имя карты
    std::string map_id_str = json::value_to<std::string>(map_obj.at(json_fields::ID));
    std::string map_name = json::value_to<std::string>(map_obj.at(json_fields::NAME));
    model::Map::Id map_id{std::move(map_id_str)};
    model::Map map{map_id, std::move(map_name)};

    // Загрузка дорог, зданий, офисов
    if (auto roads_it = map_obj.if_contains(json_fields::ROADS); roads_it && roads_it->is_array())
        LoadRoads(map, roads_it->as_array());

    if (auto buildings_it = map_obj.if_contains(json_fields::BUILDINGS); buildings_it && buildings_it->is_array())
        LoadBuildings(map, buildings_it->as_array());

    if (auto offices_it = map_obj.if_contains(json_fields::OFFICES); offices_it && offices_it->is_array())
        LoadOffices(map, offices_it->as_array());

    // Определяем скорость для карты
    auto map_speed = default_speed;
    if (auto speed_it = map_obj.if_contains(json_fields::DOG_SPEED);
        speed_it && speed_it->is_number())
        {
            map_speed = speed_it->to_number<double>();
        }

    // Определяем вместимость рюкзака для карты
    auto bag_capacity = default_capacity;
    if (auto capacity_it = map_obj.if_contains(json_fields::BAG_CAPACITY);
        capacity_it && capacity_it->is_number())
        {
            bag_capacity = capacity_it->to_number<int>();
        }

    // Устанавливаем скорость и вместимость рюкзака в карту
    map.SetDefaultSpeed(app_geom::Speed2D{map_speed, map_speed});
    map.SetDefaultCapacity(bag_capacity);

    // Загружаем типы лута
    if (auto loot_it = map_obj.if_contains(json_fields::LOOT_TYPES);
        loot_it && loot_it->is_array() && !loot_it->as_array().empty())
        {
            LoadLootTypes(extra_data, map.GetId(), loot_it->as_array());
        }

    return map;
}

// Загрузка всех карт и их скоростей
void LoadMaps(const json::array& maps_array,
                  double default_speed,
                  int default_capacity,
                  model::Game& game,
                  extra_data::GameExtraData* extra_data)
{
    extra_data::GameExtraData::MapsSpeed maps_speed;
    extra_data::GameExtraData::MapsBagCapacity maps_bag_capacity;

    for (const auto& item : maps_array) {
        if (!item.is_object()) continue;
        const auto& map_obj = item.as_object();

        // Пропускаем карты без id
        if (!map_obj.if_contains(json_fields::ID)) continue;

        // Загружаем карту
        model::Map map = LoadMap(map_obj, default_speed, default_capacity, extra_data);

        // Сохраняем скорость карты в maps_speed для extra_data
        maps_speed[map.GetId()] = map.GetDefaultSpeed();
        maps_bag_capacity[map.GetId()] = map.GetDefaultCapacity();

        game.AddMap(std::move(map));
    }

    // Сохраняем доп.данные в extra_data
    extra_data->AddMapsSpeed(std::move(maps_speed));
    extra_data->AddMapsBagCapacity(std::move(maps_bag_capacity));
}

// Загрузка конфигурации генератора лута из корневого объекта JSON
void LoadLootGeneratorConfig(const json::object& root, extra_data::GameExtraData* extra_data)
{
    auto config_it = root.if_contains(json_fields::LOOT_GENERATOR_CONFIG);
    if (!config_it || !config_it->is_object() || config_it->as_object().size() == 0) {
        return; // Конфигурация отсутствует - используем значения по умолчанию
    }

    const auto& cfg = config_it->as_object();
    double period_sec = 0.0;
    double probability = common_values::DEFAULT_LOOT_CONFIG_PROBABILITY;

    if (auto period_it = cfg.if_contains(json_fields::PERIOD);
        period_it && period_it->is_number()) {
        period_sec = period_it->to_number<double>();
        }

    if (auto prob_it = cfg.if_contains(json_fields::PROBABILITY);
        prob_it && prob_it->is_number()) {
        probability = prob_it->to_number<double>();
        }

    // Преобразуем секунды в миллисекунды для хранения
    auto sec = std::chrono::duration<double>(period_sec);
    extra_data::LootGeneratorConfig loot_cfg{
        std::chrono::duration_cast<std::chrono::milliseconds>(sec),
        probability
    };
    if (loot_cfg.period.count() == 0) {
        loot_cfg.period = std::chrono::milliseconds(common_values::DEFAULT_LOOT_CONFIG_PERIOD);
    }
    extra_data->AddLootGeneratorConfig(std::move(loot_cfg));
}

double LoadDefaultSpeed(const json::object& root)
{
    if (auto def_speed = root.if_contains(json_fields::DEFAULT_DOG_SPEED);
        def_speed && def_speed->is_number()) {
        return def_speed->to_number<double>();
        }
    return common_values::DEFAULT_DOG_SPEED;
}

int LoadDefaultCapacity(const json::object& root)
{
    if (auto def_capacity = root.if_contains(json_fields::DEFAULT_BAG_CAPACITY);
        def_capacity && def_capacity->is_number()) {
        return def_capacity->to_number<int>();
        }
    return common_values::DEFAULT_BAG_CAPACITY;
}

void LoadDogRetirementTime(const json::object& root, extra_data::GameExtraData* extra_data)
{
    if (auto dog_retirement_time = root.if_contains(json_fields::DOG_RETIREMENT_TIME);
        dog_retirement_time && dog_retirement_time->is_number()) {
        std::chrono::seconds time_sec{dog_retirement_time->to_number<int>()};
        // auto time_msec = std::chrono::duration_cast<std::chrono::milliseconds>(time_sec);
        extra_data->SetDogRetirementTime(time_sec);
        }
}

} // namespace

// Main function to load the entire game from JSON file
model::Game LoadGame(const std::filesystem::path &json_path)
{
    // 1. Парсим JSON один раз
    json::value json_value = LoadJson(json_path);
    const json::object& root = json_value.as_object();

    // 2. Создаём extra_data
    auto extra_data = std::make_shared<extra_data::GameExtraData>();

    // 3. Загрузка конфигурации генератора лута (выделенная функция)
    LoadLootGeneratorConfig(root, extra_data.get());

    // 4. Загрузка базовой скорости (defaultDogSpeed) и базовой вместимости рюкзака (defaultBagCapacity)
    // Загрузка Время бездействия пса (dogRetirementTime)
    double default_dog_speed = LoadDefaultSpeed(root);
    int default_bag_capacity = LoadDefaultCapacity(root);
    LoadDogRetirementTime(root, extra_data.get());

    // 5. Проверяем наличие массива карт
    auto maps_it = root.if_contains(json_fields::MAPS);
    if (!maps_it || !maps_it->is_array()) {
        throw std::runtime_error("Invalid JSON: 'maps' array not found");
    }

    // 6. Создаём игру, передавая ссылку на extra_data
    // К этому моменту данные для инициализации LootGenerator уже доступны (см. выше LoadLootGeneratorConfig)
    model::Game game(extra_data);

    // 7. Загружаем все карты - одновременно загружается Loot внутри LoadMap
    LoadMaps(maps_it->as_array(), default_dog_speed, default_bag_capacity, game, extra_data.get());

    return {std::move(game)};
}

namespace {

void PrintRoadDetails(const model::Road& road, size_t index, std::ostream& output) {
    output << "      Road " << (index + 1) << ": ";
    if (road.IsHorizontal()) {
        output << "Horizontal from (" << road.GetStart().x << ","
               << road.GetStart().y << ") to ("
               << road.GetEnd().x << "," << road.GetEnd().y << ")";
    } else {
        output << "Vertical from (" << road.GetStart().x << ","
               << road.GetStart().y << ") to ("
               << road.GetEnd().x << "," << road.GetEnd().y << ")";
    }
    output << "\n";
}

void PrintBuildingDetails(const model::Building& building, size_t index, std::ostream& output) {
    const auto& bounds = building.GetBounds();
    output << "      Building " << (index + 1) << ": at ("
           << bounds.position.x << "," << bounds.position.y
           << ") size " << bounds.size.width << "x"
           << bounds.size.height << "\n";
}

void PrintOfficeDetails(const model::Office& office, size_t index, std::ostream& output) {
    const auto& pos = office.GetPosition();
    const auto& offset = office.GetOffset();
    output << "      Office " << (index + 1) << " (ID: "
           << std::string(*office.GetId()) << "): at ("
           << pos.x << "," << pos.y << ") offset ("
           << offset.dx << "," << offset.dy << ")\n";
}

void PrintMapDetails(const model::Map& map, bool show_details, std::ostream& output) {
    if (!show_details) {
        return;
    }

    const auto& roads = map.GetRoads();
    const auto& buildings = map.GetBuildings();
    const auto& offices = map.GetOffices();

    if (!roads.empty()) {
        output << "    Road details:\n";
        for (size_t i = 0; i < roads.size(); ++i) {
            PrintRoadDetails(roads[i], i, output);
        }
    }

    if (!buildings.empty()) {
        output << "    Building details:\n";
        for (size_t i = 0; i < buildings.size(); ++i) {
            PrintBuildingDetails(buildings[i], i, output);
        }
    }

    if (!offices.empty()) {
        output << "    Office details:\n";
        for (size_t i = 0; i < offices.size(); ++i) {
            PrintOfficeDetails(offices[i], i, output);
        }
    }
}

} // namespace

void CheckGameLoad(const model::Game& game,
                   std::ostream& output,
                   bool show_details) {
    output << "=== Game Load Statistics ===\n";

    const auto& maps = game.GetMaps();
    output << "Total maps: " << maps.size() << "\n";

    size_t total_roads = 0;
    size_t total_buildings = 0;
    size_t total_offices = 0;

    for (size_t i = 0; i < maps.size(); ++i) {
        const auto& map = maps[i];
        output << "\nMap #" << (i + 1) << ": " << map.GetName()
               << " (ID: " << std::string(*map.GetId()) << ")\n";

        const auto& roads = map.GetRoads();
        const auto& buildings = map.GetBuildings();
        const auto& offices = map.GetOffices();

        output << "  - Roads: " << roads.size() << "\n";
        output << "  - Buildings: " << buildings.size() << "\n";
        output << "  - Offices: " << offices.size() << "\n";

        total_roads += roads.size();
        total_buildings += buildings.size();
        total_offices += offices.size();

        // Print details if requested
        PrintMapDetails(map, show_details, output);
    }

    output << "\n=== Totals ===\n";
    output << "Total roads: " << total_roads << "\n";
    output << "Total buildings: " << total_buildings << "\n";
    output << "Total offices: " << total_offices << "\n";
    output << "====================\n";
}

}  // namespace json_loader
