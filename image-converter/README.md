# Image Converter

A cross-platform command-line tool for converting between **BMP**, **JPEG**, and **PPM** image formats.  
Built in C++17 with a custom `ImgLib` static library and optional LibJPEG integration.

## Features

- Convert any supported input format to any supported output format (based on file extension)
- Uses a clean polymorphic interface for image format handlers
- Automatic padding and BGR/RGB conversion for BMP
- Relies on `libjpeg` for JPEG I/O; PPM and BMP are implemented natively

## Dependencies

- **C++17** compiler (GCC, Clang, MSVC)
- **CMake 3.11+**
- **LibJPEG** – required for JPEG support.  
  Download or build a static version and note its installation path.

## Building

```bash
git clone <repo>
cd <repo>
mkdir build && cd build

# Provide the path to LibJPEG (adjust according to your system)
cmake .. -DLIBJPEG_DIR=/path/to/libjpeg

cmake --build . --config Release