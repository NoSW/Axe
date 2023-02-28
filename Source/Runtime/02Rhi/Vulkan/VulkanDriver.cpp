#include "02Rhi/Vulkan/VulkanDriver.hpp"

#include <00Core/IO/IO.hpp>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

//#define VMA_IMPLEMENTATION
//#include <vma/vk_mem_alloc.h>

#include "VulkanWantedActivate.hpp"

namespace axe::rhi
{

bool VulkanDriver::init(DriverDesc& desc) noexcept
{
    mpDriverDesc = &desc;
    mGpuMode     = desc.mGpuMode;
#if AXE_02RHI_VULKAN_USE_DISPATCH_TABLE
    // TODO: using device table
#else
    // Attempt to load Vulkan loader from the system
    if (VK_FAILED(volkInitialize())) { return false; }
#endif

    if (!_createInstance()) return false;

    mOwnInstance = true;

    if (!_addDevice()) { return false; }
    if (!_initVulkanMemoryAllocator()) { return false; }

    return true;
}

void VulkanDriver::exit() noexcept
{
    if (mpVkDevice != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(mpVkDevice);  // block until all processing on all queues of a given device is finished

        if (mpImageAvailableSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(mpVkDevice, mpImageAvailableSemaphore, nullptr); }
        if (mpRenderingFinishedSemaphore != VK_NULL_HANDLE) { vkDestroySemaphore(mpVkDevice, mpRenderingFinishedSemaphore, nullptr); }
        if (mpVkSwapChain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(mpVkDevice, mpVkSwapChain, nullptr); }

        vkDestroyDevice(mpVkDevice, nullptr);  // all queues associated with it
                                               // are destroyed automatically
    }

    if (mpVkInstance != VK_NULL_HANDLE)
    {
        if (mpVkSurface != VK_NULL_HANDLE) { vkDestroySurfaceKHR(mpVkInstance, mpVkSurface, nullptr); }
        if (mpVkDebugUtilsMessenger) { vkDestroyDebugUtilsMessengerEXT(mpVkInstance, mpVkDebugUtilsMessenger, nullptr); }
        if (mOwnInstance) { vkDestroyInstance(mpVkInstance, nullptr); }
    }

    // vmaDestroyAllocator(mpVmaAllocator);
}

#if AXE_RHI_VULKAN_ENABLE_DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        AXE_ERROR("{}: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        AXE_WARN("{}: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        AXE_INFO("{}: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    else
    {
        AXE_DEBUG("{}: {}", pCallbackData->pMessageIdName, pCallbackData->pMessage);
    }
    return VK_FALSE;
}
#endif

bool VulkanDriver::_createInstance() noexcept
{
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    std::vector<const char*> layerNames;

    layerNames.reserve(layerCount);
    for (const auto& i : layers) { layerNames.push_back(i.layerName); }

    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::vector<const char*> extensionNames;

    extensionNames.reserve(extensionCount);
    for (const auto& i : extensions) { extensionNames.push_back(i.extensionName); }

    filter_unsupported(layerNames, gsWantedInstanceLayers);
    filter_unsupported(extensionNames, gsWantedInstanceExtensions);

    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = mpDriverDesc->m_appName,
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName        = AXE_ENGINE_NAME,
        .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion         = AXE_02RHI_TARGET_VULKAN_VERSION};

    VkInstanceCreateInfo createInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = (u32)gsWantedInstanceLayers.size(),
        .ppEnabledLayerNames     = gsWantedInstanceLayers.data(),
        .enabledExtensionCount   = (u32)gsWantedInstanceExtensions.size(),
        .ppEnabledExtensionNames = gsWantedInstanceExtensions.data()};

    auto result = vkCreateInstance(&createInfo, nullptr, &mpVkInstance);
    AXE_CHECK(VK_SUCCEEDED(result));

    // load all required Vulkan entrypoints, including all extensions
    volkLoadInstanceOnly(mpVkInstance);

#if AXE_RHI_VULKAN_ENABLE_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo{
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = nullptr,
        .flags           = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData       = nullptr};
    AXE_CHECK(VK_SUCCEEDED(vkCreateDebugUtilsMessengerEXT(mpVkInstance, &debugUtilsCreateInfo, nullptr, &mpVkDebugUtilsMessenger)));
#endif

    return mpVkInstance != VK_NULL_HANDLE;
}

bool VulkanDriver::_selectedBestDevice() noexcept
{
    // helper functions
    const auto collectDeviceInfo =
        [](const u32 i, const VkPhysicalDevice device, VkPhysicalDeviceMemoryProperties2& memProp2, VkPhysicalDeviceFeatures2& fea2,
           VkPhysicalDeviceProperties2& prop2, std::vector<VkQueueFamilyProperties2>& quFamProp2, GPUSettings& setting)
    {
        // Get all attrs of gpu
        vkGetPhysicalDeviceMemoryProperties2(device, &memProp2);

#if VK_EXT_fragment_shader_interlock
        VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fsInterlockFeas = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT};
        fea2.pNext                                                         = &fsInterlockFeas;
#endif
        vkGetPhysicalDeviceFeatures2(device, &fea2);
        fea2.pNext                                       = nullptr;

        VkPhysicalDeviceSubgroupProperties subgroupProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES};
        prop2.pNext                                      = &subgroupProps;
        vkGetPhysicalDeviceProperties2(device, &prop2);
        prop2.pNext        = nullptr;

        u32 quFamPropCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(device, &quFamPropCount, nullptr);
        quFamProp2.resize(quFamPropCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
        vkGetPhysicalDeviceQueueFamilyProperties2(device, &quFamPropCount, quFamProp2.data());

        // Collect all the things of interest into a struct
        setting.mUniformBufferAlignment          = prop2.properties.limits.minUniformBufferOffsetAlignment;
        setting.mUploadBufferTextureAlignment    = prop2.properties.limits.optimalBufferCopyOffsetAlignment;
        setting.mUploadBufferTextureRowAlignment = prop2.properties.limits.optimalBufferCopyRowPitchAlignment;
        setting.mMaxVertexInputBindings          = prop2.properties.limits.maxVertexInputBindings;
        setting.mMultiDrawIndirect               = fea2.features.multiDrawIndirect;
        setting.mWaveLaneCount                   = subgroupProps.subgroupSize;

        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_BASIC_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_BASIC_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_VOTE_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_ARITHMETIC_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_BALLOT_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_SHUFFLE_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_RELATIVE_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_SHUFFLE_RELATIVE_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_CLUSTERED_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_CLUSTERED_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_QUAD_BIT;
        if (subgroupProps.supportedOperations & VK_SUBGROUP_FEATURE_PARTITIONED_BIT_NV)
            setting.mWaveOpsSupportFlags |= WaveOpsSupportFlag::WAVE_OPS_SUPPORT_FLAG_PARTITIONED_BIT_NV;

#if VK_EXT_fragment_shader_interlock
        setting.mROVsSupported = fsInterlockFeas.fragmentShaderPixelInterlock;
#endif
        setting.mTessellationSupported     = fea2.features.tessellationShader;
        setting.mGeometryShaderSupported   = fea2.features.geometryShader;
        setting.mGpuVendorPreset.mModelId  = prop2.properties.deviceID;
        setting.mGpuVendorPreset.mVendorId = prop2.properties.vendorID;
        setting.mGpuVendorPreset.mRevisionId =
            0x0;  // Vulkan is NOT supported revision ID
        sprintf(setting.mGpuVendorPreset.mGpuName, prop2.properties.deviceName, CapabilityLevel::MAX_GPU_VENDOR_STRING_LENGTH);
        u32 major, minor, secondaryBranch, tertiaryBranch;
        switch (prop2.properties.vendorID)
        {
            case GpuVender::GPU_VENDOR_ID_NVIDIA:
                [[likely]] major = (prop2.properties.driverVersion >> 22) & 0x3ff;
                minor            = (prop2.properties.driverVersion >> 14) & 0x0ff;
                secondaryBranch  = (prop2.properties.driverVersion >> 6) & 0x0ff;
                tertiaryBranch   = (prop2.properties.driverVersion) & 0x003f;
                sprintf(setting.mGpuVendorPreset.mGpuDriverVersion, "%u.%u.%u.%u", major, minor, secondaryBranch, tertiaryBranch);
                break;
            default:
                const auto version = prop2.properties.driverVersion;
                sprintf(setting.mGpuVendorPreset.mGpuDriverVersion, "%u.%u,%u", VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));
                break;
        }
        AXE_INFO("GPU[{}], detected. Vendor ID: {:#x}, Model ID: {:#x}, GPU Name: {}, Driver Version: {}",
                 i, setting.mGpuVendorPreset.mVendorId, setting.mGpuVendorPreset.mModelId, setting.mGpuVendorPreset.mGpuName, setting.mGpuVendorPreset.mGpuDriverVersion);
    };

    const auto isBetterDevice =
        [](const VkPhysicalDeviceMemoryProperties2& bestMemProp, const VkPhysicalDeviceProperties2& bestProp,
           const VkPhysicalDeviceMemoryProperties2& testMemProp, const VkPhysicalDeviceProperties2& testProp)
    {
        if (testProp.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            bestProp.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { return true; }

        if (testProp.properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            bestProp.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) { return false; }

        if (testProp.properties.vendorID == bestProp.properties.vendorID &&
            testProp.properties.deviceID == bestProp.properties.deviceID)
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
            return getVram(testMemProp) > getVram(bestMemProp);
        }
        return false;
    };

    const auto haveGraphicsQueueFamily = [](const std::vector<VkQueueFamilyProperties2>& quFamProps2) -> std::tuple<int, int, int>
    {
        u32 graphicsID = U8_MAX, transferID = U8_MAX, computeID = U8_MAX;
        for (u32 i = 0; i < quFamProps2.size(); ++i)
            if (quFamProps2[i].queueFamilyProperties.queueCount)
            {
                const auto& flags = quFamProps2[i].queueFamilyProperties.queueFlags;
                if (flags & VK_QUEUE_GRAPHICS_BIT) { graphicsID = i; }
                if (flags & VK_QUEUE_TRANSFER_BIT) { transferID = i; }
                if (flags & VK_QUEUE_COMPUTE_BIT) { computeID = i; }
            }

        return {graphicsID, transferID, computeID};
    };

    // select a best gpu: choose discrete gpu first instead of integrated one, otherwise prefer the gpu with bigger VRAM size
    u32 deviceCount = 0;
    auto result     = vkEnumeratePhysicalDevices(mpVkInstance, &deviceCount, nullptr);
    AXE_ASSERT(VK_SUCCEEDED(result));
    AXE_ASSERT(deviceCount > 0);
    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(mpVkInstance, &deviceCount, devices.data());
    AXE_ASSERT(VK_SUCCEEDED(result));

    std::vector<VkPhysicalDeviceProperties2> props2(deviceCount, {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2});
    std::vector<VkPhysicalDeviceMemoryProperties2> memProps2(deviceCount, {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2});
    std::vector<VkPhysicalDeviceFeatures2> feas2(deviceCount, {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2});
    std::vector<std::vector<VkQueueFamilyProperties2> > quFamProps2(deviceCount);
    std::vector<GPUSettings> gpuSettings(deviceCount);
    u32 bestDeviceIndex = U32_MAX;
    for (u32 i = 0; i < deviceCount; ++i)
    {
        collectDeviceInfo(i, devices[i], memProps2[i], feas2[i], props2[i], quFamProps2[i], gpuSettings[i]);
        auto [graphicsID, transferID, computeID] = haveGraphicsQueueFamily(quFamProps2[i]);
        if (bestDeviceIndex == UINT32_MAX ||
            ((isBetterDevice(memProps2[bestDeviceIndex], props2[bestDeviceIndex], memProps2[i], props2[i])) && graphicsID != U8_MAX))
        {
            bestDeviceIndex           = i;
            mGraphicsQueueFamilyIndex = graphicsID;
            mTransferQueueFamilyIndex = transferID;
            mComputeQueueFamilyIndex  = computeID;
        }
    }

    if (bestDeviceIndex == U32_MAX)
    {
        AXE_ERROR("No available device");
        return false;
    }

    if (props2[bestDeviceIndex].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
    {
        AXE_ERROR("The only available GPU is type of CPU");
        return false;
    }

    mpVkActiveGPUProperties = new VkPhysicalDeviceProperties2{props2[bestDeviceIndex]};
    mpVkActiveGPUFeatures   = new VkPhysicalDeviceFeatures2{feas2[bestDeviceIndex]};
    mActiveGpuSettings      = gpuSettings[bestDeviceIndex];
    mpVkActiveGPU           = devices[bestDeviceIndex];
    AXE_INFO("Selected device[{}], Name: {}, Vendor Id: {:#x}, Model Id {:#x}, Driver Version: {}",
             bestDeviceIndex, mActiveGpuSettings.mGpuVendorPreset.mGpuName, mActiveGpuSettings.mGpuVendorPreset.mVendorId,
             mActiveGpuSettings.mGpuVendorPreset.mModelId, mActiveGpuSettings.mGpuVendorPreset.mGpuDriverVersion);

    return true;
}

bool VulkanDriver::_addDevice() noexcept
{
    mDeviceGroupCreationExtension = std::find_if(gsWantedInstanceExtensions.begin(), gsWantedInstanceExtensions.end(), [](const char* n)
                                                 { return strcmp(n, VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME) == 0; }) != gsWantedInstanceExtensions.end();
#if VK_KHR_device_group_creation
    VkDeviceGroupDeviceCreateInfo deviceGroupInfo = {.sType = VK_STRUCTURE_TYPE_DEVICE_GROUP_DEVICE_CREATE_INFO};
    std::vector<VkPhysicalDeviceGroupProperties> gpuGroupProps;
    mLinkedNodeCount = 1;
    if (mGpuMode == GPU_MODE_LINKED && mDeviceGroupCreationExtension)
    {
        u32 deviceGroupCount = 0;
        vkEnumeratePhysicalDeviceGroups(mpVkInstance, &deviceGroupCount, nullptr);
        AXE_CHECK(deviceGroupCount <= MAX_LINKED_GPUS);
        gpuGroupProps.resize(deviceGroupCount, VkPhysicalDeviceGroupProperties{
                                                   .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES_KHR, .pNext = nullptr});
        vkEnumeratePhysicalDeviceGroups(mpVkInstance, &deviceGroupCount, gpuGroupProps.data());

        for (const auto& prop : gpuGroupProps)
        {
            if (prop.physicalDeviceCount > 1)
            {  // using first device group whose count > 1, to create a logical device using all of the physical devices.
                mLinkedNodeCount                    = prop.physicalDeviceCount;
                deviceGroupInfo.physicalDeviceCount = prop.physicalDeviceCount;
                deviceGroupInfo.pPhysicalDevices    = prop.physicalDevices;
                break;
            }
        }
    }
#endif

    if (mLinkedNodeCount <= 1 && mGpuMode == GPU_MODE_LINKED)
    {
        mGpuMode = GPU_MODE_SINGLE;
    }

    constexpr bool useContext = false;
    if (useContext)
    {  // TODO
    }
    else
    {
        bool succ = _selectedBestDevice();
        AXE_ASSERT(succ);
    }

    u32 layerCount = 0;
    vkEnumerateDeviceLayerProperties(mpVkActiveGPU, &layerCount, nullptr);
    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateDeviceLayerProperties(mpVkActiveGPU, &layerCount, layers.data());
    std::vector<const char*> layerNames;
    layerNames.reserve(layerCount);
    for (const auto& i : layers)
    {
        layerNames.push_back(i.layerName);
        if (strcmp(i.layerName, "VK_LAYER_RENDERDOC_Capture") == 0) { mRenderDocLayerEnabled = true; }
    }

    u32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(mpVkActiveGPU, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(mpVkActiveGPU, nullptr, &extensionCount, extensions.data());
    std::vector<const char*> extensionNames;
    extensionNames.reserve(extensionCount);
    for (const auto& i : extensions) { extensionNames.push_back(i.extensionName); }

    filter_unsupported(layerNames, gWantedDeviceLayers);
    filter_unsupported(extensionNames, gWantedDeviceExtensions);

    // queue family properties
    u32 quFamPropCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(mpVkActiveGPU, &quFamPropCount, nullptr);
    std::vector<VkQueueFamilyProperties2> quFamProp2(quFamPropCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(mpVkActiveGPU, &quFamPropCount, quFamProp2.data());

    constexpr u32 MAX_QUEUE_FLAG = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                                   VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT | VK_QUEUE_PROTECTED_BIT;
    mAvailableQueueCounts.resize(mLinkedNodeCount, std::vector<u32>(MAX_QUEUE_FLAG, 0));
    mUsedQueueCounts.resize(mLinkedNodeCount, std::vector<u32>(MAX_QUEUE_FLAG, 0));

    constexpr u32 MAX_QUEUE_FAMILIES = 16, MAX_QUEUE_COUNT = 64;
    std::array<std::array<float, MAX_QUEUE_COUNT>, MAX_QUEUE_FAMILIES> quFamPriorities{};
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (u32 i = 0; i < quFamProp2.size(); ++i)
    {
        u32 quCount = quFamProp2[i].queueFamilyProperties.queueCount;
        if (quCount > 0)
        {
            quCount              = mRequestAllAvailableQueues ? quCount : 1;
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
            for (u32 n = 0; n < mLinkedNodeCount; ++n)
            {
                mAvailableQueueCounts[n][queueFamilyFlags] = quCount;
            }
        }
    }

    // std::vector<float> queuePriorities      = {1.f};
    // VkDeviceQueueCreateInfo queueCreateInfo = {
    //     .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    //     .pNext            = nullptr,
    //     .flags            = 0,
    //     .queueFamilyIndex = mGraphicsQueueFamilyIndex,
    //     .queueCount       = (u32)queuePriorities.size(),
    //     .pQueuePriorities = queuePriorities.data(),
    // };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = mpVkActiveGPUFeatures,
        .flags                   = 0,
        .queueCreateInfoCount    = (u32)queueCreateInfos.size(),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledLayerCount       = (u32)gWantedDeviceLayers.size(),
        .ppEnabledLayerNames     = gWantedDeviceLayers.data(),
        .enabledExtensionCount   = (u32)gWantedDeviceExtensions.size(),
        .ppEnabledExtensionNames = gWantedDeviceExtensions.data(),
        .pEnabledFeatures        = nullptr};
    auto result = vkCreateDevice(mpVkActiveGPU, &deviceCreateInfo, nullptr, &mpVkDevice);
    AXE_ASSERT(VK_SUCCEEDED(result));
    volkLoadDevice(mpVkDevice);

    return true;
}  // namespace axe::rhi

bool VulkanDriver::_initVulkanMemoryAllocator() noexcept
{
    return true;
    // VmaAllocatorCreateInfo vmaCreateInfo = {0};
    // vmaCreateInfo.device                 = mpVkDevice;
    // vmaCreateInfo.physicalDevice         = mpVkActiveGPU;
    // vmaCreateInfo.instance               = mpVkInstance;
    // vmaCreateInfo.flags |= mDedicatedAllocationExtension ?
    // VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT : 0;
    // vmaCreateInfo.flags |= mBufferDeviceAddressExtension ?
    // VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT : 0;

    // VmaVulkanFunctions vulkanFunctions                      = {};
    // vulkanFunctions.vkGetInstanceProcAddr                   =
    // vkGetInstanceProcAddr; vulkanFunctions.vkGetDeviceProcAddr =
    // vkGetDeviceProcAddr; vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
    // vulkanFunctions.vkBindBufferMemory                      =
    // vkBindBufferMemory; vulkanFunctions.vkBindImageMemory =
    // vkBindImageMemory; vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
    // vulkanFunctions.vkCreateImage                           = vkCreateImage;
    // vulkanFunctions.vkDestroyBuffer                         =
    // vkDestroyBuffer; vulkanFunctions.vkDestroyImage = vkDestroyImage;
    // vulkanFunctions.vkFreeMemory                            = vkFreeMemory;
    // vulkanFunctions.vkGetBufferMemoryRequirements           =
    // vkGetBufferMemoryRequirements;
    // vulkanFunctions.vkGetBufferMemoryRequirements2KHR       =
    // vkGetBufferMemoryRequirements2KHR;
    // vulkanFunctions.vkGetImageMemoryRequirements            =
    // vkGetImageMemoryRequirements;
    // vulkanFunctions.vkGetImageMemoryRequirements2KHR        =
    // vkGetImageMemoryRequirements2KHR;
    // vulkanFunctions.vkGetPhysicalDeviceMemoryProperties     =
    // vkGetPhysicalDeviceMemoryProperties;
    // vulkanFunctions.vkGetPhysicalDeviceProperties =
    // vkGetPhysicalDeviceProperties; vulkanFunctions.vkMapMemory = vkMapMemory;
    // vulkanFunctions.vkUnmapMemory                           = vkUnmapMemory;
    // vulkanFunctions.vkFlushMappedMemoryRanges               =
    // vkFlushMappedMemoryRanges; vulkanFunctions.vkInvalidateMappedMemoryRanges
    // = vkInvalidateMappedMemoryRanges; vulkanFunctions.vkCmdCopyBuffer =
    // vkCmdCopyBuffer; vulkanFunctions.vkBindBufferMemory2KHR =
    // vkBindBufferMemory2; // need to VK_VERSION >= 1.1
    // vulkanFunctions.vkBindImageMemory2KHR                   =
    // vkBindImageMemory2;                       // need to VK_VERSION >= 1.1
    // vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR =
    // vkGetPhysicalDeviceMemoryProperties2KHR;  // need to VK_VERSION >= 1.1
    // vulkanFunctions.vkGetDeviceBufferMemoryRequirements     =
    // vkGetDeviceBufferMemoryRequirements;      // need to VK_VERSION >= 1.3
    // vulkanFunctions.vkGetDeviceImageMemoryRequirements      =
    // vkGetDeviceImageMemoryRequirements;
    // // need to VK_VERSION >= 1.3 vmaCreateInfo.pVulkanFunctions =
    // &vulkanFunctions; vmaCreateInfo.pAllocationCallbacks = nullptr;  //
    // TODO: add custom memory management return
    // vmaCreateAllocator(&vmaCreateInfo, &mpVmaAllocator)
    // == VK_SUCCESS;
}

}  // namespace axe::rhi
