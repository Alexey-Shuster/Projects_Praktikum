#pragma once

#include "database_interface.h"
#include "player_db.h"
#include "unit_of_work.h"

#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace db {

// Forward declaration for friendship
class LocalDatabase;

/**
 * @brief In‑memory implementation of PlayerScoreRepository.
 *
 * Operates on a mutable map (the transaction’s working copy) passed at construction.
 * All changes are local to that map until the enclosing unit of work commits.
 */
class PlayerScoreRepositoryLocal : public PlayerScoreRepository {
public:
    /**
     * @brief Construct a repository that uses the given map as its storage.
     * @param working_map Reference to the map owned by the current unit of work.
     */
    explicit PlayerScoreRepositoryLocal(std::map<PlayerId, PlayerScore>& working_map)
        : working_map_(working_map) {}

    /**
     * @brief Insert or update a player score.
     *
     * If a record with the same PlayerId already exists, it is overwritten.
     * @param score The player score to store.
     */
    void Save(const PlayerScore& score) override {
        working_map_[score.id] = score;
    }

    /**
     * @brief Retrieve a sorted slice of player scores.
     *
     * The order is: score descending, play_time_sec ascending, name ascending.
     * @param limit Maximum number of records to return.
     * @param offset Number of records to skip from the beginning of the sorted list.
     * @return A vector containing the requested slice.
     */
    std::vector<PlayerScore> GetSorted(int limit, int offset) override {
        // Copy all entries into a vector for sorting.
        std::vector<PlayerScore> sorted;
        sorted.reserve(working_map_.size());
        for (const auto& [id, score] : working_map_) {
            sorted.push_back(score);
        }

        // Sort according to the required order.
        std::sort(sorted.begin(), sorted.end(),
            [](const PlayerScore& a, const PlayerScore& b) {
                if (a.score != b.score)
                    return a.score > b.score;               // higher score first
                if (a.play_time_sec != b.play_time_sec)
                    return a.play_time_sec < b.play_time_sec; // less time first
                return a.name < b.name;                       // lexicographically
            });

        // Apply limit and offset.
        if (offset >= static_cast<int>(sorted.size()))
            return {};
        auto begin = sorted.begin() + offset;
        auto end = (limit <= 0 || offset + limit >= static_cast<int>(sorted.size()))
                       ? sorted.end()
                       : begin + limit;
        return {begin, end};
    }

private:
    std::map<PlayerId, PlayerScore>& working_map_; // owned by the unit of work
};

/**
 * @brief Unit of work for the local in‑memory database.
 *
 * Holds a private copy of the database snapshot. All changes are made to this copy.
 * On Commit() the modified copy is published back to the database atomically.
 * If destroyed without commit, the changes are discarded.
 */
class LocalUnitOfWork : public UnitOfWork {
public:
    /**
     * @brief Construct a unit of work from a snapshot of the main database.
     * @param db The database that created this unit of work.
     * @param snapshot A mutable copy of the current persistent data.
     */
    LocalUnitOfWork(LocalDatabase& db, std::map<PlayerId, PlayerScore> snapshot)
        : db_(db)
        , working_map_(std::move(snapshot))
        , player_repo_(working_map_) {}

    /**
     * @brief Access the player score repository bound to this transaction.
     */
    PlayerScoreRepository& PlayerScores() override {
        return player_repo_;
    }

    /**
     * @brief Commit the changes made in this unit of work.
     *
     * Publishes the modified map back to the database, making the changes
     * visible to future units of work. This operation is atomic.
     */
    void Commit() override;  // defined out-of-line after LocalDatabase is complete

    /**
     * @brief Destructor – if not committed, the working copy is simply discarded.
     */
    ~LocalUnitOfWork() override = default;
        // No action needed; working_map_ goes out of scope.

private:
    LocalDatabase& db_;
    std::map<PlayerId, PlayerScore> working_map_;
    PlayerScoreRepositoryLocal player_repo_;
    bool committed_ = false; // optional, for debugging or double‑commit prevention
};

/**
 * @brief Concrete DatabaseInterface that uses a purely in‑memory storage.
 *
 * The data is held as a const map protected by a mutex. Each unit of work
 * receives a copy of the current snapshot. On commit, the snapshot is replaced
 * atomically under the mutex.
 */
class LocalDatabase : public DatabaseInterface {
public:
    /**
     * @brief Construct an empty database.
     */
    LocalDatabase() = default;

    /**
     * @brief Create a new unit of work.
     *
     * Takes a snapshot of the current persistent data (under lock) and
     * returns a LocalUnitOfWork that will operate on that snapshot.
     */
    std::unique_ptr<UnitOfWork> CreateUnitOfWork() override {
        std::lock_guard<std::mutex> lock(mutex_);
        // Copy the entire map – snapshot isolation.
        auto snapshot = data_ ? *data_ : std::map<PlayerId, PlayerScore>{};
        return std::make_unique<LocalUnitOfWork>(*this, std::move(snapshot));
    }

    // Allow LocalUnitOfWork to call CommitTransaction
    friend class LocalUnitOfWork;

private:
    /**
     * @brief Atomically replace the persistent data with a new map.
     * @param new_data The modified map from a committing unit of work.
     */
    void CommitTransaction(std::map<PlayerId, PlayerScore>&& new_data) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = std::make_shared<const std::map<PlayerId, PlayerScore>>(std::move(new_data));
    }

    mutable std::mutex mutex_;
    std::shared_ptr<const std::map<PlayerId, PlayerScore>> data_; // initially null = empty
};

    // Out-of-line definition of LocalUnitOfWork::Commit, now that LocalDatabase is complete.
    inline void LocalUnitOfWork::Commit() {
        db_.CommitTransaction(std::move(working_map_));
        committed_ = true;
    }

} // namespace db