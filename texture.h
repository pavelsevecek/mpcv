#pragma once

#include "pvl/Vector.hpp"
#include <QImage>
#include <cstdint>
#include <memory>

namespace Mpcv {

enum class ImageFormat {
    GRAY,
    RGB,
    BGR,
    RGBA,
    BGRA,
};

class ITexture {
public:
    virtual ~ITexture() = default;

    virtual Pvl::Vec2i size() const = 0;

    virtual ImageFormat format() const = 0;

    virtual uint8_t* data() = 0;
};

class QtTexture : public ITexture {
    QImage image_;

public:
    QtTexture(QImage&& image)
        : image_(std::move(image)) {
        if (image_.format() == QImage::Format_Invalid) {
            throw std::runtime_error("Error reading texture image");
        }
    }

    virtual Pvl::Vec2i size() const override {
        return Pvl::Vec2i(image_.width(), image_.height());
    }

    virtual ImageFormat format() const override {
        switch (image_.depth()) {
        case 32:
            return ImageFormat::BGRA;
        case 24:
            return ImageFormat::BGR;
        case 8:
            return ImageFormat::GRAY;
        default:
            throw std::runtime_error("Unknown image depth = " + std::to_string(image_.depth()));
        }
    }

    virtual uint8_t* data() override {
        return image_.bits();
    }
};

#ifdef HAS_JPEG

class JpegTexture : public ITexture {
    uint8_t* data_;
    std::size_t width_, height_;
    std::size_t channels_;

public:
    JpegTexture(const std::string& filename);

    ~JpegTexture();

    virtual Pvl::Vec2i size() const override;

    virtual ImageFormat format() const override;

    virtual uint8_t* data() override;
};

#endif

#ifdef HAS_PNG

class PngTexture : public ITexture {
    uint8_t* data_;
    std::size_t width_, height_;
    std::size_t channels_;

public:
    PngTexture(const std::string& filename);

    ~PngTexture();

    virtual Pvl::Vec2i size() const override;

    virtual ImageFormat format() const override;

    virtual uint8_t* data() override;
};

#endif

std::unique_ptr<ITexture> makeTexture(const QString& filename);

} // namespace Mpcv
