#pragma once

#include "game_session_repr.h"
#include "players_repr.h"

namespace serialize_game_save {

    class GameRepr {
    public:
        GameRepr() = default;

        explicit GameRepr(const model::Game& game, const app::Players& players) {
            for (const auto &session: game.GetSessions() | std::views::values) {
                sessions_reprs_.emplace_back(session);
            }
            players_repr_ = PlayersRepr(players);
        }

        void Restore(model::Game& game, app::Players& players, boost::asio::io_context& ioc) const {
            std::unordered_map<model::GameSession::Id, model::GameSession*, model::Game::GameSessionIdHasher> sessions_by_id;

            for (const auto& session_repr : sessions_reprs_) {
                session_repr.Restore(game, ioc, game.GetLootGenerator(),
                                     game.GetGameExtraData(), sessions_by_id);
            }

            players_repr_.Restore(players, sessions_by_id);
        }

    private:
        std::vector<GameSessionRepr> sessions_reprs_;
        PlayersRepr players_repr_;

        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned const /*version*/) {
            ar & sessions_reprs_;
            ar & players_repr_;
        }
    };

} // namespace serialize_game_save