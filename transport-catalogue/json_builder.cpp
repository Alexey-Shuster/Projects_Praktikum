#include "json_builder.h"

#include <cassert>

namespace json {

compile_check::BuilderValid Builder::Value(json::NodeVariantType input) {
    CheckSingleValue();
    CheckKeyInput();
    if (!root_started_) {
        root_value_type_ = CheckInput(input);
        RegisterRootStart();
        root_ = input;
        single_value_input_ = true;
        return *this;
    }
    if (!opened_structures_.empty()) {
        if (std::holds_alternative<json::Array>(*opened_structures_.top())) {
            std::get<json::Array>(*opened_structures_.top()).emplace_back(CreateNodeFromValue(input));
        }
        if (std::holds_alternative<json::Dict>(*opened_structures_.top())) {
            AddElementDictionary(input);
        }
    } else {
        throw std::logic_error("Value command NOT possible in this context");
    }

    return *this;
}

compile_check::ValidStartDict Builder::StartDict() {
    CheckSingleValue();
    CheckKeyInput();
    if (!root_started_) {
        root_value_type_ = ValueType::DICTIONARY;
        RegisterRootStart();
    }
    opened_structures_.emplace(std::make_unique<StructureValue>(json::Dict{}));
    dict_key_expected_ = true;
    return *this;
}

compile_check::ValidDictKey Builder::Key(std::string key) {
    CheckSingleValue();
    if (dict_key_expected_) {
        dict_key_expected_ = false;
        dict_keys_.emplace(key);
    } else {
        throw std::logic_error("Key command ONLY AFTER StartDict");
    }
    return *this;
}

compile_check::BuilderValid Builder::EndDict() {
    CheckSingleValue();
    if (!opened_structures_.empty() && CheckTopStructureValue<json::Dict>()) {
        ProcessEndStructure(json::Node(std::get<json::Dict>(*opened_structures_.top())));
        if ((!opened_structures_.empty() && !CheckTopStructureValue<json::Dict>()) || (opened_structures_.empty() && root_value_type_ == ValueType::DICTIONARY)) {
            dict_key_expected_ = false;     // check if Next structure is NOT Dict or no structures left and Root is Dict
        }
        return *this;
    }
    throw std::logic_error("EndDict command NOT possible in this context");
}

compile_check::ValidStartArray Builder::StartArray() {
    CheckSingleValue();
    CheckKeyInput();
    if (!root_started_) {
        root_value_type_ = ValueType::ARRAY;
        RegisterRootStart();
    }
    opened_structures_.emplace(std::make_unique<StructureValue>(json::Array{}));
    return *this;
}

compile_check::BuilderValid Builder::EndArray() {
    CheckSingleValue();
    CheckKeyInput();
    if (!opened_structures_.empty() && CheckTopStructureValue<json::Array>()) {
        ProcessEndStructure(json::Node(std::get<json::Array>(*opened_structures_.top())));
        return *this;
    }
    throw std::logic_error("EndArray command NOT possible in this context");
}

json::Node Builder::Build() {
    CheckKeyInput();
    if (!root_started_) {
        throw std::logic_error("Build NOT possible without input");
    }
    return CreateNodeFromValue(root_);
}

void Builder::RegisterRootStart() {
    root_started_ = true;
}

ValueType Builder::CheckInput(json::NodeVariantType& input)
{
    ValueType result;
    if (std::holds_alternative<std::nullptr_t>(input)) {
        result = ValueType::NULLPTR;
    } else if (std::holds_alternative<int>(input)) {
        result = ValueType::INTEGER;
    } else if (std::holds_alternative<double>(input)) {
        result = ValueType::DOUBLE;
    } else if (std::holds_alternative<std::string>(input)) {
        result = ValueType::STRING;
    } else if (std::holds_alternative<bool>(input)) {
        result = ValueType::BOOL;
    } else if (std::holds_alternative<json::Array>(input)) {
        result = ValueType::ARRAY;
    } else if (std::holds_alternative<json::Dict>(input)) {
        result = ValueType::DICTIONARY;
    } else {
        throw std::logic_error("Wrong input");
    }
    return result;
}

void Builder::CheckSingleValue()
{
    if (single_value_input_) {
        throw std::logic_error("Single Value input -> ONLY Build possible");
    }
}

void Builder::CheckKeyInput()
{
    if (dict_key_expected_) {
        throw std::logic_error("Key command EXPECTED)");
    }
}

json::Node Builder::CreateNodeFromValue(json::NodeVariantType& value)
{
    switch (CheckInput(value)) {
    case ValueType::NULLPTR:
        return json::Node(std::get<std::nullptr_t>(value));
        break;
    case ValueType::INTEGER:
        return json::Node(std::get<int>(value));
        break;
    case ValueType::DOUBLE:
        return json::Node(std::get<double>(value));
        break;
    case ValueType::STRING:
        return json::Node(std::get<std::string>(value));
        break;
    case ValueType::BOOL:
        return json::Node(std::get<bool>(value));
        break;
    case ValueType::ARRAY:
        return json::Node(std::get<json::Array>(value));
        break;
    case ValueType::DICTIONARY:
        return json::Node(std::get<json::Dict>(value));
        break;
    default:
        throw std::logic_error("Cannot Build Node");
        break;
    }
}

void Builder::AddElementDictionary(json::NodeVariantType input)
{
    std::get<json::Dict>(*opened_structures_.top()).emplace(dict_keys_.top(), CreateNodeFromValue(input));
    dict_keys_.pop();
    dict_key_expected_ = true;
    // std::cout << "AddElementDictionary:Key[" << std::get<json::Dict>(*opened_structures_.top()).rbegin()->first
    //           << "]#Value[" << std::get<json::Dict>(*opened_structures_.top()).rbegin()->second << "]\n";
}

void Builder::ProcessEndStructure(json::Node node)
{
    opened_structures_.pop();
    if (opened_structures_.size() >= 1) {
        ProcessNestedStructure(node);
    } else {
        ProcessRootStructure(node);
    }
}

void Builder::ProcessNestedStructure(json::Node& node) {
    if (CheckTopStructureValue<json::Array>()) {
        std::get<json::Array>(*opened_structures_.top()).emplace_back(node);
    }
    if (CheckTopStructureValue<json::Dict>()) {
        AddElementDictionary(node.GetValue());
    }
}

void Builder::ProcessRootStructure(json::Node& node) {
    if (root_value_type_ == ValueType::ARRAY) {
        root_ = node.AsArray();
    }
    if (root_value_type_ == ValueType::DICTIONARY) {
        root_ = node.AsMap();
    }
}

} // namespace json

namespace compile_check {

BuilderValid BuilderValid::Value(json::NodeVariantType input) {
    return builder_.Value(std::move(input));
}

ValidStartDict BuilderValid::StartDict() {
    return builder_.StartDict();
}

ValidDictKey BuilderValid::Key(std::string key) {
    return builder_.Key(std::move(key));
}

BuilderValid BuilderValid::EndDict() {
    return builder_.EndDict();
}

ValidStartArray BuilderValid::StartArray() {
    return builder_.StartArray();
}

BuilderValid BuilderValid::EndArray() {
    return builder_.EndArray();
}

json::Node BuilderValid::Build() {
    return builder_.Build();
}

ValidStartDict ValidDictKey::Value(json::NodeVariantType input) {
    builder_.Value(std::move(input));
    return ValidStartDict(builder_);
}

ValidStartDict ValidDictKey::StartDict()
{
    return BuilderValid::StartDict();
}

ValidStartArray ValidDictKey::StartArray()
{
    return BuilderValid::StartArray();
}

ValidDictKey ValidStartDict::Key(std::string key)
{
    return BuilderValid::Key(std::move(key));
}

BuilderValid ValidStartDict::EndDict() {
    return BuilderValid::EndDict();
}

ValidStartArray ValidStartArray::Value(json::NodeVariantType input) {
    builder_.Value(std::move(input));
    return ValidStartArray(builder_);
}

ValidStartDict ValidStartArray::StartDict()
{
    return BuilderValid::StartDict();
}

ValidStartArray ValidStartArray::StartArray()
{
    return BuilderValid::StartArray();
}

BuilderValid ValidStartArray::EndArray() {
    return BuilderValid::EndArray();
}

} // namespace compile_check
