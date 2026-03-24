#pragma once

#include "repository_impls.h"

namespace db {

    class UnitOfWork {
    public:
        virtual db::PlayerScoreRepository& PlayerScores() = 0;
        virtual void Commit() = 0;
        virtual ~UnitOfWork() = default;
    };

    class UnitOfWorkRemote : public UnitOfWork {
    public:
        explicit UnitOfWorkRemote(pqxx::connection& conn)
            : work_(conn)
            , playerScoreRepo_(work_)
        {}

        db::PlayerScoreRepository& PlayerScores() override {
            return playerScoreRepo_;
        }

        void Commit() override {
            work_.commit();
        }

    private:
        pqxx::work work_;
        PlayerScoreRepositoryRemote playerScoreRepo_;
    };

}  // namespace db