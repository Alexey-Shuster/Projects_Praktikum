#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/archive_exception.hpp>
#include "../game_repr/game_repr.h"
#include "../common/boost_logger.h"

namespace app {

class GameStatePersistence {
public:
    static void Save(const model::Game& game, const Players& players, const std::string& filename) {
        std::ofstream ofs(filename, std::ios::out | std::ios::trunc);
        if (!ofs.is_open()) {
            std::string error_msg = "Could not open file " + filename + " for writing.";
            boost_logger::LogError(EXIT_FAILURE, error_msg, "GameStatePersistence::Save");
            throw std::runtime_error(error_msg);
        }

        try {
            boost_logger::LogInfo("Game saving started");
            boost::archive::text_oarchive oa(ofs);
            serialize_game_save::GameRepr repr(game, players);
            oa << repr;
            boost_logger::LogInfo("Game successfully saved in: " + filename);
        } catch (const boost::archive::archive_exception& e) {
            boost_logger::LogError(EXIT_FAILURE, e.what(), "GameStatePersistence::Save");
            throw std::runtime_error(e.what());
        } catch (const std::exception& e) {
            boost_logger::LogError(EXIT_FAILURE, e.what(), "GameStatePersistence::Save");
            throw std::runtime_error(e.what());
        }
    }

    static void Load(const std::string& filename, model::Game& game, Players& players, boost::asio::io_context& ioc) {
        if (!std::filesystem::exists(filename)) {
            boost_logger::LogInfo("Game state file does not exist. Starting new Game...");
            return;
        }

        std::ifstream ifs(filename);
        boost::archive::text_iarchive ia(ifs);

        try {
            boost_logger::LogInfo("Game loading started");
            serialize_game_save::GameRepr repr;
            ia >> repr;
            repr.Restore(game, players, ioc);
            boost_logger::LogInfo("Game successfully loaded from: " + filename);
        } catch (const boost::archive::archive_exception& e) {
            ia.delete_created_pointers();
            // Detailed error logging
            switch (e.code) {
                case boost::archive::archive_exception::invalid_signature:
                    boost_logger::LogError(EXIT_FAILURE, "File is not a valid Boost archive.",
                        "GameStatePersistence::Load");
                    break;
                case boost::archive::archive_exception::unsupported_version:
                    boost_logger::LogError(EXIT_FAILURE, "Archive was created with a newer library version.",
                        "GameStatePersistence::Load");
                    break;
                case boost::archive::archive_exception::input_stream_error:
                    boost_logger::LogError(EXIT_FAILURE, "Data corrupted or unexpected end of file.",
                        "GameStatePersistence::Load");
                    break;
                case boost::archive::archive_exception::unregistered_class:
                    boost_logger::LogError(EXIT_FAILURE, "Attempted to load a class not registered with the archive.",
                        "GameStatePersistence::Load");
                    break;
                default:
                    boost_logger::LogError(EXIT_FAILURE, "Boost Archive Error: " + std::string(e.what()),
                        "GameStatePersistence::Load");
                    break;
            }
            throw;
        } catch (const std::exception& e) {
            boost_logger::LogError(EXIT_FAILURE, e.what(), "GameStatePersistence::Load");
            throw;
        }
    }
};

} // namespace app