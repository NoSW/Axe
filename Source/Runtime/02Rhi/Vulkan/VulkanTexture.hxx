#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.hxx"

class VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;
namespace axe::rhi
{
class VulkanDevice;
class VulkanRenderTarget;
class VulkanCmd;
class VulkanDescriptorSet;
class VulkanTexture final : public Texture
{
    friend class VulkanDevice;
    friend class VulkanRenderTarget;
    friend class VulkanCmd;
    friend class VulkanDescriptorSet;
    AXE_NON_COPYABLE(VulkanTexture);
    VulkanTexture(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const TextureDesc&) noexcept;
    bool _destroy() noexcept;

public:
    ~VulkanTexture() noexcept override = default;
    bool update(TextureUpdateDesc&) noexcept override;

public:
    AXE_PRIVATE auto handle() noexcept { return _mpHandle; }
    AXE_PRIVATE u32 width() const noexcept { return _mWidth; }
    AXE_PRIVATE u32 height() const noexcept { return _mHeight; }
    AXE_PRIVATE u32 depth() const noexcept { return _mDepth; }
    AXE_PRIVATE u32 mipLevels() const noexcept { return _mMipLevels; }
    AXE_PRIVATE u32 arraySize() const noexcept { return _mArraySizeMinusOne + 1; }
    AXE_PRIVATE auto format() const noexcept { return (TinyImageFormat)_mFormat; }
    AXE_PRIVATE u32 aspectMask() const noexcept { return _mAspectMask; }

public:
    constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_IMAGE;

private:
    VulkanDevice* const _mpDevice         = nullptr;
    VkImage _mpHandle                     = VK_NULL_HANDLE;
    VkImageView _mpVkSRVDescriptor        = VK_NULL_HANDLE;
    VkImageView _mpVkSRVStencilDescriptor = VK_NULL_HANDLE;
    std::pmr::vector<VkImageView> _mpVkUAVDescriptors;
    union
    {
        VmaAllocation _mpVkAllocation;
        VkDeviceMemory _mpVkDeviceMemory;
    };
    // VirtualTexture* pSvt;

    // Current state of the buffer
    u32 _mWidth             : 16 = 0;
    u32 _mHeight            : 16 = 0;
    u32 _mDepth             : 16 = 0;
    u32 _mMipLevels         : 5  = 0;
    u32 _mArraySizeMinusOne : 11 = 0;
    u32 _mFormat            : 8  = 0;
    u32 _mAspectMask        : 4  = 0;  // specifying which aspects (COLOR,DEPTH,STENCIL) are included in the pVkImageView
    u32 _mSampleCount       : 5  = 1;
    u32 _mUav               : 1  = 0;
    u32 _mOwnsImage         : 1  = 0;  // will be false if the underlying resource is not owned by the texture (swapchain textures,...)
    u32 _mLazilyAllocated   : 1  = 0;
};

}  // namespace axe::rhi