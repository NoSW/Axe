#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include <volk/volk.h>

#include "02Rhi/Vulkan/VulkanWantedActivate.hpp"
namespace axe::rhi
{

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

    vkDeviceWaitIdle(_mpHandle);  // block until all processing on all queues of a given device is finished

    // if (mpImageAvailableSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(mpVkDevice, mpImageAvailableSemaphore, nullptr); }
    // if (mpRenderingFinishedSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(mpVkDevice, mpRenderingFinishedSemaphore, nullptr); }
    // if (mpVkSwapChain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(mpVkDevice, mpVkSwapChain, nullptr); }

    vkDestroyDevice(_mpHandle, nullptr);  // all queues associated with it are destroyed automatically
}

void VulkanDevice::findQueueFamilyIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) noexcept
{
    bool found    = false;
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
            quIndex = dedicatedQueueInfo.mUsedCount++;
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
                    quFamIndex = info.mFamilyIndex;
                    quIndex    = dedicatedQueueInfo.mUsedCount++;
                    outFlag    = flag;
                    found      = true;
                    AXE_DEBUG("Not found a dedicated queue of {}, using a no-dedicated one(Flags={})",
                              reflection::enum_name(quType), flag)
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
}

}  // namespace axe::rhi