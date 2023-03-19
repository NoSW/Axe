#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include <volk/volk.h>

#include "02Rhi/Vulkan/VulkanWantedActivate.hpp"

#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

namespace axe::rhi
{

#if !(VMA_VULKAN_VERSION >= 1003000)
#error "need vulkan>=1.3"
#endif

bool VulkanDevice::_initVMA() noexcept
{
    VmaVulkanFunctions vmaVkFunctions{
        .vkGetInstanceProcAddr                   = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr                     = vkGetDeviceProcAddr,
        .vkGetPhysicalDeviceProperties           = vkGetPhysicalDeviceProperties,
        .vkGetPhysicalDeviceMemoryProperties     = vkGetPhysicalDeviceMemoryProperties,
        .vkAllocateMemory                        = vkAllocateMemory,
        .vkFreeMemory                            = vkFreeMemory,
        .vkMapMemory                             = vkMapMemory,
        .vkUnmapMemory                           = vkUnmapMemory,
        .vkFlushMappedMemoryRanges               = vkFlushMappedMemoryRanges,
        .vkInvalidateMappedMemoryRanges          = vkInvalidateMappedMemoryRanges,
        .vkBindBufferMemory                      = vkBindBufferMemory,
        .vkBindImageMemory                       = vkBindImageMemory,
        .vkGetBufferMemoryRequirements           = vkGetBufferMemoryRequirements,
        .vkGetImageMemoryRequirements            = vkGetImageMemoryRequirements,
        .vkCreateBuffer                          = vkCreateBuffer,
        .vkDestroyBuffer                         = vkDestroyBuffer,
        .vkCreateImage                           = vkCreateImage,
        .vkDestroyImage                          = vkDestroyImage,
        .vkCmdCopyBuffer                         = vkCmdCopyBuffer,
        .vkGetBufferMemoryRequirements2KHR       = vkGetBufferMemoryRequirements2,
        .vkGetImageMemoryRequirements2KHR        = vkGetImageMemoryRequirements2,
        .vkBindBufferMemory2KHR                  = vkBindBufferMemory2,
        .vkBindImageMemory2KHR                   = vkBindImageMemory2,
        .vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR,
        .vkGetDeviceBufferMemoryRequirements     = vkGetDeviceBufferMemoryRequirements,
        .vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements

    };

    VmaAllocatorCreateInfo vmaCreateInfo = {
        .flags                       = 0,  // to be continue
        .physicalDevice              = _mpAdapter->handle(),
        .device                      = _mpHandle,
        .preferredLargeHeapBlockSize = 0,  // default value
        .pAllocationCallbacks        = &g_VkAllocationCallbacks,
        .pDeviceMemoryCallbacks      = nullptr,
        .pHeapSizeLimit              = nullptr,
        .pVulkanFunctions            = &vmaVkFunctions,
        .instance                    = _mpAdapter->backendHandle(),
        .vulkanApiVersion            = VK_API_VERSION_1_3,
#if VMA_EXTERNAL_MEMORY
        .pTypeExternalMemoryHandleTypes = nullptr,
#endif
    };
    if (_mDedicatedAllocationExtension) { vmaCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT; }
    if (_mBufferDeviceAddressExtension) { vmaCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; }
    return VK_SUCCEEDED(vmaCreateAllocator(&vmaCreateInfo, &_mpVmaAllocator));
}

VulkanDevice::VulkanDevice(VulkanAdapter* pAdapter, DeviceDesc& desc) noexcept
    : _mpAdapter(pAdapter), _mShaderModel(desc.mShaderModel)
{
    u32 layerCount = 0;
    vkEnumerateDeviceLayerProperties(pAdapter->handle(), &layerCount, nullptr);
    std::pmr::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateDeviceLayerProperties(pAdapter->handle(), &layerCount, layers.data());
    std::pmr::vector<const char*> layerNames(layerCount, nullptr);
    for (u32 i = 0; i < layers.size(); ++i) { layerNames[i] = layers[i].layerName; }
    // if (strcmp(layerNames[i], "VK_LAYER_RENDERDOC_Capture") == 0) { true; }

    u32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(pAdapter->handle(), nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(pAdapter->handle(), nullptr, &extensionCount, extensions.data());
    std::pmr::vector<const char*> extensionNames(extensionCount, nullptr);
    for (u32 i = 0; i < extensions.size(); ++i) { extensionNames[i] = extensions[i].extensionName; }

    filter_unsupported(layerNames, gWantedDeviceLayers);
    filter_unsupported(extensionNames, gWantedDeviceExtensions);

    const auto sup = [&extensionNames](const char* extName)
    {
        bool ret       = std::find_if(gWantedDeviceExtensions.begin(), gWantedDeviceExtensions.end(), [extName](const char* ext)
                                      { return !strcmp(extName, ext); }) != gWantedDeviceExtensions.end();
        bool supported = std::find_if(extensionNames.begin(), extensionNames.end(), [extName](const char* ext)
                                      { return !strcmp(extName, ext); }) != extensionNames.end();
        if (!ret && supported) { AXE_ERROR("{} is supported but added to wanted", extName); }
        return ret;
    };

    _mDedicatedAllocationExtension = sup(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME) &&
                                     sup(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    _mBufferDeviceAddressExtension = sup(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
    _mRaytracingSupported          = sup(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME) &&
                            sup(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
                            sup(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) &&
                            sup(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) &&
                            sup(VK_KHR_SPIRV_1_4_EXTENSION_NAME) &&
                            sup(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

#ifdef VK_USE_PLATFORM_WIN32_KHR
    _mExternalMemoryExtension = sup(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME) &&
                                sup(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
    if (_mExternalMemoryExtension)
    {
        AXE_INFO("Successfully loaded External Memory extension");
    }
#endif

    // queue family properties
    u32 quFamPropCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(pAdapter->handle(), &quFamPropCount, nullptr);
    std::vector<VkQueueFamilyProperties2> quFamProp2(quFamPropCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(pAdapter->handle(), &quFamPropCount, quFamProp2.data());

    constexpr u32 MAX_QUEUE_FAMILIES = 16, MAX_QUEUE_COUNT = 64;
    std::array<std::array<float, MAX_QUEUE_COUNT>, MAX_QUEUE_FAMILIES> quFamPriorities{1.0f};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (u32 i = 0; i < quFamProp2.size(); ++i)
    {
        u32 quCount = quFamProp2[i].queueFamilyProperties.queueCount;
        if (quCount > 0)
        {
            quCount              = desc.mRequestAllAvailableQueues ? quCount : 1;
            u32 queueFamilyFlags = quFamProp2[i].queueFamilyProperties.queueFlags;
            if (quCount > MAX_QUEUE_COUNT)
            {
                AXE_WARN("{} queues queried from device are too many in queue family index {} with flag {} (support up to {})",
                         quCount, i, (u32)queueFamilyFlags, MAX_QUEUE_COUNT);
                quCount = MAX_QUEUE_COUNT;
            }

            queueCreateInfos.emplace_back(VkDeviceQueueCreateInfo{
                .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext            = nullptr,
                .flags            = 0,
                .queueFamilyIndex = i,
                .queueCount       = quCount,
                .pQueuePriorities = quFamPriorities[i].data(),
            });
            AXE_INFO("Ready to be create: FamilyIndex[{}](total count is {}), requested queueCount is {} (total count is {}) (No specified priorities)",
                     i, quFamProp2.size(), quCount, quFamProp2[i].queueFamilyProperties.queueCount);

            _mQueueInfos[queueFamilyFlags] = QueueInfo{
                .mAvailableCount = quCount, .mUsedCount = 0, .mFamilyIndex = (u8)i};
        }
    }

    for (u8 i = 0; i < QUEUE_TYPE_COUNT; ++i)
    {
        u8 dontCareOutput1, dontCareOutput2;
        queryAvailableQueueIndex((QueueType)i, _mQueueFamilyIndexes[i], dontCareOutput1, dontCareOutput2);
    }

    VkPhysicalDeviceVulkan13Features deviceFeatures13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr};
    deviceFeatures13.dynamicRendering   = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &deviceFeatures13,
        .flags                   = 0,
        .queueCreateInfoCount    = (u32)queueCreateInfos.size(),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledLayerCount       = (u32)gWantedDeviceLayers.size(),
        .ppEnabledLayerNames     = gWantedDeviceLayers.data(),
        .enabledExtensionCount   = (u32)gWantedDeviceExtensions.size(),
        .ppEnabledExtensionNames = gWantedDeviceExtensions.data(),
        .pEnabledFeatures        = &pAdapter->features()->features};

    auto result = vkCreateDevice(pAdapter->handle(), &deviceCreateInfo, nullptr, &_mpHandle);
    AXE_ASSERT(VK_SUCCEEDED(result));
    volkLoadDevice(_mpHandle);
}

VulkanDevice::~VulkanDevice() noexcept
{
    AXE_ASSERT(_mpHandle);
    vmaDestroyAllocator(_mpVmaAllocator);

    vkDeviceWaitIdle(_mpHandle);  // block until all processing on all queues of a given device is finished

    // if (mpImageAvailableSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(mpVkDevice, mpImageAvailableSemaphore, nullptr); }
    // if (mpRenderingFinishedSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(mpVkDevice, mpRenderingFinishedSemaphore, nullptr); }
    // if (mpVkSwapChain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(mpVkDevice, mpVkSwapChain, nullptr); }

    vkDestroyDevice(_mpHandle, nullptr);  // all queues associated with it are destroyed automatically
}

void VulkanDevice::requestQueueIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) noexcept
{
    auto [bitFlag, isConsume] = queryAvailableQueueIndex(quType, outQuFamIndex, outQuIndex, outFlag);
    if (isConsume) { _mQueueInfos[bitFlag].mUsedCount++; }
}

std::pair<u8, bool> VulkanDevice::queryAvailableQueueIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) const noexcept
{
    bool found                   = false;
    bool willConsumeDedicatedOne = false;
    u8 quFamIndex = U8_MAX, quIndex = U8_MAX;
    auto requiredFlagBit = VK_QUEUE_FLAG_BITS_MAX_ENUM;
    switch (quType)
    {
        case QUEUE_TYPE_GRAPHICS: requiredFlagBit = VK_QUEUE_GRAPHICS_BIT; break;
        case QUEUE_TYPE_TRANSFER: requiredFlagBit = VK_QUEUE_TRANSFER_BIT; break;
        case QUEUE_TYPE_COMPUTE: requiredFlagBit = VK_QUEUE_COMPUTE_BIT; break;
        default: AXE_ERROR("Unsupported queue type {}", reflection::enum_name(quType)); break;
    }

    u32 minQueueFlag         = U32_MAX;

    // Try to find a dedicated queue of this type
    auto& dedicatedQueueInfo = _mQueueInfos[requiredFlagBit];
    if (dedicatedQueueInfo.mUsedCount < dedicatedQueueInfo.mAvailableCount)
    {
        found      = true;
        quFamIndex = dedicatedQueueInfo.mFamilyIndex;
        outFlag    = requiredFlagBit;
        found      = true;

        if (requiredFlagBit & VK_QUEUE_GRAPHICS_BIT)
        {
            quIndex = 0;  // always return same one if graphics queue
        }
        else
        {
            quIndex                 = dedicatedQueueInfo.mUsedCount;
            willConsumeDedicatedOne = true;
        }
    }
    else
    {
        // Try to find a non-dedicated queue of this type if NOT provide a dedicated one.
        for (u32 flag = 0; flag < _mQueueInfos.size(); ++flag)
        {
            auto& info = _mQueueInfos[flag];
            if (info.mFamilyIndex == 0) { outFlag = flag; }  // default flag
            if ((requiredFlagBit & flag) == requiredFlagBit)
            {
                if (info.mUsedCount < info.mAvailableCount)
                {
                    quFamIndex              = info.mFamilyIndex;
                    quIndex                 = dedicatedQueueInfo.mUsedCount;
                    willConsumeDedicatedOne = true;
                    outFlag                 = flag;
                    found                   = true;
                    AXE_DEBUG("Not found a dedicated queue of {}, using a no-dedicated one(Flags={})", reflection::enum_name(quType), flag)
                    break;
                }
            }
        }
    }

    // Choose default queue if all tries fail.
    if (!found)
    {
        found      = true;
        quFamIndex = 0;
        quIndex    = 0;
        AXE_DEBUG("Not found queue of {}, using default one(familyIndex=0.queueIndex=0)", reflection::enum_name(quType));
    }
    outQuFamIndex = quFamIndex;
    outQuIndex    = quIndex;
    return {requiredFlagBit, willConsumeDedicatedOne};
}

void VulkanDevice::_setDebugLabel(void* handle, VkObjectType type, std::string_view name) noexcept
{
#if AXE_RHI_VULKAN_ENABLE_DEBUG
    if (type != VK_OBJECT_TYPE_UNKNOWN && !name.empty() && handle != nullptr)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{
            .sType        = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
            .objectType   = type,
            .objectHandle = (u64)handle,
            .pObjectName  = name.data()};
        vkSetDebugUtilsObjectNameEXT(_mpHandle, &nameInfo);
    }
#endif
}

}  // namespace axe::rhi