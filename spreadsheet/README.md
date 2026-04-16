# Spreadsheet Engine

A C++17 spreadsheet library that supports **text cells**, **arithmetic formulas**, and **cell references**.  
Built with a custom ANTLR‑based formula parser, automatic dependency tracking, and circular detection.

## Features

- **Text cells** – plain text, with optional escaping (`'` prefix to treat `=` as literal).
- **Formula cells** – start with `=` followed by an expression (e.g., `=A1+B2*3`).
- **Cell references** – formulas can refer to any cell (empty cells are treated as `0`).
- **Arithmetic operations** – `+`, `-`, `*`, `/`, unary plus/minus, parentheses.
- **Error handling** – returns `#REF!`, `#VALUE!`, or `#ARITHM!` for invalid references, non‑numeric values, or arithmetic errors (division by zero, overflow).
- **Circular dependency detection** – throws `CircularDependencyException` when a formula would create a cycle.
- **Caching** – computed formula values are cached and automatically invalidated when dependent cells change.
- **Printable size** – bounding rectangle of all non‑empty cells.
- **Two output modes** – `PrintValues` (evaluated results) and `PrintTexts` (raw cell contents).
- **Position utilities** – conversion between `(row, col)` and spreadsheet notation (e.g., `A1`).

## Dependencies

- **C++17** compiler (GCC, Clang, MSVC)
- **CMake 3.8+**
- **Java Runtime** – required for running ANTLR during build.
- **ANTLR 4.13.2** – included as `antlr-4.13.2-complete.jar` in the project root.

## Building

```bash
git clone <repo>
cd <repo>
mkdir build && cd build

# Configure (CMake will use antlr-4.13.2-complete.jar automatically)
cmake ..

# Build
cmake --build . --config Release
```

The build produces:
- `spreadsheet` (or `spreadsheet.exe`) – the executable containing the test suite.

## Usage

```cpp
#include "sheet.h"
#include "common.h"

int main() {
    auto sheet = CreateSheet();

    // Set text cells
    sheet->SetCell(Position::FromString("A1"), "Hello");
    sheet->SetCell(Position::FromString("B1"), "World");

    // Set formula
    sheet->SetCell(Position::FromString("A2"), "=1+2*3");
    sheet->SetCell(Position::FromString("B2"), "=A1");

    // Get value (evaluated)
    const CellInterface* cell = sheet->GetCell(Position::FromString("A2"));
    if (cell) {
        auto value = cell->GetValue(); // variant<string, double, FormulaError>
        // ...
    }

    // Print as values
    sheet->PrintValues(std::cout);

    // Clear a cell
    sheet->ClearCell(Position::FromString("A1"));
}
```

## CMake Options

| Option | Description |
|--------|-------------|
| `-DANTLR_EXECUTABLE` | Path to ANTLR jar (default: `./antlr-4.13.2-complete.jar`) |

## Project Structure

- `common.h` – core types: `Position`, `Size`, `FormulaError`, `CellInterface`, `SheetInterface`
- `cell.h` / `cell.cpp` – `Cell` implementation, caching, dependency management
- `sheet.h` / `sheet.cpp` – `Sheet` implementation, size recalculation, printing, circular check
- `formula.h` / `formula.cpp` – formula parsing and evaluation using ANTLR AST
- `FormulaAST.h` / `FormulaAST.cpp` – abstract syntax tree for formulas
- `Formula.g4` – ANTLR grammar for formulas (numbers, ops, cell references)
- `structures.cpp` – `Position` conversion and validation helpers
- `test_runner_p.h` – lightweight unit test framework
- `main.cpp` – comprehensive test suite (run the executable to execute tests)

## Error Handling

The library throws exceptions in the following situations:

| Exception | Condition |
|-----------|-----------|
| `InvalidPositionException` | Position row/col out of bounds or negative |
| `FormulaException` | Syntactically incorrect formula string |
| `CircularDependencyException` | Formula would create a cycle (e.g., `A1 = B1`, `B1 = A1`) |

Formula evaluation errors are returned as `FormulaError` values (not thrown):
- `#REF!` – cell reference is invalid (e.g., `=XFD16385`)
- `#VALUE!` – referenced cell contains non‑numeric text
- `#ARITHM!` – division by zero or floating‑point overflow

## Running the Tests

The executable contains a built‑in test suite. Run it without arguments:

```bash
./spreadsheet
```

All tests from `main.cpp` will execute and report success/failure.

## Limitations

- Maximum sheet size: `16384` rows × `16384` columns (as per `Position::MAX_ROWS`/`MAX_COLS`).
- Only cell references are supported (no built‑in functions like `SUM`, `IF`).
- All numbers are `double`; no integer arithmetic mode.

## License

Feel free to use and adapt for learning purposes.