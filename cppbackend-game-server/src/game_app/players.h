#pragma once

#include <boost/signals2.hpp>
#include <map>
#include <memory>
#include <unordered_map>
#include <optional>

#include "../game_model/game_model.h"
#include "../common/tagged.h"
#include "token.h"

namespace app {

class Player {
public:
    explicit Player(std::uint32_t id, std::string name, model::GameSession* session,
                    std::chrono::milliseconds join_time, bool restored = false)
        : id_(id)
        , name_(std::move(name))
        , session_(session)
        , join_time_(join_time)
    {
        dog_ = session_->RequestDog(id_, name_, restored);
    }

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    [[nodiscard]] std::uint32_t GetId() const {
        return id_;
    }
    [[nodiscard]] const std::string& GetName() const {
        return name_;
    }

    [[nodiscard]] app_geom::Position2D GetPosition() const {
        return dog_ ? dog_->GetPosition() : throw std::runtime_error("Dog* = nullptr.");
    }
    [[nodiscard]] app_geom::Speed2D GetSpeed() const {
        return dog_ ? dog_->GetSpeed() : throw std::runtime_error("Dog* = nullptr.");
    }
    [[nodiscard]] app_geom::Direction2D GetDirection() const {
        return dog_ ? dog_->GetDirection() : throw std::runtime_error("Dog* = nullptr.");
    }
    [[nodiscard]] const model::GameSession* GetGameSession() const {
        return session_;
    }
    [[nodiscard]] model::Dog* GetDog() const {
        return dog_;
    }

    void SetDirection(app_geom::Direction2D dir);

    [[nodiscard]] std::chrono::milliseconds GetJoinTime() const noexcept {
        return join_time_;
    }

private:
    std::uint32_t id_{};
    std::string name_;
    model::GameSession* session_ = nullptr;
    model::Dog* dog_ = nullptr;
    std::chrono::milliseconds join_time_{};
};

class Players {
public:
    // Signal emitted when a player (real player, not a bot) is retired from the game.
    // Retirement occurs when the associated dog has been idle for too long (dog retirement timeout).
    // The signal is triggered from the GameSession's dog-deleted signal, which is connected
    // in ConnectToDogDeletedSignal. The signal provides:
    // - dog_id: the ID of the dog (same as player ID)
    // - dog_score: the total score accumulated by the dog before retirement
    // - player: a const reference to the Player object (still valid at emission)
    // Subscribers can use this to record scores, notify external systems, etc.
    // After all subscribers finish, the player is removed from internal containers.
    using PlayerRetiredSignal = boost::signals2::signal<void(uint32_t dog_id, loot::Score dog_score, const Player& player)>;

    explicit Players(model::Game& game)
        : game_(game)
    {};

    Players(const Players&) = delete;
    Players& operator=(const Players&) = delete;

    [[nodiscard]] Player& AddPlayer(std::string_view name, std::string_view map,
                                            std::chrono::milliseconds join_time, boost::asio::io_context &ioc);

    const Token* FindTokenByPlayer(const Player& player) const;

    const Player* FindPlayerByToken(const Token& token) const;

    Player* FindPlayerByToken(const Token& token);

    const Player* FindPlayerById(std::uint32_t player_id) const;

    // Get all players ordered by ID
    const std::map<std::uint32_t, const Player*> GetPlayersAll() const;

    // Get players for selected Map from all GameSessions
    const std::map<std::uint32_t, const Player*> GetPlayersByMap(const model::Map::Id &map_id) const;

    // Get players for selected Map for current GameSession
    const std::map<std::uint32_t, const Player*> GetPlayersBySession(const model::GameSession::Id& session_id) const;

    uint32_t GetNextPlayerId() const {
        return next_player_id_;
    }

    void SetNextPlayerId(uint32_t id) {
        next_player_id_ = id;
    }

    const std::unordered_map<uint32_t, const Player*>& GetAllPlayers() const {
        return player_id_to_player_;
    }

    // Create player with given ID and insert into all containers
    void AddRestoredPlayer(uint32_t id, std::string_view name, model::GameSession* session, std::string_view token);

    void RemoveRetiredPlayer(uint32_t player_id) {
        auto it = player_id_to_player_.find(player_id);
        // should not happen
        if (it == player_id_to_player_.end()) {
            throw std::runtime_error("Players::RemoveRetiredPlayer: Player not found");
        };
        // Remove from token map
        const Token* token = player_id_to_token_[player_id];
        token_to_player_.erase(*token);
        // Remove from helper maps
        player_id_to_token_.erase(player_id);
        player_id_to_player_.erase(player_id);
    }

    // Return a connection to the retirement signal.
    // Used in Application::SetupPlayerRetirementHandling
    boost::signals2::connection OnPlayerRetired(const PlayerRetiredSignal::slot_type& slot) {
        return player_retired_signal_.connect(slot);
    }

private:
    model::Game& game_;
    uint32_t next_player_id_ = 1;   // same used for Dog id
    TokenGen token_gen_;

    // origin Token & Player data storage
    std::unordered_map<Token, std::unique_ptr<Player>, util::TaggedHasher<Token>> token_to_player_;
    // helper containers for quick search
    std::map<std::uint32_t, const Token*> player_id_to_token_;
    std::unordered_map<std::uint32_t, const Player*> player_id_to_player_;

    std::function<void(model::GameSession* session)> player_retire_handler_{nullptr};

    // Signal is emitted when player (bots exluded) is retired.
    PlayerRetiredSignal player_retired_signal_;

    // Map from session ID to the connection to that session's DogDeletedSignal.
    // We keep exactly one connection per session to avoid duplicate handlers.
    std::unordered_map<model::GameSession::Id, boost::signals2::connection,
                       util::TaggedHasher<model::GameSession::Id>> session_connections_;
    // Ensures that exactly one connection per session is stored in session_connections_.
    // Called when the first player joins a session (including restored players).
    void ConnectToDogDeletedSignal(model::GameSession* session);
};

} // namespace app
