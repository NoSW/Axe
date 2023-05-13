#pragma once
#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.hxx"
#include "VulkanTexture.hxx"

namespace axe::rhi
{

class VulkanDevice;
class VulkanTexture;
class VulkanCmd;

class VulkanRenderTarget : public RenderTarget
{
    friend class VulkanDevice;
    friend class VulkanCmd;
    AXE_NON_COPYABLE(VulkanRenderTarget);
    VulkanRenderTarget(VulkanDevice* device) noexcept : _mpDevice(device) {}
    bool _create(const RenderTargetDesc&) noexcept;
    bool _destroy() noexcept;

public:
    AXE_PUBLIC ~VulkanRenderTarget() noexcept override { AXE_CHECK(_mpVkDescriptor == VK_NULL_HANDLE); }
    AXE_PUBLIC TinyImageFormat format() const noexcept override { return _mFormat; }
    AXE_PUBLIC MSAASampleCount sampleCount() const noexcept override { return _mSampleCount; }
    AXE_PUBLIC u32 sampleQuality() const noexcept override { return _mSampleQuality; }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VulkanTexture::getVkTypeId(); }

public:
    auto handle() noexcept { return _mpTexture->handle(); }

private:
    VulkanDevice* const _mpDevice = nullptr;
    VulkanTexture* _mpTexture     = nullptr;

    VkImageView _mpVkDescriptor   = VK_NULL_HANDLE;
    std::pmr::vector<VkImageView> _mpVkSliceDescriptors;
    u32 _mId = 0;  // global created-id for this render target

    ClearValue _mClearValue;
    u32 _mArraySize     : 16;
    u32 _mDepth         : 16;
    u32 _mWidth         : 16;
    u32 _mHeight        : 16;
    u32 _mDescriptors   : 20;
    u32 _mMipLevels     : 10;
    u32 _mSampleQuality : 5;
    TinyImageFormat _mFormat;
    MSAASampleCount _mSampleCount;
};

}  // namespace axe::rhi