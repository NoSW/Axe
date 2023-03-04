#pragma once
#include "02Rhi/Vulkan/VulkanDevice.hpp"

namespace axe::rhi
{
class VmaAllocation_T;

class VulkanTexture : public Texture
{
    friend class VulkanDevice;
    AXE_NON_COPYABLE(VulkanTexture);
    VulkanTexture(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(TextureDesc&) noexcept { return true; }
    bool _destroy() noexcept { return true; }

public:
    ~VulkanTexture() noexcept override = default;

private:
    VulkanDevice* const _mpDevice = nullptr;
    VkImageView _mpVkSRVDescriptor;
    VkImageView _mpVkSRVStencilDescriptor;
    std::vector<VkImageView> _mpVkUAVDescriptors;
    VkImage _mpVkImage;
    union
    {
        VmaAllocation_T* _mpVkAllocation;
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
    u32 _mNodeIndex         : 4  = 0;
    u32 _mSampleCount       : 5  = 1;
    u32 _mUav               : 1  = 0;
    u32 _mOwnsImage         : 1  = 0;  // will be false if the underlying resource is not owned by the texture (swapchain textures,...)
    u32 _mLazilyAllocated   : 1  = 0;
};

}  // namespace axe::rhi