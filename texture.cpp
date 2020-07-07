#include "texture.h"
#include <QFileInfo>
#include <QImageReader>
#include <iostream>
#include <jerror.h>
#include <jpeglib.h>
#include <png.h>

namespace Mpcv {

JpegTexture::JpegTexture(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "rb");

    jpeg_error_mgr err;
    jpeg_decompress_struct info;
    info.err = jpeg_std_error(&err);
    jpeg_create_decompress(&info);

    if (!file) {
        throw std::runtime_error("Error reading JPEG image '" + filename + "'");
    }

    jpeg_stdio_src(&info, file);
    jpeg_read_header(&info, TRUE);

    jpeg_start_decompress(&info);

    width_ = info.output_width;
    height_ = info.output_height;
    channels_ = info.num_components;

    std::cout << "Loading JPEG image of size " << width_ << "x" << height_ << std::endl;

    data_ = (uint8_t*)malloc(width_ * height_ * channels_);
    uint8_t* rowptr[1];
    while (info.output_scanline < info.output_height) {
        rowptr[0] = (uint8_t*)data_ + channels_ * info.output_width * info.output_scanline;
        jpeg_read_scanlines(&info, rowptr, 1);
    }

    jpeg_finish_decompress(&info);
    jpeg_destroy_decompress(&info);
    fclose(file);

    std::cout << "row = " << int(data_[0]) << "," << int(data_[1]) << "," << int(data_[2]) << ","
              << int(data_[3]) << "," << int(data_[4]) << "," << int(data_[5]) << "," << int(data_[6])
              << std::endl;
}

JpegTexture::~JpegTexture() {
    free(data_);
}

Pvl::Vec2i JpegTexture::size() const {
    return Pvl::Vec2i(width_, height_);
}

ImageFormat JpegTexture::format() const {
    return channels_ == 4 ? ImageFormat::RGBA : ImageFormat::RGB;
}

uint8_t* JpegTexture::data() {
    return data_;
}

PngTexture::PngTexture(const std::string& filename) {
    png_image image;
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;

    if (!png_image_begin_read_from_file(&image, filename.c_str())) {
        throw std::runtime_error("Failed to read PNG file '" + filename + "'");
    }

    image.format = PNG_FORMAT_RGB;
    data_ = (uint8_t*)malloc(PNG_IMAGE_SIZE(image));

    if (!png_image_finish_read(&image, nullptr, data_, 0, nullptr)) {
        throw std::runtime_error("Failed to read PNG file '" + filename + "'");
    }
    width_ = image.width;
    height_ = image.height;
    channels_ = PNG_IMAGE_SAMPLE_CHANNELS(image.format);
}

PngTexture::~PngTexture() {
    free(data_);
}

Pvl::Vec2i PngTexture::size() const {
    return Pvl::Vec2i(width_, height_);
}

ImageFormat PngTexture::format() const {
    return channels_ == 4 ? ImageFormat::RGBA : ImageFormat::RGB;
}

uint8_t* PngTexture::data() {
    return data_;
}

std::unique_ptr<ITexture> makeTexture(const QString& filename) {
    QString ext = QFileInfo(filename).suffix();
    QImageReader reader(filename);
    QSize size = reader.size();
    const int maxQtSize = (1 << 15) - 1;
    if (size.width() <= maxQtSize && size.height() <= maxQtSize) {
        std::cout << "Loading image '" + filename.toStdString() + "' using Qt reader" << std::endl;
        return std::make_unique<QtTexture>(reader.read());
    } else if (ext == "jpg" || ext == "jpeg") {
        std::cout << "Loading image '" + filename.toStdString() + "' using libjpeg reader" << std::endl;
        return std::make_unique<JpegTexture>(filename.toStdString());
    } else if (ext == "png") {
        std::cout << "Loading image '" + filename.toStdString() + "' using libpng reader" << std::endl;
        return std::make_unique<PngTexture>(filename.toStdString());
    } else {
        throw std::runtime_error("Cannot read texture '" + filename.toStdString() + "', image too large");
    }
}

} // namespace Mpcv
