#pragma once

#include <condition_variable>
#include <mutex>
#include <pqxx/pqxx>

namespace db {

    // Timeout for waiting to acquire a connection from the pool
    constexpr static inline std::chrono::seconds CONNECTION_TIMEOUT_SEC {60};

    /**
     * @brief Thread-safe pool of PostgreSQL connections.
     *
     * Manages a fixed number of pqxx::connection objects. Clients borrow
     * connections via GetConnection(), which returns a RAII wrapper that
     * automatically returns the connection when destroyed.
     */
    class ConnectionPool {
        using PoolType = ConnectionPool;
        using ConnectionPtr = std::shared_ptr<pqxx::connection>;

    public:
        /**
         * @brief RAII wrapper for a borrowed connection.
         *
         * Provides pointer-like access to the underlying connection.
         * When destroyed, the connection is returned to the pool.
         */
        class ConnectionWrapper {
        public:
            // Constructor: takes ownership of a connection and remembers the pool.
            ConnectionWrapper(std::shared_ptr<pqxx::connection>&& conn, PoolType& pool) noexcept
                : conn_{std::move(conn)}
                , pool_{&pool} {
            }

            // Non-copyable to avoid confusion about ownership.
            ConnectionWrapper(const ConnectionWrapper&) = delete;
            ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

            // Move operations are allowed (transfer ownership).
            ConnectionWrapper(ConnectionWrapper&&) noexcept = default;
            ConnectionWrapper& operator=(ConnectionWrapper&&) noexcept = default;

            // Dereference to access the connection (lvalue only).
            pqxx::connection& operator*() const& noexcept {
                return *conn_;
            }
            // Prevent dereferencing a temporary wrapper.
            pqxx::connection& operator*() const&& = delete;

            // Arrow operator for member access.
            pqxx::connection* operator->() const& noexcept {
                return conn_.get();
            }

            // Destructor: returns the connection to the pool.
            ~ConnectionWrapper() {
                if (conn_) {
                    pool_->ReturnConnection(std::move(conn_));
                }
            }

        private:
            std::shared_ptr<pqxx::connection> conn_; // The borrowed connection
            PoolType* pool_;                          // Back-pointer to the pool
        };

        /**
         * @brief Constructs a pool with a fixed number of connections.
         *
         * @tparam ConnectionFactory A callable that returns a ConnectionPtr
         * @param capacity Number of connections to create
         * @param connection_factory Factory function invoked for each connection
         * @throws std::runtime_error if any created connection is invalid or not open
         */
        template <typename ConnectionFactory>
        ConnectionPool(size_t capacity, ConnectionFactory&& connection_factory) {
            pool_.reserve(capacity);
            for (size_t i = 0; i < capacity; ++i) {
                auto conn = connection_factory();               // Create a new connection
                if (!conn || !conn->is_open()) {                // Validate it
                    throw std::runtime_error("ConnectionPool: Failed to create a valid database connection");
                }
                pool_.emplace_back(connection_factory());       // Store it in the pool
            }
        }

        /**
         * @brief Borrow a connection from the pool.
         *
         * If no connection is available, the calling thread blocks until one
         * becomes free or the timeout expires.
         *
         * @return ConnectionWrapper holding a valid connection
         * @throws std::runtime_error if timeout occurs or the connection is invalid
         */
        ConnectionWrapper GetConnection() {
            std::unique_lock lock{mutex_};

            // First wait: block until at least one connection is free.
            // (This condition may already be true, so wait returns immediately.)
            cond_var_.wait(lock, [this] {
                return used_connections_ < pool_.size();
            });

            // Second wait: now wait with a timeout for a free slot.
            // If the timeout expires, throw an exception.
            if (!cond_var_.wait_for(lock, CONNECTION_TIMEOUT_SEC, [this] {
                    return used_connections_ < pool_.size();
                }))
            {
                throw std::runtime_error("ConnectionPool::GetConnection: Timeout waiting for a free connection");
            }

            // At this point we hold the lock and a connection is available.
            // Connections are stored contiguously; the first 'used_connections_'
            // slots are currently in use, the rest are free. We take the next free slot.
            auto& conn_ptr = pool_[used_connections_];

            // Optional validation: if the connection has been closed (e.g., by the server),
            // we cannot use it. For simplicity, we just throw.
            if (!conn_ptr->is_open()) {
                throw std::runtime_error("ConnectionPool::GetConnection: Connection is no longer valid");
            }

            ++used_connections_;                // Mark as used
            return {std::move(conn_ptr), *this}; // Return wrapper that owns the connection
        }

        /// Returns the total number of connections managed by the pool.
        [[nodiscard]] size_t Size() const {
            return pool_.size();
        }

        /// Returns the number of connections currently in use.
        [[nodiscard]] size_t Used() const {
            return used_connections_;
        }

        /// Returns the number of connections available for borrowing.
        [[nodiscard]] size_t Available() const {
            return pool_.size() - used_connections_;
        }

    private:
        /**
         * @brief Return a connection to the pool.
         *
         * Called automatically by ConnectionWrapper destructor.
         * @param conn The connection being returned (must be non-null).
         */
        void ReturnConnection(ConnectionPtr&& conn) {
            {
                std::lock_guard lock{mutex_};
                if (used_connections_ == 0) {
                    // This should never happen if the pool logic is correct.
                    throw std::runtime_error("ConnectionPool::ReturnConnection: Attempt to return a connection when none are used");
                }
                // Connections are stored in a vector; we maintain that the first
                // 'used_connections_' slots are occupied. To return a connection,
                // we decrement used_connections_ and place the returned connection
                // into the slot that is now free.
                pool_[--used_connections_] = std::move(conn);
            }
            // Notify one waiting thread that a connection has become available.
            cond_var_.notify_one();
        }

        std::mutex mutex_;                    // Protects pool_ and used_connections_
        std::condition_variable cond_var_;    // Used for blocking and signalling
        std::vector<ConnectionPtr> pool_;      // The actual connection storage
        size_t used_connections_ = 0;          // Number of connections currently borrowed
    };

} // namespace db