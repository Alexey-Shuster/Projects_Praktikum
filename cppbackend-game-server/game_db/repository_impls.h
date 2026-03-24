#pragma once

#include <pqxx/result>
#include <pqxx/row>
#include <pqxx/transaction>
#include <pqxx/zview.hxx>
#include "player_db.h"

namespace db {
    using pqxx::operator""_zv;

    class PlayerScoreRepositoryRemote : public db::PlayerScoreRepository {
    public:
        explicit PlayerScoreRepositoryRemote(pqxx::work& work) : work_(work) {}

        void Save(const db::PlayerScore& score) override {
            work_.exec_params(
                "INSERT INTO player_scores (id, player_name, score, play_time_sec) "
                "VALUES ($1, $2, $3, $4)",
                score.id.ToString(), score.name, score.score, score.play_time_sec
            );
        }

        std::vector<db::PlayerScore> GetSorted(int limit, int offset) override {
            auto rows = work_.exec_params(
                "SELECT id, player_name, score, play_time_sec FROM player_scores "
                "ORDER BY score DESC, play_time_sec ASC, player_name ASC "
                "LIMIT $1 OFFSET $2",
                limit, offset
            );
            std::vector<db::PlayerScore> result;
            for (const auto& row : rows) {
                result.push_back({
                    db::PlayerId::FromString(row[0].as<std::string>()),
                    row[1].as<std::string>(),
                    row[2].as<int>(),
                    row[3].as<double>()
                });
            }
            return result;
        }



    private:
        pqxx::work& work_;
    };

} // namespace db