#pragma once

#include "json.h"
#include <memory>
#include <stack>

namespace compile_check {
    class BuilderValid;
    class ValidStartDict;
    class ValidDictKey;
    class ValidStartArray;
}

namespace json {

enum ValueType {
    NULLPTR = 0,
    INTEGER, DOUBLE, STRING, BOOL, ARRAY, DICTIONARY
};

class Builder final {
public:
    Builder() = default;

    compile_check::BuilderValid Value(json::NodeVariantType input);

    compile_check::ValidStartDict StartDict();
    compile_check::ValidDictKey Key(std::string key);
    compile_check::BuilderValid EndDict();

    compile_check::ValidStartArray StartArray();
    compile_check::BuilderValid EndArray();

    json::Node Build();

private:
    json::NodeVariantType root_;
    using StructureValue = std::variant<json::Array, json::Dict>;
    std::stack<std::unique_ptr<StructureValue>> opened_structures_;

    std::stack<std::string> dict_keys_;

    ValueType root_value_type_;
    bool root_started_ = false;
    bool single_value_input_ = false;
    bool dict_key_expected_ = false;

    void RegisterRootStart();
    ValueType CheckInput(json::NodeVariantType& input);
    void CheckSingleValue();
    void CheckKeyInput();
    json::Node CreateNodeFromValue(json::NodeVariantType& input);

    void AddElementDictionary(json::NodeVariantType input);
    void ProcessEndStructure(json::Node node);
    void ProcessNestedStructure(json::Node& node);
    void ProcessRootStructure(json::Node& node);

    template<typename T>
    bool CheckTopStructureValue() const {
        if (!opened_structures_.empty()) {
            return std::holds_alternative<T>(*opened_structures_.top());
        }
        throw std::logic_error("CheckStructureValue -> opened structures conrainer is EMPTY");
    }
};

} // namespace json

namespace compile_check {

class ValidStartDict;
class ValidDictKey;
class ValidDictValue;
class ValidStartArray;

class BuilderValid {
public:
    BuilderValid(json::Builder& builder) : builder_(builder) {};

    BuilderValid Value(json::NodeVariantType input);

    ValidStartDict StartDict();
    ValidDictKey Key(std::string key);
    BuilderValid EndDict();

    ValidStartArray StartArray();
    BuilderValid EndArray();

    json::Node Build();

private:
    friend class ValidDictKey;
    friend class ValidStartArray;
    json::Builder& builder_;
};

class ValidStartDict : public BuilderValid {
public:
    ValidStartDict(json::Builder& builder) : BuilderValid(builder) {};

    ValidDictKey Key(std::string key);
    BuilderValid EndDict();

    BuilderValid Value(json::NodeVariantType input) = delete;
    ValidStartDict StartDict() = delete;
    ValidStartArray StartArray() = delete;
    BuilderValid EndArray() = delete;
    json::Node Build() = delete;
};

class ValidDictKey : public BuilderValid {
public:
    ValidDictKey(json::Builder& builder) : BuilderValid(builder) {};

    ValidStartDict Value(json::NodeVariantType input);
    ValidStartDict StartDict();
    ValidStartArray StartArray();

    ValidDictKey Key(std::string key) = delete;
    BuilderValid EndDict() = delete;
    BuilderValid EndArray() = delete;
    json::Node Build() = delete;
};

class ValidStartArray : public BuilderValid {
public:
    ValidStartArray(json::Builder& builder) : BuilderValid(builder) {};

    ValidStartArray Value(json::NodeVariantType input);
    ValidStartDict StartDict();
    ValidStartArray StartArray();
    BuilderValid EndArray();

    ValidDictKey Key(std::string key) = delete;
    BuilderValid EndDict() = delete;
    json::Node Build() = delete;
};

} //namespace compile_check

