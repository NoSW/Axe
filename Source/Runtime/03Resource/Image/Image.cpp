#include "03Resource/Image/Image.hpp"

#include "00Core/IO/IO.hpp"
#include "00Core/Memory/Memory.hpp"
#include "00Core/Log/Log.hpp"
#include "00Core/Math/Math.hpp"

#include <tiny_imageformat/tinyimageformat_query.h>

#define TINYDDS_IMPLEMENTATION
#include <tinydds.h>

#include <fstream>

namespace axe::resource
{

ImageDDS::~ImageDDS() noexcept { TinyDDS_DestroyContext(_mDDSContext); }  // convention: who dereferences who checks nullptr

bool ImageDDS::reset(const std::filesystem::path& imageFilePath) noexcept
{
    if (std::filesystem::is_regular_file(imageFilePath))
    {
        std::fstream ifs(imageFilePath, std::ios::binary | std::ios::in);
        if (ifs.is_open()) { return _reset(ifs); }
        else { AXE_ERROR("Failed to open image file: {}", (char*)imageFilePath.c_str()); }
    }
    else
    {
        AXE_ERROR("Failed to read non-existing image file: {}", (char*)imageFilePath.c_str());
    }
    return false;
}

bool ImageDDS::reset(u8* pData, u64 dataBytes) noexcept
{
    std::istringstream iss(std::string((char*)pData, dataBytes));
    return _reset(iss);
}

bool ImageDDS::_reset(std::istream& is) noexcept
{
    constexpr TinyDDS_Callbacks CALL_BACKS{
        .errorFn = [](void* user, char const* msg)
        { AXE_ERROR("Failed to read dds file: {}", msg); },
        .allocFn = [](void* user, size_t size) -> void*
        { return memory::DefaultMemoryResource::get()->new_n(size); },
        .freeFn = [](void* user, void* memory) -> void
        { memory::DefaultMemoryResource::get()->free(memory); },
        .readFn = [](void* user, void* buffer, size_t byteCount) -> size_t
        { return ((std::istream*)user)->read((char*)buffer, byteCount).gcount(); },
        .seekFn = [](void* user, i64 offset) -> bool  // Sets the position of the next character to be extracted from the input stream.
        { std::istream& ret = ((std::istream*)user)->seekg(offset); return !ret.fail() && !ret.eof(); },
        .tellFn = [](void* user) -> i64  // Returns the position of the current character in the input stream.
        { return ((std::istream*)user)->tellg(); },
    };

    auto pContext = TinyDDS_CreateContext(&CALL_BACKS, &is);
    if (!TinyDDS_ReadHeader(pContext))
    {
        TinyDDS_DestroyContext(pContext);
        return false;
    }
    else
    {
        if (_mDDSContext) { TinyDDS_DestroyContext(_mDDSContext); }
        _mDDSContext       = pContext;

        const u32 mipCount = mipLevels();
        for (u32 i = 0; i < mipCount; ++i) { TinyDDS_ImageRawData(pContext, i); }  // alloc memory

        return true;
    }
}
bool ImageDDS::isValid() const noexcept
{
    if (_mDDSContext) { return true; }
    else { AXE_ERROR("ImageDDS is not valid."); }

    return false;
}

u32 ImageDDS::width() const noexcept { return TinyDDS_Width(_mDDSContext); }
u32 ImageDDS::height() const noexcept { return TinyDDS_Height(_mDDSContext); }
u32 ImageDDS::depth() const noexcept { return std::max(1U, TinyDDS_Depth(_mDDSContext)); }
bool ImageDDS::is3D() const noexcept { return TinyDDS_Is3D(_mDDSContext); }
bool ImageDDS::is2D() const noexcept { return TinyDDS_Is2D(_mDDSContext); }
bool ImageDDS::is1D() const noexcept { return TinyDDS_Is1D(_mDDSContext); }
u32 ImageDDS::arraySize() const noexcept { return std::max(1U, TinyDDS_ArraySlices(_mDDSContext)); }
u32 ImageDDS::mipLevels() const noexcept { return std::max(1U, TinyDDS_NumberOfMipmaps(_mDDSContext)); }
TinyImageFormat ImageDDS::format() const noexcept { return TinyImageFormat_FromTinyDDSFormat(TinyDDS_GetFormat(_mDDSContext)); }
u32 ImageDDS::totalPixelsCount() const noexcept { return width() * height() * depth() * arraySize(); }
u32 ImageDDS::bytesPerPixel() const noexcept { return TinyDDS_FormatSize(TinyDDS_GetFormat(_mDDSContext)); }
u32 ImageDDS::pixelPerSlice() const noexcept { return width() * height() * depth(); }
u32 ImageDDS::bytesPerSlice() const noexcept { return pixelPerSlice() * bytesPerPixel(); }
u32 ImageDDS::totalBytes() const noexcept { return bytesPerSlice() * arraySize(); }
bool ImageDDS::isCubemap() const noexcept { return TinyDDS_IsCubemap(_mDDSContext); }
bool ImageDDS::isCompressed() const noexcept { return TinyDDS_IsCompressed(TinyDDS_GetFormat(_mDDSContext)); }
void const* ImageDDS::rawData(u32 mipLevel) const noexcept { return TinyDDS_ImageRawData(_mDDSContext, mipLevel); }
u32 ImageDDS::imageSize(u8 mip) const noexcept { return TinyDDS_ImageSize(_mDDSContext, mip); }

}  // namespace axe::resource