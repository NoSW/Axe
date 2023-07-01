#pragma once

#include "03Resource/Config.hpp"
#include "02Rhi/Rhi.hpp"

#include <tiny_imageformat/tinyimageformat_base.h>

#include <filesystem>

struct TinyDDS_Context;

namespace axe::resource
{

class ImageDDS
{
    AXE_NON_COPYABLE(ImageDDS);

private:
    [[nodiscard]] bool _reset(std::istream& ifs) noexcept;

public:
    ImageDDS() = default;
    ~ImageDDS() noexcept;

    [[nodiscard]] bool reset(const std::filesystem::path& imageFilePath) noexcept;  // must check return

    [[nodiscard]] bool reset(u8* pData, u64 dataBytes) noexcept;  // must check retur

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] u32 width() const noexcept;
    [[nodiscard]] u32 height() const noexcept;
    [[nodiscard]] u32 depth() const noexcept;
    [[nodiscard]] bool is3D() const noexcept;
    [[nodiscard]] bool is2D() const noexcept;
    [[nodiscard]] bool is1D() const noexcept;
    [[nodiscard]] u32 arraySize() const noexcept;
    [[nodiscard]] u32 mipLevels() const noexcept;
    [[nodiscard]] TinyImageFormat format() const noexcept;
    [[nodiscard]] u32 totalPixelsCount() const noexcept;
    [[nodiscard]] u32 bytesPerPixel() const noexcept;
    [[nodiscard]] u32 pixelPerSlice() const noexcept;
    [[nodiscard]] u32 bytesPerSlice() const noexcept;
    [[nodiscard]] u32 totalBytes() const noexcept;
    [[nodiscard]] bool isCubemap() const noexcept;
    [[nodiscard]] bool isCompressed() const noexcept;
    [[nodiscard]] u32 imageSize(u8 mip) const noexcept;

    // return a referred resource pointer
    [[nodiscard]] const void* rawData(u32 mipLevel) const noexcept;

private:
    TinyDDS_Context* _mDDSContext = nullptr;  // store memory of cpu
    rhi::Buffer* _mpStagingBuffer = nullptr;  // store memory of gpu
};

}  // namespace axe::resource