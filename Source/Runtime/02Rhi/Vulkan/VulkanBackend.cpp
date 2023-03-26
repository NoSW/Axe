#include "02Rhi/Vulkan/VulkanBackend.hpp"

#include <00Core/IO/IO.hpp>
#ifndef VK_VERSION_1_3
#error ""
#endif  // !VK_VERSION_1_3

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include "02Rhi/Vulkan/VulkanAdapter.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include "00Core/Memory/Memory.hpp"
#include "00Core/OS/OS.hpp"

namespace axe::rhi
{

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

const static std::pmr::vector<const char*> gsWantedInstanceLayers = {
#if AXE_RHI_VULKAN_ENABLE_DEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
    "VK_LAYER_KHRONOS_synchronization2",
    "VK_LAYER_RENDERDOC_Capture",
};

const static std::pmr::vector<const char*> gsWantedInstanceExtensions = {
    // Surface
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
#if _WIN32
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif __linux__
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif __APPLE__
#error "Unsupported platform yet"
#endif

// For debug
#if AXE_RHI_VULKAN_ENABLE_DEBUG
#if VK_EXT_debug_utils
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#else
#error "Unsupported VK_EXT_debug_utils"
#endif
#endif

    // Memory
    VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,

    // To legally use HDR formats
    VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,

// Multi GPU Extensions
#if VK_KHR_device_group_creation
    VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
#endif

    // Property querying extensions
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,

};

bool find_intersect_set(const std::pmr::vector<const char*>& support, const std::pmr::vector<const char*>& want, std::pmr::vector<const char*>& ready, bool earlyExit) noexcept
{
    for (const char* v1 : want)
    {
        if (std::find_if(ready.begin(), ready.end(), [v1](const char* v2)
                         { return strcmp(v1, v2) == 0; }) != ready.end())
        {
            continue;  // already added to readyList
        }

        if (std::find_if(support.begin(), support.end(), [v1](const char* v2)
                         { return strcmp(v1, v2) == 0; }) != support.end())
        {
            ready.push_back(v1);  // found firstly
            AXE_INFO("{} enabled", v1);
        }
        else
        {
            AXE_WARN("{} is not supported", v1);  // not supported
            if (earlyExit)
            {
                return false;
            }
        }
    }
    return true;
}

VulkanBackend::VulkanBackend(BackendDesc& desc) noexcept
{
    memory::MonoMemoryResource<4096> arena;
    // create instance
    _mGpuMode = desc.mGpuMode;
#if AXE_RHI_VULKAN_USE_DISPATCH_TABLE
    // TODO: using device table
#else
    // Attempt to load Vulkan loader from the system
    AXE_CHECK(VK_SUCCEEDED(volkInitialize()))
#endif

    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::pmr::vector<VkLayerProperties> layers(layerCount, &arena);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());
    std::pmr::vector<const char*> layerNames(layerCount, nullptr, &arena);
    for (u32 i = 0; i < layers.size(); ++i) { layerNames[i] = layers[i].layerName; }

    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::pmr::vector<VkExtensionProperties> extensions(extensionCount, &arena);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::pmr::vector<const char*> extensionNames(extensionCount, nullptr, &arena);
    for (u32 i = 0; i < extensions.size(); ++i) { extensionNames[i] = extensions[i].extensionName; }

    std::pmr::vector<const char*> readyLayers(&arena);
    std::pmr::vector<const char*> readyExts(&arena);
    find_intersect_set(layerNames, gsWantedInstanceLayers, readyLayers, false);
    find_intersect_set(extensionNames, gsWantedInstanceExtensions, readyExts, false);

    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = nullptr,
        .pApplicationName   = desc.mAppName.data(),
        .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
        .pEngineName        = "Axe",
        .engineVersion      = VK_MAKE_VERSION(0, 1, 0),
        .apiVersion         = VK_API_VERSION_1_3};

    VkInstanceCreateInfo createInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = (u32)readyLayers.size(),
        .ppEnabledLayerNames     = readyLayers.data(),
        .enabledExtensionCount   = (u32)readyExts.size(),
        .ppEnabledExtensionNames = readyExts.data()};

    auto result = vkCreateInstance(&createInfo, nullptr, &_mpHandle);
    AXE_CHECK(VK_SUCCEEDED(result));

    // load all required Vulkan entrypoints, including all extensions
    volkLoadInstanceOnly(_mpHandle);

    // create debug utils
#if AXE_RHI_VULKAN_ENABLE_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo{
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext           = nullptr,
        .flags           = 0,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
        .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debug_callback,
        .pUserData       = nullptr};
    AXE_CHECK(VK_SUCCEEDED(vkCreateDebugUtilsMessengerEXT(_mpHandle, &debugUtilsCreateInfo, nullptr, &mpVkDebugUtilsMessenger)));
#endif

    // collect available gpus
    u32 gpuCount = 0;
    result       = vkEnumeratePhysicalDevices(_mpHandle, &gpuCount, nullptr);
    AXE_ASSERT(VK_SUCCEEDED(result));
    AXE_ASSERT(gpuCount > 0, "No detected gpu");
    std::pmr::vector<VkPhysicalDevice> gpus(gpuCount, &arena);
    result = vkEnumeratePhysicalDevices(_mpHandle, &gpuCount, gpus.data());
    AXE_ASSERT(VK_SUCCEEDED(result));

    gpuCount = std::min(gpuCount, (u32)MAX_NUM_ADAPTER_PER_BACKEND);
    for (u32 i = 0; i < gpuCount; ++i)
    {
        if (i >= gpuCount) { break; }
        _mAdapters[i] = std::make_unique<VulkanAdapter>(this, gpus[i], i);
    }
    std::sort(_mAdapters.begin(), _mAdapters.begin() + gpuCount, [this](auto& gpu1, auto& gpu2)
              { return VulkanAdapter::isBetterGpu(*gpu1, *gpu2); });

    AXE_CHECK(_mAdapters[0]->isSupportQueue(QUEUE_TYPE_GRAPHICS), "No gpu supporting graphics queue");
    AXE_CHECK(_mAdapters[0]->type() != ADAPTER_TYPE_CPU, "The only available GPU is type of CPU");
}

VulkanBackend::~VulkanBackend() noexcept
{
    if (mpVkSurface != VK_NULL_HANDLE) { vkDestroySurfaceKHR(_mpHandle, mpVkSurface, nullptr); }
#if AXE_RHI_VULKAN_ENABLE_DEBUG
    if (mpVkDebugUtilsMessenger)
    {
        vkDestroyDebugUtilsMessengerEXT(_mpHandle, mpVkDebugUtilsMessenger, nullptr);
    }
#endif
    vkDestroyInstance(_mpHandle, nullptr);
}

Adapter* VulkanBackend::requestAdapter(AdapterDesc& desc) noexcept
{
    for (auto& adapter : _mAdapters)
    {
        if (adapter->idle())
        {
            adapter->take();
            const auto& setting = adapter->requestGPUSettings();
            AXE_INFO("Selected GPU[{}], Name: {}, Vendor Id: {:#x}, Model Id {:#x}, Backend Version: {}",
                     adapter->nodeIndex(), setting.mGpuVendorPreset.mGpuName, setting.mGpuVendorPreset.mVendorId,
                     setting.mGpuVendorPreset.mModelId, setting.mGpuVendorPreset.mGpuBackendVersion);
            return adapter.get();
        }
    }
    AXE_ERROR("Failed to fond an idle adapter, return nullptr");
    return nullptr;
}

void VulkanBackend::releaseAdapter(Adapter*& adapter) noexcept
{
    AXE_ASSERT(adapter);
    static_cast<VulkanAdapter*>(adapter)->release();
    static_cast<VulkanAdapter*>(adapter)->take();
    adapter = nullptr;
}

}  // namespace axe::rhi
