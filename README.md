### ⚙️ C++ Projects (Yandex Praktikum)

* **[advanced-vector](advanced-vector)**
  A header-only custom C++ vector container. Employs `RawMemory` with manual construction/destruction and strong exception safety. Supports `Emplace`/`EmplaceBack` with Copy & Move semantics based on type traits. Designed for benchmarking against `std::vector`.

* **[image-converter](image-converter)**
  A `C++17` tool for converting `BMP`, `JPEG`, and `PPM` images. Requires `LibJPEG`.

* **[preprocessor](preprocessor)**
  A lightweight tool that recursively processes `#include` directives in C++ source files, resolving both quoted and angle-bracketed includes using custom search directories.

* **[simple-vector](simple-vector)**
  A simple C++ vector implementation. Uses `ArrayPtr` with simple reallocation logic. Relies on temporary copies for insertion and less optimal growth.

* **[single-linked-list](single-linked-list)**
  A simple linked list with forward iterators, strong exception safety and operations like `InsertAfter`, `EraseAfter`, `PushFront`, and `PopFront`.

* **[spreadsheet](spreadsheet)**
  This project implements a `Spreadsheet` engine in C++17 that supports text cells and arithmetic formulas with cell references. It includes a custom formula parser built with `ANTLR`, circular dependency detection, error handling (e.g., division by zero, invalid references) and the ability to print the sheet as values or raw texts. The code provides a complete `SheetInterface` with methods for setting, getting and clearing cells, as well as computing printable size.

* **[transport-catalogue](transport-catalogue)**
  This project implements a transport directory system in C++17 that processes `JSON` input to build a database of bus stops and routes, computes route lengths and curvatures, performs graph-based routing between stops, and renders an `SVG` map. It uses custom JSON parsing/building, a directed weighted graph for route finding and a map renderer with configurable visual settings. The application reads base requests and stat requests from `std::cin`, then outputs JSON responses and SVG maps.
