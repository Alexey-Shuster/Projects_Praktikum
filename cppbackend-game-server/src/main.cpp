#include "common/sdk.h"     // ensures consistent Windows version targeting for #include <sdkddkver.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include "common/main_utils.h"
#include "common/cmd_parser.h"
#include "common/json_loader.h"
#include "http_server/logging_request_handler.h"

#include "game_db/pooled_database.h"    // real remote DB
#include "game_db/mock_database.h"      // dummy when DB is disabled
#include "game_db/local_database.h"     // local DB

namespace {
    namespace net = boost::asio;
    namespace sys = boost::system;
}  // namespace

int main(int argc, const char* argv[]) {

    try {
        // 0a. Parse command line
        std::optional<parse::Args> args = parse::ParseCommandLine(argc, argv).value();

        // 0b. Logger initialize & configure
        boost_logger::InitBoostLogSetFilter();
        boost_logger::SendBoostLogToStream();

        // 0c. Initialize io_context
        const unsigned num_threads = std::max(1u, std::thread::hardware_concurrency());
        net::io_context ioc(num_threads);

        // 1a. Load Game data from config-file and build game model
        auto game_data = json_loader::LoadGame(args.value().config_file);
        // json_loader::CheckGameLoad(game);    // Debug


        // 1b. Process database
        std::unique_ptr<db::DatabaseInterface> database;

        if (args->no_database) {
            // MockDatabase - skip connection pool and migrations
            database = std::make_unique<db::MockDatabase>();
        } else {
            if (args->local_database) {
                database = std::make_unique<db::LocalDatabase>();
            } else {

                // Create real connection pool, run migrations, and instantiate PooledDatabase
                auto conn_factory = [db_url = utils::GetConfigFromEnv().db_url] {
                    return std::make_shared<pqxx::connection>(db_url);
                };
                auto pool = std::make_shared<db::ConnectionPool>(num_threads, conn_factory);

                // Run schema migrations & Create the pooled database object
                db::RunMigrations(pool);
                database = std::make_unique<db::PooledDatabase>(pool);
            }
        }

        // 2a. Create Application Layer
        app::Application app(game_data, args.value(), ioc, *database);

        // 2b. Load Game if required
        if (!args.value().state_file.empty()) {
            app.LoadGameState(args->state_file);
        }

        // 3. Add asynchronous signal handler for SIGINT & SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        // 4a. Create HTTP request handler and link it with the game
        auto handler = std::make_shared<http_handler::RequestHandler>(
            app, ioc);

        // 4b. Wrap with logging decorator
        server_logging::LoggingRequestHandler logging_handler{
                                                              [handler](auto&& endpoint, auto&& req, auto&& send) {
                                                                  (*handler)(
                                                                             std::forward<decltype(req)>(req),
                                                                             std::forward<decltype(send)>(send)
                                                                            );
                                                              }};

        // 5. Launch the HTTP request handler
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;

        http_server::ServeHttp(ioc, {address, port}, logging_handler);

        // This informs tests that the server is running and ready to process requests
        boost_logger::LogServerStarted(port, address.to_string());

        // 6. Start processing asynchronous operations
        utils::RunWorkers(num_threads, [&ioc] {
            ioc.run();
        });

        // 7. Save Game state if required
        if (!args.value().state_file.empty()) {
            app.SaveGameState(args->state_file);
        }

        boost_logger::LogServerExited(EXIT_SUCCESS);
    } catch (const std::exception& e) {
        boost_logger::LogServerExited(EXIT_FAILURE, e.what());
        utils::Pause();
        return EXIT_FAILURE;
    } catch (...) {
        boost_logger::LogError(EXIT_FAILURE, std::string(http_handler::error_messages::UNKNOWN_ERROR), "");
        utils::Pause();
        return EXIT_FAILURE;
    }
    utils::Pause();
} // main
