#pragma once

#include "database_interface.h"
#include "connection_pool.h"      // Provides ConnectionPool and ConnectionWrapper (RAII for borrowed connections)
#include "unit_of_work.h"
#include "repository_impls.h"

namespace db {

    /**
     * @brief Concrete UnitOfWork that uses a connection from the pool.
     *
     * This class ties together a borrowed connection (via ConnectionWrapper),
     * a libpqxx transaction (pqxx::work), and a repository implementation that
     * operates within that transaction. The connection is automatically returned
     * to the pool when the PooledUnitOfWork object is destroyed (RAII).
     */
    class PooledUnitOfWork : public UnitOfWork {
    public:
        /**
         * @brief Construct a unit of work that owns a borrowed connection.
         *
         * @param conn_wrapper A ConnectionWrapper obtained from ConnectionPool::GetConnection().
         *                     The wrapper holds the connection and will return it on destruction.
         *
         * The constructor also creates a pqxx::work transaction from the connection and
         * initializes the player repository with that transaction.
         */
        explicit PooledUnitOfWork(ConnectionPool::ConnectionWrapper&& conn_wrapper)
            : conn_wrapper_(std::move(conn_wrapper))       // Take ownership of the wrapper
            , work_(*conn_wrapper_)                     // Create a transaction on the borrowed connection
            , player_repo_(work_) {}                    // Pass the transaction to the repository

        /**
         * @brief Returns a reference to the player repository.
         *
         * Overrides the pure virtual method from UnitOfWork.
         * The repository is already bound to the transaction.
         */
        PlayerScoreRepository& PlayerScores() override {
            return player_repo_;
        }

        /**
         * @brief Commits the transaction.
         *
         * Overrides UnitOfWork::Commit(). If this method is not called,
         * the transaction will be rolled back when the pqxx::work object is destroyed.
         */
        void Commit() override {
            work_.commit();   // Commit all operations performed within this unit of work
        }

    private:
        // RAII wrapper that holds the borrowed connection and returns it to the pool upon destruction.
        ConnectionPool::ConnectionWrapper conn_wrapper_;

        // libpqxx transaction object. It uses the connection managed by conn_wrapper_.
        // All database operations in this unit of work are executed inside this transaction.
        pqxx::work work_;

        // Concrete repository implementation. It is constructed with the transaction (work_)
        // and executes queries through it.
        PlayerScoreRepositoryRemote player_repo_;
    };

    /**
     * @brief Concrete DatabaseInterface that creates UnitOfWork objects backed by a connection pool.
     *
     * This class is a factory for creating PooledUnitOfWork instances. Each unit of work
     * obtains a connection from the pool and returns it when done.
     */
    class PooledDatabase : public DatabaseInterface {
    public:
        /**
         * @brief Construct a pooled database accessor.
         *
         * @param pool Shared pointer to a ConnectionPool. The pool is kept alive
         *             as long as this PooledDatabase exists (or any other shared owners).
         */
        explicit PooledDatabase(std::shared_ptr<ConnectionPool> pool)
            : pool_(std::move(pool))
        {}

        /**
         * @brief Create a new unit of work.
         *
         * This method obtains a connection from the pool (blocking if necessary),
         * wraps it in a ConnectionWrapper, and creates a PooledUnitOfWork that owns
         * that wrapper. The unit of work is returned as a polymorphic pointer to the
         * UnitOfWork interface.
         *
         * @return std::unique_ptr<UnitOfWork> pointing to a newly created PooledUnitOfWork.
         */
        std::unique_ptr<UnitOfWork> CreateUnitOfWork() override {
            // Get a connection from the pool. GetConnection() returns a ConnectionWrapper.
            // PooledUnitOfWork is constructed, taking ownership of that wrapper.
            return std::make_unique<PooledUnitOfWork>(pool_->GetConnection());
        }

    private:
        std::shared_ptr<ConnectionPool> pool_;   // Shared pool of database connections
    };

} // namespace db