#include "formula.h"

#include "FormulaAST.h"

#include <cassert>
#include <cctype>
#include <set>
#include <sstream>

using namespace std::literals;

namespace {
class Formula : public FormulaInterface {
public:
// Реализуйте следующие методы:
    explicit Formula(std::string expression) try
        : ast_(FormulaAST(ParseFormulaAST(std::move(expression)))) {
    } catch (...) {
        throw FormulaException("ParseFormulaAST failed");
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        } catch (const FormulaError& fe) {
            return fe;
        } catch (const std::exception& e) {
            throw e.what();
        } catch (...) {
            throw std::runtime_error("Unknown exception in Formula::Evaluate");
        }
    }
    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        auto cells_list = ast_.GetCells();
        std::set<Position> cells_unique(cells_list.begin(), cells_list.end());
        return std::vector<Position>{cells_unique.begin(), cells_unique.end()};
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}

FormulaError::FormulaError(Category category)
    : category_(category) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
    case Category::Ref:
        return "#REF!";
    case Category::Value:
        return "#VALUE!";
    case Category::Arithmetic:
        return "#ARITHM!";
    default:
        return "#ERROR!";
    }
    // Fallback for safety (should never be reached with valid Category)
    return "#UNKNOWN!";
}

// std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
//     return output << fe.ToString();
// }
