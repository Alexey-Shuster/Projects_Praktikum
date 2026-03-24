#pragma once

#include <boost/serialization/access.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <unordered_map>

#include "../game_app/players.h"

namespace serialize_game_save {

    class PlayerRepr {
    public:
        PlayerRepr() = default;

        explicit PlayerRepr(const app::Player& player, std::string_view token)
            : player_id_(player.GetId())
            , name_(player.GetName())
            , session_id_(*player.GetGameSession()->GetId())
            , token_(token)
        {}

        void Restore(app::Players& players,
                     const std::unordered_map<model::GameSession::Id, model::GameSession*, model::Game::GameSessionIdHasher>& sessions_by_id) const {
            auto* session = sessions_by_id.at(model::GameSession::Id(session_id_));
            players.AddRestoredPlayer(player_id_, name_, session, token_);
        }

    private:
        uint32_t player_id_;
        std::string name_;
        uint32_t session_id_;
        std::string token_;

        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned const /*version*/) {
            ar & player_id_;
            ar & name_;
            ar & token_;
            ar & session_id_;
        }
    };

    class PlayersRepr {
    public:
        PlayersRepr() = default;

        explicit PlayersRepr(const app::Players& players) {
            next_player_id_ = players.GetNextPlayerId();
            for (const auto &player_ptr: players.GetAllPlayers() | std::views::values) {
                players_reprs_.emplace_back(*player_ptr, players.FindTokenByPlayer(*player_ptr)->operator*());
            }
        }

        void Restore(app::Players& players,
                     const std::unordered_map<model::GameSession::Id, model::GameSession*, model::Game::GameSessionIdHasher>& sessions_by_id) const {
            // next_player_id set first - otherwise later check in AddRestoredPlayer fails
            players.SetNextPlayerId(next_player_id_);
            for (const auto& player_repr : players_reprs_) {
                player_repr.Restore(players, sessions_by_id);
            }
        }

    private:
        uint32_t next_player_id_;
        std::vector<PlayerRepr> players_reprs_;

        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned const /*version*/) {
            ar & next_player_id_;
            ar & players_reprs_;
        }
    };

} // namespace serialize_game_save