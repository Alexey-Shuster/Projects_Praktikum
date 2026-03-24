#include <catch2/catch_test_macros.hpp>
#include "test_database.h"

using namespace db;
using namespace db_test;

SCENARIO("TestPlayerScoreRepository saves and retrieves scores") {
    GIVEN("an empty repository") {
        TestPlayerScoreRepository repo;

        WHEN("a single score is saved") {
            PlayerId id = PlayerId::New();
            PlayerScore score{id, "Alice", 100, 12.5};
            repo.Save(score);

            THEN("it can be retrieved with GetSorted") {
                auto results = repo.GetSorted(10, 0);
                REQUIRE(results.size() == 1);
                CHECK(results[0].id == id);
                CHECK(results[0].name == "Alice");
                CHECK(results[0].score == 100);
                CHECK(results[0].play_time_sec == 12.5);
            }
        }

        WHEN("multiple scores are saved") {
            PlayerId id1 = PlayerId::New();
            PlayerId id2 = PlayerId::New();
            PlayerId id3 = PlayerId::New();
            repo.Save({id1, "Charlie", 50, 10.0});
            repo.Save({id2, "Bob",    80, 5.0});
            repo.Save({id3, "Alice",  80, 8.0});

            THEN("GetSorted returns them in the correct order") {
                auto results = repo.GetSorted(10, 0);
                REQUIRE(results.size() == 3);
                // Highest score first: Bob (80,5.0) then Alice (80,8.0) then Charlie (50,10.0)
                CHECK(results[0].name == "Bob");
                CHECK(results[1].name == "Alice");
                CHECK(results[2].name == "Charlie");
                // IDs should match the corresponding players
                CHECK(results[0].id == id2);
                CHECK(results[1].id == id3);
                CHECK(results[2].id == id1);
            }
        }

        WHEN("saving a score with an existing ID") {
            PlayerId id = PlayerId::New();
            repo.Save({id, "First", 10, 1.0});
            repo.Save({id, "Second", 20, 2.0}); // same ID, should replace

            THEN("only the latest version remains") {
                auto results = repo.GetSorted(10, 0);
                REQUIRE(results.size() == 1);
                CHECK(results[0].name == "Second");
                CHECK(results[0].score == 20);
            }
        }
    }
}

SCENARIO("GetSorted pagination") {
    GIVEN("a repository with 10 scores") {
        TestPlayerScoreRepository repo;
        std::vector<PlayerId> ids;
        for (int i = 0; i < 10; ++i) {
            ids.push_back(PlayerId::New());
            repo.Save({ids.back(),
                       "Player" + std::to_string(i),
                       100 - i,          // descending scores
                       static_cast<double>(i)});
        }

        WHEN("requesting limit=5, offset=0") {
            auto results = repo.GetSorted(5, 0);
            THEN("the first 5 are returned") {
                REQUIRE(results.size() == 5);
                CHECK(results[0].score == 100);
                CHECK(results[4].score == 96);
            }
        }

        WHEN("requesting limit=5, offset=8") {
            auto results = repo.GetSorted(5, 8);
            THEN("only the last 2 are returned") {
                REQUIRE(results.size() == 2);
                CHECK(results[0].score == 92);
                CHECK(results[1].score == 91);
            }
        }

        WHEN("offset is beyond the end") {
            auto results = repo.GetSorted(10, 20);
            THEN("an empty list is returned") {
                REQUIRE(results.empty());
            }
        }
    }
}

SCENARIO("TestUnitOfWork works with the repository") {
    GIVEN("a TestUnitOfWork") {
        TestUnitOfWork uow;
        auto& repo = uow.PlayerScores();

        WHEN("saving a score and committing") {
            PlayerId id = PlayerId::New();
            repo.Save({id, "UOW", 99, 9.9});
            uow.Commit();

            THEN("the data is still accessible") {
                auto results = repo.GetSorted(10, 0);
                REQUIRE(results.size() == 1);
                CHECK(results[0].name == "UOW");
            }
        }
    }
}

SCENARIO("TestDatabase creates independent units of work") {
    GIVEN("a TestDatabase") {
        TestDatabase db;

        WHEN("two units of work are created") {
            auto uow1 = db.CreateUnitOfWork();
            auto uow2 = db.CreateUnitOfWork();

            THEN("they operate on separate repository instances") {
                PlayerId id1 = PlayerId::New();
                PlayerId id2 = PlayerId::New();
                uow1->PlayerScores().Save({id1, "First", 10, 1.0});
                uow2->PlayerScores().Save({id2, "Second", 20, 2.0});

                auto res1 = uow1->PlayerScores().GetSorted(10, 0);
                auto res2 = uow2->PlayerScores().GetSorted(10, 0);

                CHECK(res1.size() == 1);
                CHECK(res1[0].name == "First");
                CHECK(res2.size() == 1);
                CHECK(res2[0].name == "Second");
            }
        }
    }
}