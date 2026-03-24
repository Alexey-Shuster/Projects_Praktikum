#pragma once

#include <filesystem>
#include <iostream>

#include "../game_model/game_model.h"

namespace json_loader {

    // Main function to load the entire Game from JSON file
    model::Game LoadGame(const std::filesystem::path& json_path);

    // Function to print statistics about loaded game objects
    // output: stream to write output to (default: std::cout)
    // show_details: if true, prints detailed information about each object
    void CheckGameLoad(const model::Game& game,
                       std::ostream& output = std::cout,
                       bool show_details = false);

}  // namespace json_loader
