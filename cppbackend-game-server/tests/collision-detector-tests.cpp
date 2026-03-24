#define _USE_MATH_DEFINES

#include "../src/common/collision_detector.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <sstream>
#include <vector>

#include "../src/common/constants.h"

namespace Catch {

// Custom string maker for Catch2 to print GatheringEvent
template <>
struct StringMaker<collision_detector::GatheringEvent> {
    static std::string convert(collision_detector::GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id << ","
            << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};

}  // namespace Catch

// Tolerance for floating point comparisons (10⁻¹⁰)
constexpr static double EPS = common_values::DOUBLE_ABS_TOLERANCE;



TEST_CASE("No items or gatherers") {
    collision_detector::ItemGatherer provider;
    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty());
}

TEST_CASE("No gatherers") {
    collision_detector::ItemGatherer provider;
    provider.AddItem({{0, 0}, 1.0});
    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty());
}

TEST_CASE("No items") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty());
}

TEST_CASE("Gatherer moves directly to item, within radius") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{5, 0}, 1.0});

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Catch::Approx(0.0).margin(EPS));
    CHECK(events[0].time == Catch::Approx(0.5).margin(EPS));
}

TEST_CASE("Item off to side but within combined width") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{5, 1.5}, 1.0}); // distance = 1.5 ≤ 2

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].sq_distance == Catch::Approx(2.25).margin(EPS));
    CHECK(events[0].time == Catch::Approx(0.5).margin(EPS));
}

TEST_CASE("Item too far away") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{5, 3.0}, 1.0}); // distance = 3 > 2

    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty());
}

TEST_CASE("Item before start") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{-1, 0}, 1.0}); // proj_ratio < 0

    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty());
}

TEST_CASE("Item after end") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{11, 0}, 1.0}); // proj_ratio > 1

    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty());
}

TEST_CASE("Multiple items, some collected") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{2, 0}, 1.0});      // collected, time 0.2, sq 0
    provider.AddItem({{5, 1.5}, 1.0});    // collected, time 0.5, sq 2.25
    provider.AddItem({{8, -2.1}, 1.0});   // not collected (distance 2.1 > 2)
    provider.AddItem({{10, 0}, 1.0});     // collected, time 1.0, sq 0

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 3);

    std::vector<collision_detector::GatheringEvent> expected;
    expected.push_back({0, 0, 0.0, 0.2});
    expected.push_back({1, 0, 2.25, 0.5});
    expected.push_back({3, 0, 0.0, 1.0});

    SortEvents(events);
    SortEvents(expected);

    for (size_t i = 0; i < events.size(); ++i) {
        CHECK(events[i].gatherer_id == expected[i].gatherer_id);
        CHECK(events[i].item_id == expected[i].item_id);
        CHECK(events[i].sq_distance == Catch::Approx(expected[i].sq_distance).margin(EPS));
        CHECK(events[i].time == Catch::Approx(expected[i].time).margin(EPS));
    }
}

TEST_CASE("Multiple gatherers and items") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0}); // gatherer 0
    provider.AddGatherer({{0, 5}, {10, 5}, 1.0}); // gatherer 1
    provider.AddItem({{5, 0}, 1.0});   // item 0, collected by gatherer 0
    provider.AddItem({{5, 5}, 1.0});   // item 1, collected by gatherer 1
    provider.AddItem({{5, 2.5}, 1.0}); // item 2, too far from both (distance 2.5 > 2)
    provider.AddItem({{2, 5}, 1.0});   // item 3, collected by gatherer 1

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 3);

    std::vector<collision_detector::GatheringEvent> expected;
    expected.push_back({3, 1, 0.0, 0.2}); // item 3 with gatherer 1
    expected.push_back({0, 0, 0.0, 0.5}); // item 0 with gatherer 0
    expected.push_back({1, 1, 0.0, 0.5}); // item 1 with gatherer 1

    SortEvents(events);
    SortEvents(expected);

    for (size_t i = 0; i < events.size(); ++i) {
        CHECK(events[i].gatherer_id == expected[i].gatherer_id);
        CHECK(events[i].item_id == expected[i].item_id);
        CHECK(events[i].sq_distance == Catch::Approx(expected[i].sq_distance).margin(EPS));
        CHECK(events[i].time == Catch::Approx(expected[i].time).margin(EPS));
    }
}

TEST_CASE("Gatherer with zero movement") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {0, 0}, 1.0});
    provider.AddItem({{0, 0}, 1.0});

    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty()); // no movement → no collisions
}

TEST_CASE("Item exactly at boundary distance") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{5, 2.0}, 1.0}); // distance = 2 (equal to combined width)

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].sq_distance == Catch::Approx(4.0).margin(EPS));
    CHECK(events[0].time == Catch::Approx(0.5).margin(EPS));
}

TEST_CASE("Item just outside boundary") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{5, 2.000000001}, 1.0}); // slightly > 2

    auto events = collision_detector::FindGatherEvents(provider);
    CHECK(events.empty());
}

TEST_CASE("Events are in chronological order") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    // items at x = 1, 3, 5, 7, 9 on the line
    for (int x = 1; x <= 9; x += 2) {
        provider.AddItem({{double(x), 0}, 1.0});
    }

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 5);

    for (size_t i = 0; i < events.size(); ++i) {
        double expected_time = (2 * i + 1) / 10.0; // 0.1, 0.3, 0.5, 0.7, 0.9
        CHECK(events[i].time == Catch::Approx(expected_time).margin(EPS));
    }
    // additionally verify non-decreasing order
    for (size_t i = 1; i < events.size(); ++i) {
        CHECK(events[i - 1].time <= events[i].time);
    }
}

TEST_CASE("sq_distance is computed correctly") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{3, 1.5}, 1.0}); // distance 1.5 → sq = 2.25

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].sq_distance == Catch::Approx(2.25).margin(EPS));
}

TEST_CASE("Multiple gatherers can collect the same item") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0}); // along x-axis
    provider.AddGatherer({{0, 0}, {0, 10}, 1.0}); // along y-axis
    provider.AddItem({{2, 2}, 1.0}); // both can collect (distance 2, combined width 2)

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 2);

    // sort by gatherer_id to compare
    std::sort(events.begin(), events.end(),
              [](const auto& a, const auto& b) { return a.gatherer_id < b.gatherer_id; });

    CHECK(events[0].gatherer_id == 0);
    CHECK(events[0].item_id == 0);
    CHECK(events[0].sq_distance == Catch::Approx(4.0).margin(EPS));
    CHECK(events[0].time == Catch::Approx(0.2).margin(EPS));

    CHECK(events[1].gatherer_id == 1);
    CHECK(events[1].item_id == 0);
    CHECK(events[1].sq_distance == Catch::Approx(4.0).margin(EPS));
    CHECK(events[1].time == Catch::Approx(0.2).margin(EPS));
}

// ----- Additional tests for diagonal movement and endpoints -----
TEST_CASE("Diagonal movement collects item on path") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 10}, 1.0});
    provider.AddItem({{5, 5}, 1.0}); // exactly on line

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].time == Catch::Approx(0.5).margin(EPS));
    CHECK(events[0].sq_distance == Catch::Approx(0.0).margin(EPS));
}

TEST_CASE("Item exactly at start point") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{0, 0}, 1.0}); // at start

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].time == Catch::Approx(0.0).margin(EPS));
    CHECK(events[0].sq_distance == Catch::Approx(0.0).margin(EPS));
}

TEST_CASE("Item exactly at end point") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});
    provider.AddItem({{10, 0}, 1.0}); // at end

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].time == Catch::Approx(1.0).margin(EPS));
    CHECK(events[0].sq_distance == Catch::Approx(0.0).margin(EPS));
}

TEST_CASE("Item at diagonal endpoint") {
    collision_detector::ItemGatherer provider;
    provider.AddGatherer({{0, 0}, {10, 10}, 1.0});
    provider.AddItem({{10, 10}, 1.0}); // at end

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 1);
    CHECK(events[0].time == Catch::Approx(1.0).margin(EPS));
    CHECK(events[0].sq_distance == Catch::Approx(0.0).margin(EPS));
}

TEST_CASE("Simultaneous events with different gatherers and items") {
    collision_detector::ItemGatherer provider;
    // Two gatherers crossing at same time
    provider.AddGatherer({{0, 0}, {10, 0}, 1.0});   // gatherer 0 moves right
    provider.AddGatherer({{5, -5}, {5, 5}, 1.0});   // gatherer 1 moves up through x=5
    provider.AddItem({{5, 0}, 1.0});   // item at (5,0) – both can collect at time 0.5?
    // For gatherer0, projection ratio = 0.5, distance 0; for gatherer1, projection ratio = 0.5 as well (from start (5,-5) to (5,5) so y=0 is midpoint)
    // Both at time 0.5

    auto events = collision_detector::FindGatherEvents(provider);
    REQUIRE(events.size() == 2);
    // Both events have time 0.5
    CHECK(events[0].time == Catch::Approx(0.5).margin(EPS));
    CHECK(events[1].time == Catch::Approx(0.5).margin(EPS));
    // They may be in any order, but both present
    bool found00 = false, found10 = false;
    for (const auto& e : events) {
        if (e.gatherer_id == 0 && e.item_id == 0) found00 = true;
        if (e.gatherer_id == 1 && e.item_id == 0) found10 = true;
    }
    CHECK(found00);
    CHECK(found10);
}
