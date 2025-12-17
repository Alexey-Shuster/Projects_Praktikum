#include "cell.h"
#include "formula.h"
#include "sheet.h"

#include <cassert>
#include <string>

class Impl {
public:
    virtual ~Impl() = default;
    virtual Cell::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const = 0;
};

class EmptyImpl : public Impl {
public:
    Cell::Value GetValue() const override {
        return 0.0;         // if Cell.IsReferenced & impl_=EmptyImpl - this treated as Cell contains ZERO value
    };
    std::string GetText() const override {
        return "";
    };
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
};

class TextImpl : public Impl {
public:
    TextImpl(std::string value) : value_(value) {};
    Cell::Value GetValue() const override {
        if (value_.front() == ESCAPE_SIGN) {
            return value_.substr(1);
        }
        return value_;
    };
    std::string GetText() const override {
        return value_;
    };
    std::vector<Position> GetReferencedCells() const override {
        return {};
    }
private:
    std::string value_;
};

class FormulaImpl : public Impl {
public:
    FormulaImpl(std::string formula, const SheetInterface& sheet) : formula_(ParseFormula(std::move(formula))), sheet_(sheet) {};
    Cell::Value GetValue() const override {
        auto result = formula_->Evaluate(sheet_);       // FormulaInterface::Value = std::variant<double, FormulaError>
        if (std::holds_alternative<double>(result)) {
            return std::get<double>(result);
        }
        try {
            return std::get<FormulaError>(result);
        } catch (...) {
            throw std::runtime_error("FormulaImpl::GetValue returns NOR double NOR FormulaError");
        }
    };

    std::string GetText() const override {
        return FORMULA_SIGN + formula_->GetExpression();
    };

    std::vector<Position> GetReferencedCells() const override {
        return formula_->GetReferencedCells();
    }
private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface& sheet_;
};

Cell::Cell(Sheet& sheet, Position pos) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet), pos_(pos) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> new_impl;
    if (text.empty()) {
        Clear();
        SetEmptyImpl();
    } else if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        try {
            new_impl = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
        }
        catch (...) {
            throw FormulaException("Incorrect formula");
        }
        if (sheet_.HasCircularDependency(pos_, new_impl->GetReferencedCells())) {
            throw CircularDependencyException("Circular Dependency found at Position: [" + pos_.ToString() + "] with input text: " + text);
        }
        Clear();
        impl_ = std::move(new_impl);
        UpdateDependencies(impl_->GetReferencedCells());

        RecalculateCurrentCache(this);
    } else {
        Clear();
        impl_ = std::make_unique<TextImpl>(text);

        TryConvertTextDouble(text);
    }
    UpdateDependentCache();
}

void Cell::Clear() {
    assert(impl_ != nullptr);

    // Remove dependencies - no more cells required for calculation
    UpdateDependencies({});

    if (IsReferenced()) {
        SetEmptyImpl();    // store EmptyImpl if cell required in futher calculations
        UpdateDependentCache();
    } else {
        impl_.reset();
    }
}

Cell::Value Cell::GetValue() const {
    assert(cached_value_.has_value());

    return cached_value_.value();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const
{
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const
{
    return !cells_dependent_.empty();
}

void Cell::UpdateDependencies(const std::vector<Position> &new_refs)
{
    // Clear old dependencies
    for (auto& old_ref : cells_required_) {
        auto cell = sheet_.GetCell(old_ref);
        assert(cell != nullptr);
        if (cell) {
            auto cell_cast = static_cast<Cell*>(cell);
            cell_cast->cells_dependent_.erase(pos_);
            if (cell_cast->cells_dependent_.empty() && cell_cast->GetText().empty()) {      // if Cell empty and NO dependants - ClearCell
                sheet_.ClearCell(old_ref);
            }
        }
    }
    cells_required_.clear();

    // Add new dependencies
    for (auto& new_ref : new_refs) {
        if (new_ref.IsValid()) {
            cells_required_.insert(new_ref);
        } else {
            throw FormulaError(FormulaError::Category::Ref);
        }

        // Get or create the referenced cell
        auto cell = sheet_.GetCell(new_ref);
        if (!cell) {
            sheet_.SetCell(new_ref, "");        // Create empty cell
            cell = sheet_.GetCell(new_ref);
        }

        if (cell) {
            static_cast<Cell*>(cell)->cells_dependent_.insert(pos_);
        }
    }
}

void Cell::RecalculateCurrentCache(Cell* cell)
{
    assert(cell != nullptr);

    // Invalidate cache
    cell->cached_value_.reset();

    assert(cell->impl_ != nullptr);

    // Recompute value
    cell->cached_value_ = cell->impl_->GetValue();
}

void Cell::UpdateDependentCache()
{
    // Invalidate cache of all dependent cells
    for (const auto& dependent_pos : cells_dependent_) {
        auto cell = sheet_.GetCell(dependent_pos);
        assert(cell != nullptr);

        auto dependent_cell = static_cast<Cell*>(cell);

        RecalculateCurrentCache(dependent_cell);

        // Recursively update dependent cells
        dependent_cell->UpdateDependentCache();
    }
}

void Cell::SetEmptyImpl()
{
    impl_ = std::make_unique<EmptyImpl>();
    cached_value_ = 0.0;
}

void Cell::TryConvertTextDouble(const std::string &text)
{
    try {
        size_t pos = 0;
        double value = std::stod(text, &pos);

        // Check if entire string was consumed
        if (pos == text.length()) {
            cached_value_ = value;
        } else {
            cached_value_ = impl_->GetValue();
        }
    } catch (...) {
        cached_value_ = impl_->GetValue();
    }
}
