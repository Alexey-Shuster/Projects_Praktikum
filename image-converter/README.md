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
```

## CMake options

-DLIBJPEG_DIR=/path/to/libjpeg – must point to a directory containing include/ and lib/ subdirectories with LibJPEG.

The build produces:

- libImgLib.a (or .lib) – static library for image I/O
- imgconv (or .exe) – the converter executable

## Usage

```bash
./imgconv input.jpg output.bmp
```
Supported extensions: .jpg, .jpeg, .ppm, .bmp.
The tool detects format from the file extension. If input or output format is unknown, an error is reported.

## Project Structure

- ImgLib/ – static library with format implementations
- bmp_image.h/cpp – BMP reader/writer
- jpeg_image.h/cpp – JPEG wrapper using libjpeg
- ppm_image.h/cpp – PPM reader/writer
- img_lib.h/cpp – core Image class and color representation
- main.cpp – converter frontend with format dispatch

## Error Handling

- Returns exit code 1 on incorrect argument count.
- 2 – unknown file extension.
- 4 – loading failed.
- 5 – saving failed.

## License

Feel free to use and adapt for learning purposes.
