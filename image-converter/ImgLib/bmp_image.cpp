#include "bmp_image.h"
#include "pack_defines.h"

#include <fstream>

using namespace std;

namespace img_lib {

PACKED_STRUCT_BEGIN BitmapFileHeader {
    uint16_t signature = 0x4D42; // "BM"
    uint32_t file_size = 0;
    uint16_t reserved1 = 0;
    uint16_t reserved2 = 0;
    uint32_t data_offset = 54; // 14 + 40 bytes
}
PACKED_STRUCT_END

PACKED_STRUCT_BEGIN BitmapInfoHeader {
    uint32_t header_size = 40;
    int32_t width = 0;
    int32_t height = 0;
    uint16_t planes = 1;
    uint16_t bits_per_pixel = 24;
    uint32_t compression = 0;
    uint32_t data_size = 0;
    int32_t horizontal_res = 11811;
    int32_t vertical_res = 11811;
    int32_t colors_used = 0;
    int32_t important_colors = 0x1000000;
}
PACKED_STRUCT_END

static int GetBMPStride(int width) {
    const int bytes_per_pixel = 3;      // 24 bits per pixel = 3 bytes
    const int alignment_boundary = 4;   // BMP requires 4-byte alignment

    return alignment_boundary * ((width * bytes_per_pixel + (alignment_boundary - 1)) / alignment_boundary);
}

bool SaveBMP(const Path& file, const Image& image) {
    ofstream out(file, ios::binary);
    if (!out) {
        return false;
    }

    const int width = image.GetWidth();
    const int height = image.GetHeight();
    const int stride = GetBMPStride(width);
    const uint32_t data_size = stride * height;
    const uint32_t file_size = sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader) + data_size; // 14 + 40 + data_size

    // Prepare headers
    BitmapFileHeader file_header;
    file_header.file_size = file_size;
    file_header.data_offset = 54;

    BitmapInfoHeader info_header;
    info_header.width = width;
    info_header.height = height;
    info_header.data_size = data_size;

    // Write headers
    out.write(reinterpret_cast<const char*>(&file_header), sizeof(file_header));
    out.write(reinterpret_cast<const char*>(&info_header), sizeof(info_header));

    // Write image data (bottom-to-top, BGR format with padding)
    std::vector<char> buffer(stride, 0);

    for (int y = height - 1; y >= 0; --y) {
        const Color* line = image.GetLine(y);
        
        // Fill pixel data (BGR format)
        for (int x = 0; x < width; ++x) {
            buffer[x * 3 + 0] = static_cast<char>(line[x].b); // Blue
            buffer[x * 3 + 1] = static_cast<char>(line[x].g); // Green
            buffer[x * 3 + 2] = static_cast<char>(line[x].r); // Red
        }
        
        // Write line with padding
        out.write(buffer.data(), stride);
    }

    return out.good();
}

Image LoadBMP(const Path& file) {
    ifstream in(file, ios::binary);
    if (!in) {
        return {};
    }

    // Read headers
    BitmapFileHeader file_header;
    BitmapInfoHeader info_header;

    in.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
    if (!in) {
        return {};
    }
    in.read(reinterpret_cast<char*>(&info_header), sizeof(info_header));
    if (!in) {
        return {};
    }

    // Validate BMP format
    if (file_header.signature != 0x4D42 || // "BM"
        info_header.bits_per_pixel != 24 ||
        info_header.compression != 0) {
        return {};
    }

    const int width = info_header.width;
    const int height = info_header.height;
    
    if (width <= 0 || height == 0) {
        return {};
    }

    const bool top_to_bottom = height < 0;
    const int abs_height = abs(height);
    
    Image result(width, abs_height, Color::Black());
    const int stride = GetBMPStride(width);
    std::vector<char> buffer(stride);

    // Seek to pixel data
    in.seekg(file_header.data_offset, ios::beg);

    for (int y = 0; y < abs_height; ++y) {
        in.read(buffer.data(), stride);
        if (!in) {
            return {};
        }

        // Calculate destination line (BMP stores bottom-to-top by default)
        int dest_y = top_to_bottom ? y : abs_height - 1 - y;
        Color* line = result.GetLine(dest_y);

        // Convert BGR to RGB
        for (int x = 0; x < width; ++x) {
            line[x].b = static_cast<byte>(buffer[x * 3 + 0]); // Blue
            line[x].g = static_cast<byte>(buffer[x * 3 + 1]); // Green
            line[x].r = static_cast<byte>(buffer[x * 3 + 2]); // Red
        }
    }

    return result;
}

}  // namespace img_lib
