#pragma once

#include "common.h"
#include <memory>
#include <optional>
#include <unordered_set>

class Impl;
class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position pos);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;      // if Cell required in futher calculations

private:
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    Position pos_;
    mutable std::optional<Value> cached_value_;                     // used as cash for quick formula calculation, =0 for empty Cell
    std::unordered_set<Position, PositionHash> cells_required_;     // Cells required for current Cell calculation
    std::unordered_set<Position, PositionHash> cells_dependent_;    // Cells which calculation depends from current Cell

    void UpdateDependencies(const std::vector<Position>& new_refs);
    void RecalculateCurrentCache(Cell* cell);
    void UpdateDependentCache();
    void SetEmptyImpl();
    void TryConvertTextDouble(const std::string& text);
};
