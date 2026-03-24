#pragma once

#include <chrono>
#include "game_state_persistence.h"

namespace app {

    class AutoSaveManager {
    public:
        AutoSaveManager(const model::Game& game, const Players& players,
                        std::chrono::milliseconds period, std::string filename)
            : game_(game)
            , players_(players)
            , period_(period)
            , filename_(std::move(filename))
        {}

        void OnTick(std::chrono::milliseconds delta) {
            // Disabled if period is zero or filename is empty
            if (period_.count() == 0 || filename_.empty())
                return;

            accumulated_ += delta;
            if (accumulated_ >= period_) {
                boost_logger::LogInfo("AutoSave after msec: " + std::to_string(accumulated_.count()));
                GameStatePersistence::Save(game_, players_, filename_);
                while (accumulated_ - period_ >= std::chrono::milliseconds::zero()) {
                    accumulated_ -= period_;
                }
            }
        }

    private:
        const model::Game& game_;
        const Players& players_;
        const std::chrono::milliseconds period_;
        const std::string filename_;
        std::chrono::milliseconds accumulated_{0};
    };

} // namespace app