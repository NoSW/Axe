#include "02Rhi/Vulkan/VulkanAdapter.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include <volk.h>
#include <tiny_imageformat/tinyimageformat_apis.h>

namespace axe::rhi
{
// see https://raw.githubusercontent.com/David-DiGioia/vulkan-diagrams/main/boiler_plate.png for overview of VkPhysicalDevice

bool VulkanAdapter::isDedicatedQueue(VkQueueFlags quFlags) noexcept
{
    const auto allSupportedFalgs = quFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
    return std::has_single_bit(allSupportedFalgs);
}

static const auto query_family_index = [](VkPhysicalDevice pHandle) -> std::tuple<int, int, int>
{
    u32 quFamPropCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(pHandle, &quFamPropCount, nullptr);
    std::pmr::vector<VkQueueFamilyProperties2> quFamProps2(quFamPropCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(pHandle, &quFamPropCount, quFamProps2.data());

    u32 graphicsID = U8_MAX, transferID = U8_MAX, computeID = U8_MAX;
    for (u32 i = 0; i < quFamProps2.size(); ++i)
    {
        if (quFamProps2[i].queueFamilyProperties.queueCount)
        {
            const auto& flags = quFamProps2[i].queueFamilyProperties.queueFlags;
            bool isDedicated  = VulkanAdapter::isDedicatedQueue(flags);
            if (graphicsID == U8_MAX && (flags & VK_QUEUE_GRAPHICS_BIT)) { graphicsID = i; }
            if (transferID == U8_MAX && isDedicated && (flags & VK_QUEUE_TRANSFER_BIT)) { transferID = i; }
            if (computeID == U8_MAX && isDedicated && (flags & VK_QUEUE_COMPUTE_BIT)) { computeID = i; }
        }
    }

    for (u32 i = 0; i < quFamProps2.size(); ++i)
    {
        const auto& flags = quFamProps2[i].queueFamilyProperties.queueFlags;
        if (transferID == U8_MAX && (flags & VK_QUEUE_TRANSFER_BIT)) { transferID = i; }
        if (computeID == U8_MAX && (flags & VK_QUEUE_COMPUTE_BIT)) { computeID = i; }
    }

    return {graphicsID, computeID, transferID};
};

VulkanAdapter::VulkanAdapter(VulkanBackend* backend, VkPhysicalDevice handle, u8 nodeIndex) noexcept
    : _mpBackend(backend), _mpHandle(handle), _mNodeIndex(nodeIndex)
{
    // Get all attrs of gpu
    vkGetPhysicalDeviceMemoryProperties2(_mpHandle, &_mpMemoryProperties);

#if VK_EXT_fragment_shader_interlock
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fsInterlockFeas = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT};
    _mpFeatures.pNext                                                  = &fsInterlockFeas;
#endif
    vkGetPhysicalDeviceFeatures2(_mpHandle, &_mpFeatures);
    _mpFeatures.pNext                                = nullptr;

    VkPhysicalDeviceSubgroupProperties subgroupProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
    _mpProperties.pNext                              = &subgroupProps;
    vkGetPhysicalDeviceProperties2(_mpHandle, &_mpProperties);
    _mpProperties.pNext = nullptr;

    u32 quFamPropCount  = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(_mpHandle, &quFamPropCount, nullptr);
    std::pmr::vector<VkQueueFamilyProperties2> quFamProp2(quFamPropCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(_mpHandle, &quFamPropCount, quFamProp2.data());

    // Collect all the things of interest into structs

    /// GPUCapBits
    for (u32 i = 0; i < TinyImageFormat_Count; ++i)
    {
        VkFormat format = to_vk_enum((TinyImageFormat)i);
        if (format == VK_FORMAT_UNDEFINED || format >= 1'000'000'000) { continue; }  // only query **core** VkFormat enumeration tokens

        VkFormatProperties formatSupport;
        vkGetPhysicalDeviceFormatProperties(_mpHandle, format, &formatSupport);
        _mGPUCapBits.mCanShaderReadFrom[i]      = (formatSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
        _mGPUCapBits.mCanShaderWriteTo[i]       = (formatSupport.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT);
        _mGPUCapBits.mCanRenderTargetWriteTo[i] = (formatSupport.optimalTilingFeatures & (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT));
    }

    /// GPUSettings of interest
    _mpGPUSettings.mUniformBufferAlignment          = _mpProperties.properties.limits.minUniformBufferOffsetAlignment;
    _mpGPUSettings.mUploadBufferTextureAlignment    = _mpProperties.properties.limits.optimalBufferCopyOffsetAlignment;
    _mpGPUSettings.mUploadBufferTextureRowAlignment = _mpProperties.properties.limits.optimalBufferCopyRowPitchAlignment;
    _mpGPUSettings.mMaxVertexInputBindings          = _mpProperties.properties.limits.maxVertexInputBindings;
    _mpGPUSettings.mTimestampPeriod                 = _mpProperties.properties.limits.timestampPeriod;
    _mpGPUSettings.mMultiDrawIndirect               = _mpFeatures.features.multiDrawIndirect;
    _mpGPUSettings.mWaveLaneCount                   = subgroupProps.subgroupSize;

    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_BASIC_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_BASIC_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_VOTE_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_ARITHMETIC_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_BALLOT_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_SHUFFLE_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_SHUFFLE_RELATIVE_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_CLUSTERED_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_CLUSTERED_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_QUAD_BIT;
    if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV)
        _mpGPUSettings.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_PARTITIONED_BIT_NV;

#if VK_EXT_fragment_shader_interlock
    _mpGPUSettings.mROVsSupported = fsInterlockFeas.fragmentShaderPixelInterlock;
#endif
    _mpGPUSettings.mTessellationSupported       = _mpFeatures.features.tessellationShader;
    _mpGPUSettings.mGeometryShaderSupported     = _mpFeatures.features.geometryShader;
    _mpGPUSettings.mGpuVendorPreset.mModelId    = _mpProperties.properties.deviceID;
    _mpGPUSettings.mGpuVendorPreset.mVendorId   = _mpProperties.properties.vendorID;
    _mpGPUSettings.mGpuVendorPreset.mRevisionId = 0x0;  // Vulkan is NOT supported revision ID
    sprintf(_mpGPUSettings.mGpuVendorPreset.mGpuName, _mpProperties.properties.deviceName, CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH);
    u32 major, minor, secondaryBranch, tertiaryBranch;
    switch (_mpProperties.properties.vendorID)
    {
        case GpuVender::GPU_VENDOR_ID_NVIDIA:
            [[likely]] major = (_mpProperties.properties.driverVersion >> 22) & 0x3ff;
            minor            = (_mpProperties.properties.driverVersion >> 14) & 0x0ff;
            secondaryBranch  = (_mpProperties.properties.driverVersion >> 6) & 0x0ff;
            tertiaryBranch   = (_mpProperties.properties.driverVersion) & 0x003f;
            sprintf(_mpGPUSettings.mGpuVendorPreset.mGpuBackendVersion, "%u.%u.%u.%u", major, minor, secondaryBranch, tertiaryBranch);
            break;
        default:
            const auto version = _mpProperties.properties.driverVersion;
            sprintf(_mpGPUSettings.mGpuVendorPreset.mGpuBackendVersion, "%u.%u,%u", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
            break;
    }

    /// queue
    auto [graQuFamId, comQuFamId, transQuFamId] = query_family_index(_mpHandle);
    _mSupportGraphicsQueue                      = graQuFamId != U8_MAX;
    _mSupportComputeQueue                       = comQuFamId != U8_MAX;
    _mSupportTransferQueue                      = transQuFamId != U8_MAX;
    _mHasDedicatedComputeQueue                  = _mSupportComputeQueue && comQuFamId != graQuFamId && comQuFamId != transQuFamId;
    _mHasDedicatedTransferQueue                 = _mSupportTransferQueue && transQuFamId != graQuFamId && transQuFamId != comQuFamId;

    AXE_INFO("GPU[{}], detected. Vendor ID: {:#x}, Model ID: {:#x}, GPU Name: {}, Backend Version: {}",
             _mNodeIndex, _mpGPUSettings.mGpuVendorPreset.mVendorId, _mpGPUSettings.mGpuVendorPreset.mModelId, _mpGPUSettings.mGpuVendorPreset.mGpuName, _mpGPUSettings.mGpuVendorPreset.mGpuBackendVersion);
}

Device* VulkanAdapter::requestDevice(DeviceDesc& desc) noexcept
{
    for (auto& device : _mDevices)
    {
        if (!device)
        {
            device = std::make_unique<VulkanDevice>(this, desc);
            return device.get();
        }
    }
    AXE_ERROR("Failed to request device due to maximum number"
              " of device in adapter limit is exceeded(MAX_NUM_DEVICE_PER_ADAPTER={})",
              MAX_NUM_DEVICE_PER_ADAPTER);
    return nullptr;
}

void VulkanAdapter::releaseDevice(Device*& device) noexcept
{
    AXE_CHECK(device);
    auto iter = _mDevices.begin();
    for (u32 i = 0; i < _mDevices.size(); ++i)
    {
        if (_mDevices[i].get() == static_cast<VulkanDevice*>(device))
        {
            _mDevices[i].reset();
            device = nullptr;
            return;
        }
    }

    AXE_ERROR("Cannot release device that not create from this adapter");
}

bool VulkanAdapter::isSupportQueue(QueueType t) noexcept
{
    switch (t)
    {
        case QUEUE_TYPE_GRAPHICS: return _mSupportGraphicsQueue;
        case QUEUE_TYPE_COMPUTE: return _mSupportComputeQueue;
        case QUEUE_TYPE_TRANSFER: return _mSupportTransferQueue;
        default: AXE_ASSERT(false, "Invalid type"); return false;
    }
}

bool VulkanAdapter::isBetterGpu(const VulkanAdapter& a, const VulkanAdapter& b) noexcept
{
    // prefer one has graphics queue
    bool hasGraphicsQueueA = a._mSupportGraphicsQueue;
    bool hasGraphicsQueueB = b._mSupportGraphicsQueue;
    if (hasGraphicsQueueA && !hasGraphicsQueueB) { return true; }
    if (!hasGraphicsQueueA && hasGraphicsQueueB) { return false; }

    // otherwise, prefer discrete gpu
    auto& propA = a._mpProperties.properties;
    auto& propB = b._mpProperties.properties;
    if (propA.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        propB.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { return true; }

    if (propA.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
        propB.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { return false; }

    // otherwise, prefer one has larger VRAM(video random-access memory)
    if (propA.vendorID == propB.vendorID && propA.deviceID == propB.deviceID)
    {
        const auto getVram = [](const VkPhysicalDeviceMemoryProperties2& memProp)
        {
            u64 ret = 0;
            for (u32 i = 0; i < memProp.memoryProperties.memoryHeapCount; ++i)
            {
                if (memProp.memoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
                    ret += memProp.memoryProperties.memoryHeaps[i].size;
            }
            return ret;
        };
        auto& memPropA = a._mpMemoryProperties;
        auto& memPropB = b._mpMemoryProperties;
        return getVram(memPropA) > getVram(memPropB);
    }

    // otherwise, prefer one has dedicated compute queue for enabling asynchronous compute
    bool hasDedicatedComputeQueueA = a._mSupportComputeQueue && a._mHasDedicatedComputeQueue;
    bool hasDedicatedComputeQueueB = b._mSupportComputeQueue && b._mHasDedicatedComputeQueue;
    if (hasDedicatedComputeQueueA && !hasDedicatedComputeQueueB) { return true; }
    if (!hasDedicatedComputeQueueA && hasDedicatedComputeQueueB) { return false; }

    return true;
};

}  // namespace axe::rhi