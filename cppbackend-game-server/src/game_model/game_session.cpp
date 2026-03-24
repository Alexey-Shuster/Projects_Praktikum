#include "game_session.h"

namespace model {

    Dog* GameSession::RequestDog(std::uint32_t dog_id, std::string_view dog_name, bool restored) {
        if (auto dog_it = dogs_.find(dog_id); dog_it != dogs_.end()) {
            if (restored) {
                return &dog_it->second;
            } else {
                std::string error_msg = "Duplicate Dog";
                boost_logger::LogError(EXIT_FAILURE, error_msg, "GameSession::RequestDog");
                throw std::runtime_error(error_msg);
            }
        } else {
            if (restored) {
                std::string error_msg = "Dog not found while restore process";
                boost_logger::LogError(EXIT_FAILURE, error_msg, "GameSession::RequestDog");
                throw std::runtime_error(error_msg);
            }
        }
        auto new_dog = Dog{dog_id, std::string(dog_name), map_};
        dogs_.emplace(dog_id, std::move(new_dog));
        return &dogs_.at(dog_id);
    }

    Dog * GameSession::FindDog(std::uint32_t dog_id) noexcept {
        if (auto dog_it = dogs_.find(dog_id); dog_it != dogs_.end()) {
            return &dog_it->second;
        }
        return nullptr;
    }

    const Dog * GameSession::FindDog(std::uint32_t dog_id) const noexcept {
        if (auto dog_it = dogs_.find(dog_id); dog_it != dogs_.end()) {
            return &dog_it->second;
        }
        return nullptr;
    }

    void GameSession::UpdateDogsIdleTime(std::chrono::milliseconds time_delta_ms) {
        std::vector<std::uint32_t> dogs_to_delete;
        for (auto& [id, dog] : dogs_) {
            if (dog.GetSpeed() == app_geom::Speed2D::Zero()) {
                dog.UpdateIdleTime(time_delta_ms);
                if (dog.GetIdleTime() >= game_extra_data_.get()->GetDogRetirementTime()) {
                    dogs_to_delete.emplace_back(id);
                }
            } else {
                dog.ResetIdleTime();
            }
        }
        // if any Dog to retire exists
        for (auto id : dogs_to_delete) {
            RetireDog(id);
        }
    }

    BotWorldState GameSession::BuildBotWorldState() const {
        std::vector<app_geom::Position2D> loot_positions;
        for (const auto& loot : loot_storage_.GetLootObjects() | std::views::values) {
            if (!loot.collected) {
                loot_positions.push_back(loot.pos);
            }
        }
        std::vector<app_geom::Position2D> office_positions;
        for (const auto& office : map_->GetOffices()) {
            office_positions.push_back(utils::Point2DToPosition2D(office.GetPosition()));
        }
        return {std::move(loot_positions), std::move(office_positions)};
    }

    void GameSession::RetireDog(std::uint32_t dog_id) {
        auto dog_it = dogs_.find(dog_id);
        if (dog_it == dogs_.end()) {
            std::string error_msg = "Dog not found while riterement process";
            boost_logger::LogError(EXIT_FAILURE, error_msg, "GameSession::RetireDog");
            throw std::runtime_error(error_msg);
        }
        ReleaseRetiredBag(&dog_it->second);

        on_dog_deleted_(dog_id, dog_it->second.GetScore());

        dogs_.erase(dog_id);
    }

    void GameSession::ReleaseRetiredBag(Dog* dog) {
        // return Loot to map (collected = false) with last Dog position
        for (auto& bag_item : dog->GetBag()) {
            bag_item->collected = false;
            bag_item->pos = dog->GetPosition();
        }
    }

    void GameSession::UpdateGameState(std::chrono::milliseconds time_delta_ms) {
        // 0. Check if any Dog retired - only if retirement enabled
        if (enable_retirement_) {
            UpdateDogsIdleTime(time_delta_ms);
        }

        // 1a. Record start positions and dog pointers
        std::vector<app_geom::Position2D> start_positions;
        std::vector<Dog*> dog_ptrs;
        for (auto &dog: dogs_ | std::views::values) {
            start_positions.push_back(dog.GetPosition());
            dog_ptrs.push_back(&dog);
        }
        // 1b. same for bots (if any exist)
        if (bot_manager_.GetBotCount() > 0) {
            for (auto bot : bot_manager_.GetAllBotPointers()) {
                start_positions.push_back(bot->GetPosition());
                dog_ptrs.push_back(bot);
            }
        }

        // 2a. Move all dogs & bots
        for (const auto dog : dog_ptrs) {
            dog->Move(time_delta_ms);
        }
        // 2b. Update bots direction (if any exist) with world state
        if (bot_manager_.GetBotCount() > 0) {
            bot_manager_.UpdateDirections(BuildBotWorldState(), time_delta_ms);
        }

        // 3. Generate new loot
        auto new_loot_count = loot_generator_->Generate
            (
                time_delta_ms, loot_storage_.GetLootObjects().size(),
                dogs_.size() + bot_manager_.GetBotCount()
            );
        if (new_loot_count > 0) {
            loot_storage_.GenerateLoots(
                new_loot_count,
                map_->GetRoads(),
                game_extra_data_->GetLootTypes(map_->GetId())
            );
        }

        // 4. Process collisions (loot and offices)
        ProcessCollisions(time_delta_ms, start_positions, dog_ptrs);
    }

    const std::map<int, const loot::LootObject *> GameSession::GetLootNotCollected() const {
        std::map<int, const loot::LootObject*> result;
        for (const auto& [id, loot] : loot_storage_.GetLootObjects()) {
            if (!loot.collected) {
                result.emplace(id, &loot);
            }
        }
        return result;
    }

    void GameSession::ProcessCollisions(std::chrono::milliseconds,
                                        const std::vector<app_geom::Position2D> &start_positions, const std::vector<Dog *> &dog_ptrs) {
        collision_detector::ItemGatherer provider;

        // Collect loot objects
        std::vector<const loot::LootObject*> loot_ptrs;
        for (const auto &loot: loot_storage_.GetLootObjects() | std::views::values) {
            // Skip already collected items (for safety)
            if (loot.collected) continue;
            loot_ptrs.push_back(&loot);
            provider.AddItem({
                loot.pos,
                common_values::COLLISION_WIDTH_OBJECT
            });
        }
        size_t loot_count = loot_ptrs.size();

        // Collect offices
        std::vector<const model::Office*> office_ptrs;
        for (const auto& office : map_->GetOffices()) {
            office_ptrs.push_back(&office);
            provider.AddItem({
                utils::Point2DToPosition2D(office.GetPosition()),
                common_values::COLLISION_WIDTH_OFFICE
            });
        }

        // Build gatherers (dogs)
        for (size_t i = 0; i < dog_ptrs.size(); ++i) {
            provider.AddGatherer({
                start_positions[i],
                dog_ptrs[i]->GetPosition(),
                common_values::COLLISION_WIDTH_PLAYER
            });
        }

        // Find events (already sorted by time)
        auto events = collision_detector::FindGatherEvents(provider);

        for (const auto& event : events) {
            size_t gatherer_idx = event.gatherer_id;
            size_t item_idx = event.item_id;
            Dog* dog = dog_ptrs[gatherer_idx];

            if (item_idx < loot_count) {            // It's a loot item
                auto loot = loot_ptrs[item_idx];
                // Skip if already collected in this tick
                if (loot->collected) continue;

                if (dog->GetBag().size() < map_->GetDefaultCapacity()) {
                    // Mark as collected and add to dog's bag
                    auto loot_ptr = FindLootByID(loot->object_id);
                    loot_ptr->collected = true;
                    dog->AddToBag(loot_ptr);        // store pointer to the loot object
                }
                // else bag full → skip (item remains available)
            } else {                                // It's an office
                // Deposit all bag items to score and remove from storage
                int total_score_value = 0;
                for (const auto* loot_obj : dog->GetBag()) {
                    total_score_value += loot_obj->loot_data_ptr->value;
                    // Remove the loot object from world storage
                    loot_storage_.RemoveLoot(loot_obj->object_id);
                }
                dog->AddScore(total_score_value);
                dog->ClearBag();
            }
        }
    }

} // namespace model