# pragma once

#include <iostream>
#include <thread>

#include "../game_db/db_migrations.h"

namespace utils {

    using namespace std::literals;
    constexpr char TEST_DB_URL[] = "postgres://postgres:198487@localhost:5432/test_db";
    constexpr char GAME_DB_URL[]{"GAME_DB_URL"};

    struct AppConfig {
        std::string db_url;
    };

    AppConfig GetConfigFromEnv() {
        AppConfig config;
        if (const auto* url = std::getenv(GAME_DB_URL)) {
            config.db_url = url;
        } else {
            throw std::runtime_error(GAME_DB_URL + " environment variable not found"s);
        }
        return config;
    }

    // Helper to drop test databases after the application finishes
    inline void DropTestDatabases() {
        try {
            pqxx::connection conn{TEST_DB_URL};

            // Используем nontransaction вместо work
            pqxx::nontransaction nt{conn};

            const char* test_dbs[] = {"empty_db", "table_db", "full_db"};
            for (const char* db : test_dbs) {
                try {
                    // Добавляем FORCE (для PG 13+), чтобы разорвать существующие соединения
                    std::string query = "DROP DATABASE IF EXISTS " + nt.quote_name(db) + " WITH (FORCE);";
                    nt.exec(query);
                } catch (const std::exception& e) {
                    std::cerr << "Warning: could not drop database " << db << ": " << e.what() << std::endl;
                }
            }
            // Commit для nontransaction не требуется и ничего не делает
        } catch (const std::exception& e) {
            std::cerr << "Warning: database cleanup failed: " << e.what() << std::endl;
        }
    }

    // Запускает функцию fn на n потоках, включая текущий
    template <typename Fn>
    void RunWorkers(unsigned n, const Fn& fn) {
        n = std::max(1u, n);
        std::vector<std::jthread> workers;
        workers.reserve(n - 1);
        // Запускаем n-1 рабочих потоков, выполняющих функцию fn
        while (--n) {
            workers.emplace_back(fn);
        }
        fn();
    }

    inline void Pause() {
        // 1. Сброс флагов ошибок (если ввод был некорректным, cin "заблокируется")
        std::cin.clear();

        // 2. Очистка буфера (если не EOF) до конца строки
        if (std::cin.rdbuf()->in_avail() > 0) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }

        std::cout << "\n" << "Press Enter to close..." << std::endl;
        std::cin.get();
    }


} // namespace utils {