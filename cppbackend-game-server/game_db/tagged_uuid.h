#pragma once

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <string>

#include "../common/tagged.h"

namespace util {

    namespace detail {

        using UUIDType = boost::uuids::uuid;

        inline UUIDType NewUUID() {
            return boost::uuids::random_generator()();
        };

        inline std::string UUIDToString(const UUIDType& uuid) {
            return to_string(uuid);
        };
        inline UUIDType UUIDFromString(std::string_view str) {
            boost::uuids::string_generator gen;
            return gen(str.begin(), str.end());
        };

    }  // namespace detail

    template <typename Tag>
    class TaggedUUID : public Tagged<detail::UUIDType, Tag> {
    public:
        using Base = Tagged<detail::UUIDType, Tag>;
        using Tagged<detail::UUIDType, Tag>::Tagged;

        TaggedUUID()
            : Base{boost::uuids::nil_uuid()} {
        }

        static TaggedUUID New() {
            return TaggedUUID{detail::NewUUID()};
        }

        static TaggedUUID FromString(const std::string& uuid_as_text) {
            return TaggedUUID{detail::UUIDFromString(uuid_as_text)};
        }


        [[nodiscard]] std::string ToString() const {
            return detail::UUIDToString(**this);
        }
    };

}  // namespace util
