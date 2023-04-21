#pragma once
#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

#include <00Core/Thread/Thread.hpp>

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <set>

namespace axe::rhi
{

bool find_intersect_set(const std::pmr::vector<const char*>& support, const std::pmr::vector<const char*>& want, std::pmr::vector<const char*>& ready, bool earlyExit) noexcept;

class VulkanAdapter;

class VulkanBackend final : public Backend
{
    AXE_NON_COPYABLE(VulkanBackend);

public:
    VulkanBackend(BackendDesc&) noexcept;

    ~VulkanBackend() noexcept;

    auto handle() const noexcept { return _mpHandle; }

public:
    AXE_PUBLIC Adapter* requestAdapter(AdapterDesc&) noexcept override;

    AXE_PUBLIC void releaseAdapter(Adapter*&) noexcept override;

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_INSTANCE; }

public:
    // handle
    VkInstance _mpHandle     = VK_NULL_HANDLE;

    VkSurfaceKHR mpVkSurface = VK_NULL_HANDLE;

#ifdef AXE_RHI_VULKAN_ENABLE_DEBUG
    VkDebugUtilsMessengerEXT mpVkDebugUtilsMessenger = VK_NULL_HANDLE;
#endif

    // auto
    std::array<std::unique_ptr<VulkanAdapter>, MAX_NUM_ADAPTER_PER_BACKEND> _mAdapters;

    // scalar
    u32 _mShaderTarget              : 4 = 0;
    u32 _mEnableGpuBasedValidation  : 1 = 0;
    u32 _mRequestAllAvailableQueues : 1 = 0;
};

}  // namespace axe::rhi