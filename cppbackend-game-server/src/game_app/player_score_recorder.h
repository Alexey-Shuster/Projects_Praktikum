#pragma once

#include "../game_db/player_db.h"
#include "../game_db/pooled_database.h"
#include "../common/boost_logger.h"
#include "players.h"

namespace app {

    class PlayerScoreRecorder {
    public:
        explicit PlayerScoreRecorder(db::DatabaseInterface& db) : database_(db) {}

        void SaveScore(const Player& player, loot::Score score, std::chrono::milliseconds play_time_ms) const {
            try {
                auto uow = database_.CreateUnitOfWork();
                uow->PlayerScores().Save({
                    .id = db::PlayerId::New(),
                    .name = player.GetName(),
                    .score = static_cast<int>(score),
                    .play_time_sec = std::chrono::duration<double>(play_time_ms).count()
                });
                uow->Commit();
            } catch (const std::exception& e) {
                boost_logger::LogError(EXIT_FAILURE, e.what(), "PlayerScoreRecorder::SaveScore");
            }
        }

        [[nodiscard]] std::vector<db::PlayerScore> GetTopScores(int limit, int offset) const {
            auto uow = database_.CreateUnitOfWork();
            return uow->PlayerScores().GetSorted(limit, offset);
        }

    private:
        db::DatabaseInterface& database_;
    };

} // namespace app