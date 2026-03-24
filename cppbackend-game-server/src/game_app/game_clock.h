#pragma once

#include <chrono>

namespace app {

    class GameClock {
    public:
        [[nodiscard]] std::chrono::milliseconds Now() const { return current_; }
        void Advance(std::chrono::milliseconds delta) { current_ += delta; }
    private:
        std::chrono::milliseconds current_{0};
    };

} // namespace app