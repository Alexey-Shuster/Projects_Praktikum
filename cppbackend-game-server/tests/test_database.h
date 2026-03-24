#pragma once

#include <algorithm>
#include <vector>
#include "../src/game_db/database_interface.h"
#include "../src/game_db/player_db.h"

namespace db_test {
    using namespace db;

// In-memory repository that stores PlayerScore objects
class TestPlayerScoreRepository : public PlayerScoreRepository {
public:
    void Save(const PlayerScore& score) override {
        // Remove any existing entry with the same ID (to simulate UPSERT)
        auto it = std::find_if(scores_.begin(), scores_.end(),
                               [&](const PlayerScore& s) { return s.id == score.id; });
        if (it != scores_.end()) {
            *it = score;
        } else {
            scores_.push_back(score);
        }
    }

    std::vector<PlayerScore> GetSorted(int limit, int offset) override {
        // Sort a copy according to the required order
        std::vector<PlayerScore> sorted = scores_;
        std::sort(sorted.begin(), sorted.end(),
                  [](const PlayerScore& a, const PlayerScore& b) {
                      if (a.score != b.score)
                          return a.score > b.score;               // higher score first
                      if (a.play_time_sec != b.play_time_sec)
                          return a.play_time_sec < b.play_time_sec; // lower play time first
                      return a.name < b.name;                     // lexicographically
                  });

        // Apply pagination
        if (offset >= static_cast<int>(sorted.size()))
            return {};
        auto start = sorted.begin() + offset;
        auto end = (limit < 0 || offset + limit >= static_cast<int>(sorted.size()))
                       ? sorted.end()
                       : start + limit;
        return {start, end};
    }

private:
    std::vector<PlayerScore> scores_;
};

// Unit of work that returns the test repository
class TestUnitOfWork : public UnitOfWork {
public:
    PlayerScoreRepository& PlayerScores() override {
        return repo_;
    }

    void Commit() override {
        // Nothing to commit – the repository already holds the data
    }

private:
    TestPlayerScoreRepository repo_;
};

// Database that creates a new TestUnitOfWork each time
class TestDatabase : public DatabaseInterface {
public:
    std::unique_ptr<UnitOfWork> CreateUnitOfWork() override {
        return std::make_unique<TestUnitOfWork>();
    }
};

} // namespace db_test
