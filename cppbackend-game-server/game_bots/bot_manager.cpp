#include "bot_manager.h"

#include <ranges>

#include "../common/constants.h"

namespace model {

BotManager::BotManager(const Map* map)
    : map_(map)
    , rng_(std::random_device{}())
    , road_graph_(map->GetRoads(), map->GetRoadEngine())   // build graph once
{}

void BotManager::CreateBots() {
    if (!bots_.empty()) return;

    size_t bot_count = common_values::MAX_PLAYERS_ON_MAP / 2;
    for (size_t i = 0; i < bot_count; ++i) {
        uint32_t bot_id = next_bot_id_++;
        std::string bot_name = "Bot_" + std::to_string(bot_id);

        Dog bot(bot_id, std::move(bot_name), map_);
        bot.SetPosition(bot.GetMap()->GetRandomPoint());

        bots_.emplace(bot_id, std::move(bot));      // save bot
        bot_ais_.emplace(bot_id, BotAI{});          // create AI for this bot
    }
    UpdateDirections();  // give bots an initial direction & initialize last_direction_
}

void BotManager::MoveBots(std::chrono::milliseconds time_delta) {
    for (auto &bot: bots_ | std::views::values) {
        bot.Move(time_delta);
    }
}

void BotManager::UpdateDirections() {
    for (auto &bot: bots_ | std::views::values) {
        // If the bot is standing still, force a new direction immediately.
        if (bot.GetSpeed() == app_geom::Speed2D::Zero()) {
            bot.SetDirection(all_dirs_[dir_dist_(rng_)]);
            continue;
        }

        // Otherwise, possibly change direction with a given probability.
        if (change_prob_dist_(rng_) < common_values::BOT_DIR_CHANGE_PROBABILITY) {
            auto roads = map_->GetRoadEngine().FindRoadsAtPosition(bot.GetPosition());
            if (roads.empty()) continue;

            std::vector<app_geom::Direction2D> allowed;
            if (roads.size() == 1) {
                if (roads.front()->IsHorizontal()) {
                    allowed = {app_geom::Direction2D::LEFT, app_geom::Direction2D::RIGHT};
                } else {
                    allowed = {app_geom::Direction2D::UP, app_geom::Direction2D::DOWN};
                }
            } else {
                allowed = all_dirs_;   // intersection → any direction allowed
            }

            if (!allowed.empty()) {
                std::uniform_int_distribution<size_t> dist(0, allowed.size() - 1);
                bot.SetDirection(allowed[dist(rng_)]);
            } else {
                bot.SetDirection(app_geom::Direction2D::UP);   // fallback
            }
        }
    }
}

void BotManager::UpdateDirections(const BotWorldState& world_state,
                                      std::chrono::milliseconds time_delta) {
    for (auto& [id, bot] : bots_) {
        auto ai_it = bot_ais_.find(id);
        if (ai_it == bot_ais_.end()) continue;

        auto opt_dir = ai_it->second.UpdateDirection(
            bot, *map_, road_graph_, world_state, rng_, time_delta);

        if (opt_dir) {
            bot.SetDirection(*opt_dir);
        } else {
            bot.SetSpeed(app_geom::Speed2D::Zero());
        }
    }
}

std::vector<Dog*> BotManager::GetAllBotPointers() {
    std::vector<Dog*> result;
    result.reserve(bots_.size());
    for (auto &bot: bots_ | std::views::values) {
        result.push_back(&bot);
    }
    return result;
}

} // namespace model