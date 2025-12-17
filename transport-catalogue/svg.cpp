#include "svg.h"
#include <iomanip>

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object> &&obj)
{
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream &out) const
{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << "\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << "\n";
    RenderContext ctx(out, 2, 2);
    for (const auto& item : objects_) {
        item->Render(ctx);
    }
    out << "</svg>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos)
{
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset)
{
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size)
{
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family)
{
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight)
{
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(std::string data)
{
    data_ = data;
    return *this;
}

void Text::RenderObject(const RenderContext &context) const
{
    auto& out = context.out;
    out << "<text";
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\""sv;
    out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
    out << " font-size=\""sv << size_ << "\""sv;
    if (!font_family_.empty()) {
        out << " font-family=\""sv << font_family_ << "\""sv;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""sv << font_weight_ << "\""sv;
    }
    out << ">" << RenderTextData(data_) << "</text>"sv;
}

std::string Text::RenderTextData(const std::string& input) const
{
    std::string result;
    result.reserve(input.size() * 2);
    for (auto& item : input) {
        switch (item) {
            case '\"': result += "&quot;"; break;
            case '\'': result += "&apos;"; break;
            case '&':  result += "&amp;"; break;
            case '<':  result += "&lt;"; break;
            case '>':  result += "&gt;"; break;
            default:   result += item; break;
        }
    }
    return result;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point)
{
    points_.emplace_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext &context) const
{
    auto& out = context.out;
    out << "<polyline points=\"";
    bool first_iteration = true;
    for (const auto item : points_) {
        if (first_iteration) {
            first_iteration = false;
        } else {
            out << " ";
        }
        out << item.x << "," << item.y;
    }
    out << "\"";
    // Выводим атрибуты, унаследованные от PathProps
    RenderAttrs(context.out);
    out << "/>";
}

std::ostream& operator<<(std::ostream& os, const Rgb& color) {
    os << "rgb(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green) << "," << static_cast<int>(color.blue) << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Rgba& color) {
    os << "rgba(" << static_cast<int>(color.red) << "," << static_cast<int>(color.green) << "," << static_cast<int>(color.blue) << "," << color.opacity << ")";
    return os;  //<< std::fixed << std::setprecision(1)
}

std::ostream& operator<<(std::ostream& os, const Color& color) {
    std::visit([&os](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;

        // if constexpr (std::is_same_v<T, std::monostate>) {
        //     os << NoneColor;
        // }
        if constexpr (std::is_same_v<T, std::string>) {
            os << arg;
        } else if constexpr (std::is_same_v<T, Rgb> || std::is_same_v<T, Rgba>) {
            os << arg; // Uses the Rgb/Rgba operators<<
        } else {
            os << "none";
        }
    }, color);
    // std::visit(ColorPrinter{os}, color);
    return os;
}

std::ostream& operator<<(std::ostream& os, const StrokeLineCap line_cap) {
    switch (line_cap) {
    case StrokeLineCap::BUTT:   os << "butt"; break;
    case StrokeLineCap::ROUND:  os << "round"; break;
    case StrokeLineCap::SQUARE: os << "square"; break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const StrokeLineJoin line_join) {
    switch (line_join) {
    case StrokeLineJoin::ARCS:      os << "arcs"; break;
    case StrokeLineJoin::BEVEL:     os << "bevel"; break;
    case StrokeLineJoin::MITER:     os << "miter"; break;
    case StrokeLineJoin::MITER_CLIP: os << "miter-clip"; break;
    case StrokeLineJoin::ROUND:     os << "round"; break;
    }
    return os;
}

template<typename Owner>
void PathProps<Owner>::RenderAttrs(std::ostream& out) const {
    using namespace std::literals;

    if (fill_color_) {
        out << " fill=\""sv << *fill_color_ << "\""sv;
    }
    if (stroke_color_) {
        out << " stroke=\""sv << *stroke_color_ << "\""sv;
    }
    if (stroke_width_) {
        out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
    }
    if (stroke_linecap_) {
        out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
    }
    if (stroke_linejoin_) {
        out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
    }
}

void ColorPrinter::operator()(std::monostate) const{
    os << "none";
}
void ColorPrinter::operator()(std::string str) const {
    os << str;
}
void ColorPrinter::operator()(Rgb color) const {
    os << color;
}
void ColorPrinter::operator()(Rgba color) const {
    os << color;
}

}  // namespace svg
