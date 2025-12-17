#include "sheet.h"

#include "common.h"

#include <iostream>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    CheckSetPosition(pos);

    // Увеличиваем размеры если нужно
    UpdateSize(pos);

    // Создаем ячейку если был nullptr
    if (!cells_[pos]) {     // if cell null
        cells_[pos] = std::make_unique<Cell>(*this, pos);
    }

    // Oбновляем ячейку
    cells_[pos]->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (CheckGetPosition(pos)) {
        return cells_.at(pos).get();
    }
    return nullptr;
}
CellInterface* Sheet::GetCell(Position pos) {
    if (CheckGetPosition(pos)) {
        return cells_[pos].get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    if (CheckGetPosition(pos)) {
        cells_[pos]->Clear();

        if (!cells_[pos]->IsReferenced()) {
            cells_.erase(pos);
            // Если очистили ячейку на границе - проверяем\изменяем размер
            if (pos.row +1 == size_.rows || pos.col +1 == size_.cols) {
                RecalculateSize();
            }
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintInternal(output, true);    // print_values = true
}
void Sheet::PrintTexts(std::ostream& output) const {
    PrintInternal(output, false);   // print_values = false
}

void Sheet::CheckSetPosition(Position pos) const
{
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position");
    }
}

bool Sheet::CheckGetPosition(Position pos) const
{
    CheckSetPosition(pos);
    auto iter = cells_.find(pos);
    if (iter != cells_.end()) {
        return true;
    }
    return false;
}

void Sheet::UpdateSize(Position pos)
{
    if (pos.row +1 > size_.rows) {
        size_.rows = pos.row + 1;
    }
    if (pos.col +1 > size_.cols) {
        size_.cols = pos.col + 1;
    }
}

void Sheet::RecalculateSize()
{
    int max_row = 0;
    int max_col = 0;
    int cells_not_null = 0;

    // Ищем самую правую и нижнюю непустую ячейку
    for (const auto& [pos, ptr] : cells_) {
        if (ptr != nullptr) {
            ++cells_not_null;
            max_row = std::max(max_row, pos.row);
            max_col = std::max(max_col, pos.col);
        }
    }

    if (cells_not_null == 0) {
        size_.rows = 0;
        size_.cols = 0;
    } else {
        size_.rows = max_row + 1;
        size_.cols = max_col + 1;
    }
}

void Sheet::PrintInternal(std::ostream &output, bool print_values) const
{
    Size printable_size = GetPrintableSize();

    for (int row = 0; row < printable_size.rows; ++row) {
        bool first_in_row = true;

        for (int col = 0; col < printable_size.cols; ++col) {
            Position pos{row, col};
            const CellInterface* cell = GetCell(pos);

            // Добавляем табуляцию перед каждой ячейкой, кроме первой в строке
            if (!first_in_row) {
                output << '\t';
            }
            first_in_row = false;

            if (cell != nullptr) {
                if (print_values) {
                    // Печать значения
                    const auto& value = cell->GetValue();
                    std::visit([&output](const auto& v) { output << v; }, value);
                } else {
                    // Печать текста
                    output << cell->GetText();
                }
            }
            // Для пустой ячейки ничего не выводим
        }
        output << '\n';
    }
}

bool Sheet::HasCircularDependency(Position start, std::vector<Position> refs)
{
    std::unordered_set<Position, PositionHash> visited;

    while (!refs.empty()) {
        Position current = refs.back();
        refs.pop_back();

        // Found cycle back to starting cell
        if (current == start) {
            return true;
        }

        // Skip if already visited
        if (!visited.insert(current).second) {
            continue;
        }

        // Get references of current cell
        if (auto cell = GetCell(current)) {
            auto cell_refs = cell->GetReferencedCells();
            refs.insert(refs.end(), cell_refs.begin(), cell_refs.end());
        }
    }

    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
