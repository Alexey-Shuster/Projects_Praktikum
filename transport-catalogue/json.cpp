#include "json.h"

using namespace std;

namespace json {

Number LoadNumberFromInput(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream:"s + parsed_num);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, &parsed_num, read_char] {
        if (std::isalpha(input.peek())) {
            throw ParsingError("A digit is expected:"s + parsed_num);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadStringFromInput(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s{};
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
            case 'n':
                s.push_back('\n');
                break;
            case 't':
                s.push_back('\t');
                break;
            case 'r':
                s.push_back('\r');
                break;
            case '"':
                s.push_back('"');
                break;
            case '\\':
                s.push_back('\\');
                break;
            default:
                // Встретили неизвестную escape-последовательность
                throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return s;
}

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

namespace {

Node LoadNumber(istream& input) {
    auto value = LoadNumberFromInput(input);
    if (holds_alternative<int>(value)) {
        return Node(get<int>(value));
    } else {
        return Node(get<double>(value));
    }
}

Node LoadString(istream& input) {
    auto parsed_str = LoadStringFromInput(input);
    // std::cout << parsed_str;
    return Node(std::move(parsed_str));
}

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;

    for (char c; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (!input) {
        throw ParsingError("Array parsing error"s);
    }
    return Node(std::move(result));
}

Node LoadDict(istream& input) {
    Dict result{};

    for (char c; input >> c && c != '}';) {
        if (c == ',') {continue;}

        string key = LoadStringFromInput(input);
        input >> c;

        auto temp = LoadNode(input);
        // cout << "LoadDict: key = " << key << " : value = " << temp << endl;
        result.insert({std::move(key), std::move(temp)});
    }
    if (!input) {
        throw ParsingError("Dictionary parsing error"s);
    }
    return Node(std::move(result));
}

Node LoadBoolNull (istream& input) {
    std::string str{};
    char c;
    while (input >> c) {
        if (isalpha(c)) {str += c;}
        else {
            input.putback(c);
            break;
        }
    }
    if (str == "true") {return Node(true);}
    else if (str == "false") {return Node(false);}
    else if (str == "null") {return Node();}
    throw ParsingError("JSON wrong input:"s + str);
}

Node LoadNode(istream& input) {
    char c;
    if (!(input >> c)) {
        throw ParsingError("Unexpected EOF"s);
    }
    if (isdigit(c) || c == '-') {
        input.putback(c);
        return LoadNumber(input);
    }

    if (input.peek() == EOF) {
        throw ParsingError("JSON wrong input:"s);
    }

    if (c == 't' || c == 'f' || c == 'n') {
        input.putback(c);
        return LoadBoolNull(input);
    }

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else {
        throw ParsingError("JSON wrong input"s);
    }
}

}  // namespace

bool Node::IsNull() const
{
    return CheckNodeValueType<std::nullptr_t>();
}

bool Node::IsInt() const
{
    return CheckNodeValueType<int>();
}

bool Node::IsDouble() const
{
    return (IsPureDouble() || IsInt());
}

bool Node::IsPureDouble() const
{
    return CheckNodeValueType<double>();
}

bool Node::IsString() const
{
    return CheckNodeValueType<std::string>();
}

bool Node::IsBool() const
{
    return CheckNodeValueType<bool>();
}

bool Node::IsArray() const
{
    return CheckNodeValueType<Array>();
}

bool Node::IsMap() const
{
    return CheckNodeValueType<Dict>();
}

int Node::AsInt() const {
    return GetNodeValue<int>();
}

double Node::AsDouble() const {
    if (IsInt()) {
        return static_cast<double>(AsInt());
    }
    return GetNodeValue<double>();
}

const std::string& Node::AsString() const {
    return GetNodeValue<std::string>();
}

bool Node::AsBool() const {
    return GetNodeValue<bool>();
}

const Array& Node::AsArray() const {
    return GetNodeValue<Array>();
}

const Dict& Node::AsMap() const {
    return GetNodeValue<Dict>();
}

const NodeVariantType& Node::GetValue() const {
    return *this;
}

bool Node::operator ==(const Node &other) const {
    return GetValue() == other.GetValue();
}

bool Node::operator !=(const Node &other) const {
    return !(*this == other);
}

Document::Document(Node root)
    : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator ==(const Document &other) const
{
    return root_ == other.root_;
}

bool Document::operator !=(const Document &other) const
{
    return !(*this == other);
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void PrintNodeString (std::string_view input, std::ostream& out) {
    std::string result;
    result.reserve(input.size() * 2);
    out << '"';
    for (char c : input) {
        switch (c) {
            case '\r': out << "\\r"; break;
            case '\n': out << "\\n"; break;
            case '\t': out << "\\t"; break;
            case '"':  out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            // case '\b': out << "\\b"; break;
            // case '\f': out << "\\f"; break;
            default: out << c;
        }
    }
    out << '"';
}

void PrintNode(const Node& node, std::ostream& out, bool node_is_root = false);

void PrintNodeArray (const Array& input, std::ostream& out, bool node_is_root = false) {
    out << "[" << (node_is_root ? "\n" : "");
    bool first_loop = true;
    for (const auto& item : input) {
        if (first_loop) {first_loop = false;}
        else {out << ", " << ((node_is_root && (item.IsMap() || item.IsArray())) ? "\n" : "");}
        PrintNode(item, out);
    }
    out << (node_is_root ? "\n" : "") << "]" << (node_is_root ? "\n" : "");
}

void PrintNodeDict (const Dict& input, std::ostream& out, bool node_is_root = false) {
    out << "{" << (node_is_root ? "\n" : "");
    bool first_loop = true;
    for (const auto& [key, value] : input) {
        if (first_loop) {first_loop = false;}
        else {
            out << ", " << ((node_is_root && (value.IsArray() || value.IsMap())) ? "\n" : "");
        }
        PrintNodeString(key, out);
        out << ": ";
        PrintNode(value, out);
    }
    out << (node_is_root ? "\n" : "") << "}" << (node_is_root ? "\n" : "");
}

void PrintNode(const Node& node, std::ostream& out, bool node_is_root) {
    std::visit([&](const auto& arg) {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            out << arg;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            PrintNodeString(arg, out);
        }
        else if constexpr (std::is_same_v<T, bool>) {
            out << std::boolalpha << arg;
        }
        else if constexpr (std::is_same_v<T, Array>) {
            PrintNodeArray(arg, out, node_is_root);
        }
        else if constexpr (std::is_same_v<T, Dict>) {
            PrintNodeDict (arg, out, node_is_root);
        } else {
            out << "null";
        }
    }, node.GetValue());
}

void Print(const Document& doc, std::ostream& output) {
    // PrintContext context{output};
    const bool node_is_root = true;
    PrintNode(doc.GetRoot(), output, node_is_root);
    // context.out << "\n";
}

ostream &operator <<(std::ostream &os, const Node &node) {
    std::visit([&os](auto& arg) {
        using T = std::decay_t<decltype(arg)>;

        // if constexpr (std::is_same_v<T, std::nullptr_t>) {
        //     os << "null";
        // }
        if constexpr (std::is_same_v<T, int>) {
            os << "Node is Int:" << arg;
        }
        else if constexpr (std::is_same_v<T, double>) {
            os << "Node is Double:" << arg;
        }
        else if constexpr (std::is_same_v<T, std::string>) {
            os << "Node is String:" << arg;
        }
        else if constexpr (std::is_same_v<T, bool>) {
            os << "Node is Bool:" << std::boolalpha << arg;
        }
        else if constexpr (std::is_same_v<T, Array>) {
            os << "Node is Array of size = " << arg.size();
        }
        else if constexpr (std::is_same_v<T, Dict>) {
            os << "Node is Dict of size = " << arg.size();
        }
        else {
            os << "Node is null";
        }
    }, node.GetValue());

    return os;
}

}  // namespace json
