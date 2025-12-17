#include <img_lib.h>
#include <bmp_image.h>
#include <jpeg_image.h>
#include <ppm_image.h>

#include <iostream>

namespace image_format {

class ImageFormatInterface {
public:
    virtual bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const = 0;
    virtual img_lib::Image LoadImage(const img_lib::Path& file) const = 0;
    virtual ~ImageFormatInterface() = default;
};

class ImageFormatJPEG : public ImageFormatInterface {
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveJPEG(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadJPEG(file);
    }
};

class ImageFormatPPM : public ImageFormatInterface {
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SavePPM(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadPPM(file);
    }
};

class ImageFormatBMP : public ImageFormatInterface {
    bool SaveImage(const img_lib::Path& file, const img_lib::Image& image) const override {
        return img_lib::SaveBMP(file, image);
    }
    img_lib::Image LoadImage(const img_lib::Path& file) const override {
        return img_lib::LoadBMP(file);
    }
};

} // namespace image_format

namespace {

enum Format {
    JPEG,
    PPM,
    BMP,
    UNKNOWN
};

Format GetFormatByExtension(const img_lib::Path& input_file) {
    const std::string ext = input_file.extension().string();
    if (ext == ".jpg" || ext == ".jpeg") {
        return Format::JPEG;
    }

    if (ext == ".ppm") {
        return Format::PPM;
    }

    if (ext == ".bmp") {
        return Format::BMP;
    }

    return Format::UNKNOWN;
}

image_format::ImageFormatInterface* GetFormatInterface(const img_lib::Path& path) {
    static image_format::ImageFormatJPEG jpeg_format;
    static image_format::ImageFormatPPM ppm_format;
    static image_format::ImageFormatBMP bmp_format;

    switch (GetFormatByExtension(path)) {
    case Format::JPEG:
        return &jpeg_format;
    case Format::PPM:
        return &ppm_format;
    case Format::BMP:
        return &bmp_format;
    default:
        return nullptr;
    }
}

} // namespace

int main(int argc, const char** argv) {

    // argc = 3;
    // argv[1] = "in.ppm";
    // argv[2] = "out.jpeg";

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <in_file> <out_file>" << std::endl;
        return 1;
    }

    img_lib::Path in_path = argv[1];
    if (GetFormatByExtension(in_path) == Format::UNKNOWN) {
        std::cerr << "Unknown format of the input file" << std::endl;
        return 2;
    }

    img_lib::Path out_path = argv[2];
    if (GetFormatByExtension(out_path) == Format::UNKNOWN) {
        std::cerr << "Unknown format of the output file" << std::endl;
        return 2;
    }

    img_lib::Image image = GetFormatInterface(in_path)->LoadImage(in_path);
    if (!image) {
        std::cerr << "Loading failed" << std::endl;
        return 4;
    }

    if (!GetFormatInterface(out_path)->SaveImage(out_path, image)) {
        std::cerr << "Saving failed" << std::endl;
        return 5;
    }

    std::cout << "Successfully converted" << std::endl;
}
