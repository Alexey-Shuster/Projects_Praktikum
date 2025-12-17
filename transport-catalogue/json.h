#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;
using NodeVariantType = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;
using Number = std::variant<int, double>;   // for LoadNumber(std::istream& input)

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final : private NodeVariantType {
public:
    using variant::variant;

    bool IsNull() const;
    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsString() const;
    bool IsBool() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    double AsDouble() const;
    const std::string& AsString() const;
    bool AsBool() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    const NodeVariantType& GetValue() const;

    bool operator == (const Node& other) const;
    bool operator != (const Node& other) const;

private:
    template<typename T>
    const T& GetNodeValue() const {
        if (!CheckNodeValueType<T>()) {
            throw std::logic_error("Wrong type in variant");
        }
        return std::get<T>(*this);
    }

    template<typename T>
    bool CheckNodeValueType() const {
        return std::holds_alternative<T>(*this);
    }

};

std::ostream& operator << (std::ostream& os, const Node& node);

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator == (const Document& other) const;
    bool operator != (const Document& other) const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

Number LoadNumberFromInput(std::istream& input);

std::string LoadStringFromInput(std::istream& input);

}  // namespace json
