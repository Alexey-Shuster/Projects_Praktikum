// mock_database.h
#pragma once

#include <vector>

#include "database_interface.h"
#include "player_db.h"
#include "unit_of_work.h"

namespace db {

    // Dummy repository – all methods do nothing and return empty results
    class MockPlayerScoreRepository : public PlayerScoreRepository {
    public:
        void Save(const PlayerScore& /*score*/) override {
            // no operation
        }

        std::vector<PlayerScore> GetSorted(int /*limit*/, int /*offset*/) override {
            return {};  // return empty list
        }
    };

    // Dummy unit of work – returns the dummy repository and does nothing on commit.
    class MockUnitOfWork : public UnitOfWork {
    public:
        PlayerScoreRepository& PlayerScores() override {
            return repo_;
        }

        void Commit() override {
            // no operation
        }

    private:
        MockPlayerScoreRepository repo_;
    };

    // Dummy database – creates a new MockUnitOfWork each time.
    class MockDatabase : public DatabaseInterface {
    public:
        std::unique_ptr<UnitOfWork> CreateUnitOfWork() override {
            return std::make_unique<MockUnitOfWork>();
        }
    };

} // namespace db