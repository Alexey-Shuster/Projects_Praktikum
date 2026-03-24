#pragma once

#include "connection_pool.h"
#include "../common/boost_logger.h"

namespace db {

    inline void RunMigrations(std::shared_ptr<db::ConnectionPool> pool) {
        using pqxx::operator""_zv;

        // Obtain a connection from the pool
        auto conn = pool.get()->GetConnection();
        pqxx::work work(*conn);

        // Create tables and indexes (idempotent)
        work.exec(R"(
                    CREATE TABLE IF NOT EXISTS player_scores (
                        id             UUID PRIMARY KEY,
                        player_name    TEXT NOT NULL,
                        score          INTEGER NOT NULL CHECK (score >= 0),
                        play_time_sec  DOUBLE PRECISION NOT NULL CHECK (play_time_sec >= 0)
                    );
                )"_zv);

        work.exec(R"(
                    CREATE INDEX IF NOT EXISTS idx_player_scores_sort
                        ON player_scores (score DESC, play_time_sec ASC, player_name ASC);
                )"_zv);

        work.commit();

        // boost_logger::LogInfo("RunMigrations: Database migrations applied successfully.");
    }

} // namespace db