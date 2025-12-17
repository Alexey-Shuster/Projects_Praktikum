#pragma once

#include "cell.h"
#include "common.h"
#include <unordered_map>

class Sheet : public SheetInterface {
public:
    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    bool HasCircularDependency(Position start, std::vector<Position> refs);

private:
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHash> cells_;
    Size size_;

    void CheckSetPosition(Position pos) const;
    bool CheckGetPosition(Position pos) const;
    void UpdateSize(Position pos);
    void RecalculateSize();
    void PrintInternal(std::ostream& output, bool print_values) const;
};
