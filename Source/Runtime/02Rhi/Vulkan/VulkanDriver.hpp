#pragma once

#define VK_NO_PROTOTYPES  // used for volk
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <02Rhi/Vulkan/VulkanSemaphore.hpp>
#include <02Rhi/Vulkan/VulkanFence.hpp>
#include <02Rhi/Vulkan/VulkanQueue.hpp>
#include <02Rhi/Vulkan/VulkanCmdPool.hpp>
#include <02Rhi/Vulkan/VulkanCmd.hpp>
#include <02Rhi/Vulkan/VulkanRenderTarget.hpp>
#include <02Rhi/Vulkan/VulkanSwapChain.hpp>

// #include <vma/vk_mem_alloc.h>

#include "02Rhi/Rhi.hpp"
#include <00Core/Thread/Thread.hpp>

#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <set>

namespace axe::rhi
{

class VulkanDriver final : public Driver
{
    friend class VulkanSemaphore;
    friend class VulkanFence;
    friend class VulkanQueue;
    friend class VulkanCmdPool;
    friend class VulkanCmd;
    friend class VulkanRenderTarget;
    friend class VulkanSwapChain;

    AXE_NON_COPYABLE(VulkanDriver);

public:
    VulkanDriver()                    = default;
    ~VulkanDriver() noexcept override = default;

    bool init(DriverDesc&) noexcept override;

    void exit() noexcept override;

#if AXE_(false, "Deprecated code from tutorial of intel")
    void draw() noexcept;
#endif

    auto getSwapchain() noexcept
    {
        return mpVkSwapChain;
    }
    auto getDevice() noexcept { return mpVkDevice; }

private:
    // clang-format off
    template <typename T, typename Desc>
    [[nodiscard]] T* _createHelper(Desc& desc) noexcept
    { auto* p = new T(this); if (!p || !p->_create(desc)) { delete p; p = nullptr;} return p; }

    template <typename T, typename I>
    [[nodiscard]] bool _destroyHelper(I*& p) noexcept
    { if (p && ((T*)p)->_destroy()) { delete p; p = nullptr; } return p == nullptr; }
    // clang-format on
public:
    [[nodiscard]] Semaphore* createSemaphore(SemaphoreDesc& desc) noexcept override { return (Semaphore*)_createHelper<VulkanSemaphore>(desc); }
    [[nodiscard]] Fence* createFence(FenceDesc& desc) noexcept override { return (Fence*)_createHelper<VulkanFence>(desc); }
    [[nodiscard]] Queue* createQueue(QueueDesc& desc) noexcept override { return (Queue*)_createHelper<VulkanQueue>(desc); }
    [[nodiscard]] SwapChain* createSwapChain(SwapChainDesc& desc) noexcept override { return (SwapChain*)_createHelper<VulkanSwapChain>(desc); }
    [[nodiscard]] CmdPool* createCmdPool(CmdPoolDesc& desc) noexcept override { return (CmdPool*)_createHelper<VulkanCmdPool>(desc); }
    [[nodiscard]] Cmd* createCmd(CmdDesc& desc) noexcept override { return (Cmd*)_createHelper<VulkanCmd>(desc); }
    [[nodiscard]] RenderTarget* createRenderTarget(RenderTargetDesc& desc) noexcept override { return (RenderTarget*)_createHelper<VulkanRenderTarget>(desc); }
    bool destroySemaphore(Semaphore*& p) noexcept override { return _destroyHelper<VulkanSemaphore>(p); }
    bool destroyFence(Fence*& p) noexcept override { return _destroyHelper<VulkanFence>(p); }
    bool destroyQueue(Queue*& p) noexcept override { return _destroyHelper<VulkanQueue>(p); }
    bool destroySwapChain(SwapChain*& p) noexcept override { return _destroyHelper<VulkanSwapChain>(p); }
    bool destroyCmdPool(CmdPool*& p) noexcept override { return _destroyHelper<VulkanCmdPool>(p); }
    bool destroyCmd(Cmd*& p) noexcept override { return _destroyHelper<VulkanCmd>(p); }
    bool destroyRenderTarget(RenderTarget*& p) noexcept override { return _destroyHelper<VulkanRenderTarget>(p); }

#if AXE_(false, "Deprecated code from tutorial of intel")
    bool createRenderPass() noexcept;

    bool createPipeline() noexcept;

    bool createShaderModule(const std::filesystem::path& filename, VkShaderModule& pVkShaderModule) noexcept;
#endif

private:
    bool _createInstance() noexcept;

    bool _selectedBestDevice() noexcept;

    bool _addDevice() noexcept;

    bool _initVulkanMemoryAllocator() noexcept;
#if AXE_(false, "Deprecated code from tutorial of intel")
    bool _onWindowSizeChanged() noexcept;

    bool _createSwapChain() noexcept;

    bool _createCmdBuffers() noexcept;

    bool _destroyCmdPool() noexcept;

    bool _recordCmdBuffers() noexcept;
#endif

public:
    GPUSettings mActiveGpuSettings;
    DriverDesc* mpDriverDesc                             = nullptr;
    VkInstance mpVkInstance                              = VK_NULL_HANDLE;
    VkDevice mpVkDevice                                  = VK_NULL_HANDLE;
    VkPhysicalDevice mpVkActiveGPU                       = VK_NULL_HANDLE;
    VkQueue mpVkQueue                                    = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties2* mpVkActiveGPUProperties = nullptr;
    VkPhysicalDeviceFeatures2* mpVkActiveGPUFeatures     = nullptr;

    VkSurfaceKHR mpVkSurface                             = VK_NULL_HANDLE;
    VkSwapchainKHR mpVkSwapChain                         = VK_NULL_HANDLE;
    VkFormat mSwapchainFormat                            = VK_FORMAT_UNDEFINED;

    VkRenderPass mpVkRenderPass                          = VK_NULL_HANDLE;
    VkPipeline mpVkPipeline                              = VK_NULL_HANDLE;

    VkCommandPool mpVkCmdPool                            = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> mVkCmdBuffers;
#ifdef AXE_RHI_VULKAN_ENABLE_DEBUG
    VkDebugUtilsMessengerEXT mpVkDebugUtilsMessenger = VK_NULL_HANDLE;
#endif
    std::vector<std::vector<u32>> mAvailableQueueCounts;
    std::vector<std::vector<u32>> mUsedQueueCounts;
    VkDescriptorPool mpEmptyDescriptorPool           = VK_NULL_HANDLE;
    VkDescriptorSetLayout mpEmptyDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet mpEmptyDescriptorSet             = VK_NULL_HANDLE;
    // VmaAllocator mpVmaAllocator                      = VK_NULL_HANDLE;

    union
    {
        struct
        {
            u8 mGraphicsQueueFamilyIndex;
            u8 mTransferQueueFamilyIndex;
            u8 mComputeQueueFamilyIndex;
        };
        u8 mQueueFamilyIndices[3]{U8_MAX};
    };

    union
    {
        struct
        {
            VkSemaphore mpImageAvailableSemaphore;
            VkSemaphore mpRenderingFinishedSemaphore;
        };
        VkSemaphore mpSemaphores[2]{VK_NULL_HANDLE};
    };

    u32 mOwnInstance                  : 1 = 0;
    u32 mRenderDocLayerEnabled        : 1 = 0;
    u32 mDedicatedAllocationExtension : 1 = 0;
    u32 mBufferDeviceAddressExtension : 1 = 0;
    u32 mDeviceGroupCreationExtension : 1 = 0;
    u32 mLinkedNodeCount              : 4 = 0;
    u32 mUnlinkedDriverIndex          : 4 = 0;
    u32 mGpuMode                      : 3 = 0;
    u32 mShaderTarget                 : 4 = 0;
    u32 mEnableGpuBasedValidation     : 1 = 0;
    u32 mRequestAllAvailableQueues    : 1 = 0;
};

}  // namespace axe::rhi