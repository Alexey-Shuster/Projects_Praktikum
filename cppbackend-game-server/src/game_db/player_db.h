#pragma once

#include <string>
#include "tagged_uuid.h"
#include <vector>

namespace db {
    namespace detail {
        struct PlayerTag {};
    }  // namespace detail

    using PlayerId = util::TaggedUUID<detail::PlayerTag>;

    struct PlayerScore {
        PlayerId id;
        std::string name;
        int score{0};
        double play_time_sec{0.0};
    };

    class PlayerScoreRepository {
    public:
        virtual void Save(const PlayerScore& score) = 0;
        virtual std::vector<PlayerScore> GetSorted(int limit, int offset) = 0;
        virtual ~PlayerScoreRepository() = default;
    };

} // namespace db
