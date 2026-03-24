#pragma once

#include <iostream>
#include <optional>
#include <string>

#include <boost/program_options.hpp>

namespace parse {

constexpr static inline const char* TICK_PERIOD = "tick-period";
constexpr static inline const char* CONFIG_FILE = "config-file";
constexpr static inline const char* WWW_ROOT = "www-root";
constexpr static inline const char* SPAWN_POINTS = "randomize-spawn-points";

using namespace std::literals;

struct Args {
    uint32_t tick_period{0};
    std::string config_file = "./data/config.json"s;
    std::string www_root = "./static"s;
    std::string state_file{};
    uint32_t save_state_period{0};
    bool randomize_spawn_points{false};     // players spawns randomly
    bool enable_bots{false};                // enable-bots for each GameSession
    bool no_database{false};         // if remote database used to save Players score
    // Hidden options
    bool local_database{false};             // if local database used to save Players score
};

[[nodiscard]] inline bool ValidateDirectory(const std::filesystem::path& path, std::string& error_message) {
    namespace fs = std::filesystem;
    if (!fs::is_directory(path)) {
        error_message = "Error: path is not a directory: " + path.string();
        return false;
    }
    return true;
}

[[nodiscard]] inline bool ValidateArgs(const Args& args, std::string& error_message) {
    namespace fs = std::filesystem;

    // Validate tick_period
    if (args.tick_period < 0) {
        error_message = "Error: tick-period cannot be negative";
        return false;
    }

    // Validate config_file
    if (args.config_file.empty()) {
        error_message = "Error: config-file path cannot be empty";
        return false;
    }

    // Validate www_root
    if (args.www_root.empty()) {
        error_message = "Error: www-root path cannot be empty";
        return false;
    }

    // Check if config file exists and is readable
    std::ifstream f(args.config_file.c_str());
    if (!f.good()) {
        error_message = "Error: config-file does not exist or is not readable";
        return false;
    }

    // Check if www_root directory exists
    fs::path www_root_dir = fs::absolute(args.www_root);
    if (!ValidateDirectory(www_root_dir, error_message)) {
        return false;
    }

    if (args.state_file.empty()) {
        return true;
    }

    // Check if state_file directory exists and not equal to www_root
    fs::path state_file_path = fs::absolute(args.state_file);
    fs::path state_file_dir = state_file_path.parent_path();
    if (!fs::exists(state_file_dir)) {
        try {
            // create_directories
            fs::create_directories(state_file_dir);
        } catch (const fs::filesystem_error& e) {
            error_message = "Error creating directory: [" + state_file_dir.string() + "] - " + std::string(e.what());
            return false;
        }
    } 
    if (!ValidateDirectory(state_file_dir, error_message)) {
        return false;
    }

    if (state_file_dir == www_root_dir) {
        error_message = "Error: Game saved in www-root: "
                        + state_file_dir.string() + "==" + www_root_dir.string();
        return false;
    }

    return true;
}

[[nodiscard]] inline std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {

    namespace po = boost::program_options;

    Args args;
    // return args;

    po::options_description required("Required options");

    required.add_options()
        // Опция --config-file, задаёт путь к конфигурационному JSON-файлу игры
        ("config-file,c",
            po::value(&args.config_file)->value_name("file"s),
            "Set path to the game's JSON configuration file")

        // Опция --www-root, задаёт путь к каталогу со статическими файлами игры
        ("www-root,w", po::value(&args.www_root)->value_name("dir"s),
            "Set path to the static game files");

    po::options_description game_optional("Game settings [optional]");

    game_optional.add_options()
        // Опция --help и её короткая версию -h
        ("help,h", "Show help")

        // Опция --tick-period, задаёт период автоматического обновления игрового состояния в миллисекундах
        ("tick-period,t",
            po::value(&args.tick_period)->value_name("milliseconds"s),
            "Set period for automatic updating of game state")

        // Опция --state-file, задаёт путь к файлу сохранения игры
        ("state-file,s", po::value(&args.state_file)->value_name("file"s),
            "Set path to the game save file")

        // Опция --save-state-period, задаёт период автоматического сохранения игрового состояния в миллисекундах
        ("save-state-period,p",
            po::value(&args.save_state_period)->value_name("milliseconds"s),
            "Set period for automatic saving of game state")

        // Boolean flags (presence = true, absence = false)
        // Опция --bots, добавляет ботов для каждой игровой сессии
        ("bots,b",
            po::bool_switch(&args.enable_bots),
            "Set bots for each GameSession (bool flag, no value needed, default - false)")

        // Опция --randomize-spawn-points, включает режим, при котором пёс игрока появляется в случайной точке случайно выбранной дороги карты
        ("randomize-spawn-points,r",
            po::bool_switch(&args.randomize_spawn_points),
            "Spawn dogs at random positions (bool flag, no value needed, default - false)")

        // if remote database used to save Players score
        ("no_db,n",
            po::bool_switch(&args.no_database),
            "Turn OFF database - Player's score not saved (bool flag, no value needed, default - false)")

        // if local database used to save Players score
        ("lcl_db,l",
            po::bool_switch(&args.local_database),
            "Use Local (w\\o SQL) database to save Player's score (bool flag, no value needed, default - false)");

    po::options_description hidden("Hidden options");

    po::options_description all_options{"All options"s};
    all_options.add(required).add(game_optional).add(hidden);

    po::options_description visible_options;
    visible_options.add(required).add(game_optional);

    // variables_map хранит значения опций после разбора
    po::variables_map vm;
    try {
        po::store(
    po::command_line_parser(argc, argv)
                .options(all_options)
                .style(po::command_line_style::default_style)
                .run(),
            vm
        );
        po::notify(vm);
    } catch (const po::error& e) {
        std::cerr << "Command line parsing error: " << e.what() << std::endl;
        std::cout << visible_options << std::endl;
        return std::nullopt;
    }

    if (vm.contains("help"s)) {
        // Если был указан параметр --help, то выводим справку и возвращаем nullopt
        std::cout << visible_options;
        return std::nullopt;
    }

    if (!vm.contains(CONFIG_FILE) && !vm.contains(WWW_ROOT)) {
        std::cerr << "Minimum Usage: game_server <game-config.json> <static-files-path>."sv << std::endl;
        std::cout << visible_options << std::endl;
        return std::nullopt;
    }

    // Проверяем наличие опций
    if (!vm.contains(CONFIG_FILE)) {
        throw std::runtime_error("JSON configuration file not specified"s);
    }
    if (!vm.contains(WWW_ROOT)) {
        throw std::runtime_error("Path to static game files not specified"s);
    }

    std::string error_message;
    if (!ValidateArgs(args, error_message)) {
        std::cerr << error_message << std::endl;
        return std::nullopt;
    }

    // С опциями программы всё в порядке, возвращаем структуру args
    return args;
}

} // namespace parse
