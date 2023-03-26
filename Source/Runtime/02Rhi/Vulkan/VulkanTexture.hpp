#pragma once
#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

class VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;
namespace axe::rhi
{
class VulkanDevice;
class VulkanRenderTarget;
class VulkanCmd;
class VulkanTexture final : public Texture
{
    friend class VulkanDevice;
    friend class VulkanRenderTarget;
    friend class VulkanCmd;
    AXE_NON_COPYABLE(VulkanTexture);
    VulkanTexture(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const TextureDesc&) noexcept;
    bool _destroy() noexcept;

public:
    AXE_PUBLIC ~VulkanTexture() noexcept override = default;

public:
    auto handle() noexcept { return _mpHandle; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_IMAGE; }

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