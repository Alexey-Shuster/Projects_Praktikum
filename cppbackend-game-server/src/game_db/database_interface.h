#pragma once

#include "unit_of_work.h"

namespace db {

    class DatabaseInterface {
    public:
        virtual ~DatabaseInterface() = default;
        virtual std::unique_ptr<UnitOfWork> CreateUnitOfWork() = 0;
    };

} // namespace db