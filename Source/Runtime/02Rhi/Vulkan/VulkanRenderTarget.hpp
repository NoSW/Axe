#pragma once
#include "02Rhi/Vulkan/VulkanDriver.hpp"
#include "02Rhi/Vulkan/VulkanSemaphore.hpp"

namespace axe::rhi
{

class VulkanTexture;

class VulkanRenderTarget : public RenderTarget
{
    friend class VulkanDriver;
    AXE_NON_COPYABLE(VulkanRenderTarget);
    VulkanRenderTarget(VulkanDriver* driver) noexcept : _mpDriver(driver) {}
    bool _create(RenderTargetDesc&) noexcept { return true; }
    bool _destroy() noexcept { return true; }

public:
    ~VulkanRenderTarget() noexcept override = default;

private:
    VulkanDriver* const _mpDriver = nullptr;
    VulkanTexture* mTexture       = nullptr;

    VkImageView _mpVkDescriptor   = VK_NULL_HANDLE;
    std::vector<VkImageView> _mpVkSliceDescriptors;
    u32 _mId = 0;

    ClearValue _mClearValue;
    u32 _mArraySize     : 16;
    u32 _mDepth         : 16;
    u32 _mWidth         : 16;
    u32 _mHeight        : 16;
    u32 _mDescriptors   : 20;
    u32 _mMipLevels     : 10;
    u32 _mSampleQuality : 5;
    // TinyImageFormat mFormat;
    MSAASampleCount mSampleCount;
};

}  // namespace axe::rhi