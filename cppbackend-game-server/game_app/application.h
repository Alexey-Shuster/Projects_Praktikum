#pragma once

#include <boost/signals2.hpp>

#include "../common/cmd_parser.h"
#include "game_clock.h"
#include "game_state_persistence.h"
#include "auto_save_manager.h"
#include "player_score_recorder.h"

namespace app {

class Application {
public:
    using GameSaveSignal = boost::signals2::signal<void(std::chrono::milliseconds delta)>;

    explicit Application(model::Game& game, const parse::Args& cmd_args,
                            boost::asio::io_context &ioc, db::DatabaseInterface& db)
        : game_(game)
        , players_(game)
        , cmd_args_(cmd_args)
        , ioc_(ioc)
        , game_extra_data_(game_.GetGameExtraData())
        , database_(db)
        , auto_save_manager_(game_, players_,
                           std::chrono::milliseconds(cmd_args_.save_state_period),
                           cmd_args_.state_file)
        , score_recorder_(db)
    {
        game_.SetCreateBots(cmd_args_.enable_bots);
        game_.SetEnableRetirement(!cmd_args_.no_database);
        if (!cmd_args_.no_database) {
            SetupPlayerRetirementHandling();
        }
    };

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    model::Game& GetGame() {
        return game_;
    }

    const model::Game& GetGame() const {
        return game_;
    }

    Players& GetPlayers() {
        return players_;
    }

    const Players& GetPlayers() const {
        return players_;
    }

    const parse::Args& GetCmdArgs() const {
        return cmd_args_;
    }

    void Tick(std::chrono::milliseconds time_delta) {
        clock_.Advance(time_delta);
        game_.UpdateAllGameSessions(time_delta);
        auto_save_manager_.OnTick(time_delta);
        save_signal_(time_delta);   // Notify external subscribers (e.g. tests)
    }

    [[nodiscard]] const Player& AddPlayer(std::string_view name, std::string_view map) {
        auto new_player = &players_.AddPlayer(name, map, clock_.Now(), ioc_);
        if (cmd_args_.randomize_spawn_points) {
            new_player->GetDog()->SetPosition(new_player->GetGameSession()->GetMap()->GetRandomPoint());
        }
        return *new_player;
    }

    // Optional: if external save/load needed
    void SaveGameState(const std::string& filename) const {
        GameStatePersistence::Save(game_, players_, filename);
    }

    void LoadGameState(const std::string& filename) {
        GameStatePersistence::Load(filename, game_, players_, ioc_);
    }

    // External subscribing to save signal (e.g. tests)
    [[nodiscard]] boost::signals2::connection OnGameSave(const GameSaveSignal::slot_type& handler) {
        return save_signal_.connect(handler);
    }

    std::vector<db::PlayerScore> GetPlayerScores(int limit, int offset) const {
        auto uow = database_.CreateUnitOfWork();
        return uow->PlayerScores().GetSorted(limit, offset);
    }

private:
    model::Game& game_;
    Players players_;
    const parse::Args& cmd_args_;
    boost::asio::io_context &ioc_;
    std::shared_ptr<extra_data::GameExtraData> game_extra_data_;

    GameSaveSignal save_signal_;     // for any external subscribers (e.g. tests)

    // Dog & Player retirement control
    db::DatabaseInterface& database_;

    GameClock clock_;
    AutoSaveManager auto_save_manager_;
    PlayerScoreRecorder score_recorder_;

    // Sets up handling of player retirement events when a database is used.
    // Subscribes to Players::OnPlayerRetired to record the player's final score
    // and play duration into the database. This is only enabled when
    // no_remote_database is false (i.e., we are using a database).
    void SetupPlayerRetirementHandling() {
        // Subscribe to the Players' retirement signal.
        // The signal provides dog_id, score, and a reference to the Player object.
        players_.OnPlayerRetired([this](uint32_t dog_id, loot::Score dog_score, const Player& player) {
            // Extra safety: skip bots (though they are already filtered in Players)
            if (dog_id >= common_values::DOG_BOT_START_ID)
                return;

            // Compute how long the player was in the game.
            auto play_time_ms = clock_.Now() - player.GetJoinTime();

            // Save the player's score to the database.
            score_recorder_.SaveScore(player, dog_score, play_time_ms);

            // The player is automatically removed from Players' containers after the signal.
        });
    }
};

} // namespace app
