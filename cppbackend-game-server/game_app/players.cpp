#include "players.h"
#include "../common/constants.h"

namespace app {

Player& Players::AddPlayer(std::string_view name, std::string_view map,
                                    std::chrono::milliseconds join_time, boost::asio::io_context &ioc){
    // find Map where Player wants to play
    auto map_tagg_id = model::Map::Id{std::string(map)};
    if (auto map_ptr = game_.FindMap(map_tagg_id); !map_ptr) {
        throw std::runtime_error("Players::AddPlayer: Unknown map.");
    }
    // get GameSession for selected Map
    auto session_pair = game_.RequestGameSession(map_tagg_id, ioc);
    auto session_ptr = game_.FindGameSession(session_pair.first);
    if (!session_ptr) {
        throw std::runtime_error("Players::AddPlayer: GameSession not found for selected map.");
    }
    if (session_pair.second /*new_created_session*/ && player_retire_handler_) {
        player_retire_handler_(session_ptr);
    }

    // create Player - Dog created automatically in Player Ctor
    auto player_id = next_player_id_++;
    auto new_player = std::make_unique<Player>(player_id, std::string(name), session_ptr, join_time);
    auto player_token = token_gen_();

    // Store Player with token
    token_to_player_.emplace(player_token, std::move(new_player));

    const auto player_it = token_to_player_.find(player_token);
    // update helper containers for quick lookup
    player_id_to_token_.emplace(player_id, &(player_it->first));
    player_id_to_player_.emplace(player_id, player_it->second.get());

    /// Ensure we are connected to this session's retirement signal.
    ConnectToDogDeletedSignal(session_ptr);

    return *player_it->second;
}

const Token* Players::FindTokenByPlayer(const Player &player) const {
    auto it = player_id_to_token_.find(player.GetId());
    if (it != player_id_to_token_.end()) {
        return it->second;
    }
    return nullptr;
}

const Player* Players::FindPlayerByToken(const Token &token) const {
    auto it = token_to_player_.find(token);
    if (it != token_to_player_.end()) {
        return it->second.get();
    }
    return nullptr;
}

Player* Players::FindPlayerByToken(const Token &token)
{
    auto it = token_to_player_.find(token);
    if (it != token_to_player_.end()) {
        return it->second.get();
    }
    return nullptr;
}

const Player * Players::FindPlayerById(std::uint32_t player_id) const {
    if (auto player_it = player_id_to_player_.find(player_id); player_it != player_id_to_player_.end()) {
        return player_it->second;
    }
    return nullptr;
}

const std::map<uint32_t, const Player*> Players::GetPlayersAll() const {
    std::map<std::uint32_t, const Player*> result;
    for (const auto& [id, token] : player_id_to_token_) {
        if (auto player = FindPlayerByToken(*token)) {
            result.emplace(id, player);
        }
    }
    return result;
}

const std::map<std::uint32_t, const Player *> Players::GetPlayersBySession(const model::GameSession::Id &session_id) const {
    std::map<std::uint32_t, const Player*> result;

    auto session_ptr = game_.FindGameSession(session_id);
    // players
    for (const auto &dog_id: session_ptr->GetDogs() | std::views::keys) {
        auto player = FindPlayerById(dog_id);
        result.emplace(player->GetId(), player);
    }
    // bots
    for (const auto &dog_id: session_ptr->GetBots() | std::views::keys) {
        auto player = FindPlayerById(dog_id);
        result.emplace(player->GetId(), player);
    }
    return result;
}

void Players::AddRestoredPlayer(uint32_t id, std::string_view name,
                                    model::GameSession* session,
                                    std::string_view token) {
    // Validate session
    if (!session) {
        std::string error_msg = "GameSession is null";
        boost_logger::LogError(EXIT_FAILURE, error_msg, "Players::AddRestoredPlayer");
        throw std::runtime_error(error_msg);
    }

    // Check for duplicate player ID
    if (player_id_to_player_.contains(id)) {
        std::string error_msg = "Duplicate player ID: " + std::to_string(id);
        boost_logger::LogError(EXIT_FAILURE, error_msg, "Players::AddRestoredPlayer");
        throw std::runtime_error(error_msg);
    }

    // Check for duplicate token
    Token tkn(std::string{token});
    if (token_to_player_.contains(tkn)) {
        std::string error_msg = "Duplicate token: " + *tkn;
        boost_logger::LogError(EXIT_FAILURE, error_msg, "Players::AddRestoredPlayer");
        throw std::runtime_error(error_msg);
    }

    // Create the Player object.
    // Player Ctor creates Dog - Dog verification done in GameSession::RequestDog
    // Player restored just after Application created with game_time=0 - Player join_time also=0
    bool restored_player = true;
    auto player = std::make_unique<Player>(id, std::string(name), session,
            std::chrono::milliseconds::zero(),restored_player);
    const Player* player_ptr = player.get();

    // Insert player into token→player map (token is copied as key)
    auto [it, inserted] = token_to_player_.emplace(tkn, std::move(player));
    if (!inserted) {
        // Should not happen - already checked for duplicate token,
        // but safeguard anyway.
        throw std::runtime_error("AddRestoredPlayer: failed to insert token");
    }

    // Store a pointer to the token that now lives in the map's key.
    player_id_to_token_[id] = &(it->first);

    // Store player pointer by ID.
    player_id_to_player_[id] = it->second.get();

    // Without this, a restored player would never trigger the retirement signal.
    ConnectToDogDeletedSignal(session);

    // Update the next free player ID if necessary to avoid future collisions.
    if (id >= next_player_id_) {
        // Should not happen - next_player_id set first while Restore,
        // but safeguard anyway.
        throw std::runtime_error("AddRestoredPlayer: next_player_id violation");
    }
}

void Players::ConnectToDogDeletedSignal(model::GameSession *session) {
    // If we already have a connection for this session, do nothing (one connection per session)
    if (session_connections_.contains(session->GetId())) {
        return;
    }

    // Connect to the session's dog-deleted signal. This signal is emitted when a dog
    // is retired due to idle timeout (or possibly other reasons in the future).
    auto connection = session->GetDogDeletedSignal().connect(
        [this, session](uint32_t dog_id, loot::Score dog_score) {
            // Ignore bots (they have IDs above a threshold and are managed separately)
            if (dog_id >= common_values::DOG_BOT_START_ID)
                return;

            // Find the player associated with this dog ID
            auto it = player_id_to_player_.find(dog_id);
            if (it == player_id_to_player_.end()) {
                // Should never happen for a real player, but safeguard
                throw std::runtime_error("Players::ConnectToSessionRetirement: Player not found.");
            }

            const Player* player = it->second;

            // Emit the public player-retired signal. This allows external components
            // (like score recorder) to react before the player is removed.
            // The player object is still valid at this point.
            player_retired_signal_(dog_id, dog_score, *player);

            // Finally, remove the player from all internal containers.
            // This invalidates the player pointer, so it must not be used after this point.
            RemoveRetiredPlayer(dog_id);
        }
    );

    // Store the connection so it stays alive for the lifetime of the session.
    session_connections_.emplace(session->GetId(), std::move(connection));
}

const std::map<uint32_t, const Player*> Players::GetPlayersByMap(const model::Map::Id &map_id) const {
    std::map<std::uint32_t, const Player*> result;

    // find all players on map
    for (const auto &player: token_to_player_ | std::views::values) {
        if (player->GetGameSession()->GetMap()->GetId() == map_id) {
            result.emplace(player->GetId(), player.get());
        }
    }

    return result;
}

void Player::SetDirection(app_geom::Direction2D dir) {
    if (dog_) {
        // Forward direction to the actual game character
        dog_->SetDirection(dir);
    } else {
        throw std::runtime_error("Player::SetDirection: Dog not assigned to player.");
    }
}

} // namespace app
