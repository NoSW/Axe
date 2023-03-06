#pragma once

#include "02Rhi/Rhi.hpp"
#include "02Rhi/Vulkan/VulkanEnums.hpp"

#include "02Rhi/Vulkan/VulkanAdapter.hpp"

#include "02Rhi/Vulkan/VulkanSemaphore.hpp"
#include "02Rhi/Vulkan/VulkanFence.hpp"
#include "02Rhi/Vulkan/VulkanQueue.hpp"
#include "02Rhi/Vulkan/VulkanCmdPool.hpp"
#include "02Rhi/Vulkan/VulkanCmd.hpp"
#include "02Rhi/Vulkan/VulkanSampler.hpp"
#include "02Rhi/Vulkan/VulkanRenderTarget.hpp"
#include "02Rhi/Vulkan/VulkanSwapChain.hpp"
#include "02Rhi/Vulkan/VulkanShader.hpp"

namespace axe::rhi
{

class VulkanDevice final : public Device
{
public:
    friend class VulkanSemaphore;
    friend class VulkanFence;
    friend class VulkanQueue;
    friend class VulkanCmdPool;
    friend class VulkanCmd;
    friend class VulkanSampler;
    friend class VulkanRenderTarget;
    friend class VulkanSwapChain;
    friend class VulkanShader;

private:
    void _collectQueueInfo() noexcept;

public:
    VulkanDevice(VulkanAdapter*, DeviceDesc& desc) noexcept;
    ~VulkanDevice() noexcept;

    void findQueueFamilyIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) noexcept;

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
    AXE_PUBLIC [[nodiscard]] Semaphore* createSemaphore(SemaphoreDesc& desc) noexcept override { return (Semaphore*)_createHelper<VulkanSemaphore>(desc); }
    AXE_PUBLIC [[nodiscard]] Fence* createFence(FenceDesc& desc) noexcept override { return (Fence*)_createHelper<VulkanFence>(desc); }
    AXE_PUBLIC [[nodiscard]] Queue* requestQueue(QueueDesc& desc) noexcept override { return (Queue*)_createHelper<VulkanQueue>(desc); }
    AXE_PUBLIC [[nodiscard]] SwapChain* createSwapChain(SwapChainDesc& desc) noexcept override { return (SwapChain*)_createHelper<VulkanSwapChain>(desc); }
    AXE_PUBLIC [[nodiscard]] CmdPool* createCmdPool(CmdPoolDesc& desc) noexcept override { return (CmdPool*)_createHelper<VulkanCmdPool>(desc); }
    AXE_PUBLIC [[nodiscard]] Cmd* createCmd(CmdDesc& desc) noexcept override { return (Cmd*)_createHelper<VulkanCmd>(desc); }
    AXE_PUBLIC [[nodiscard]] Sampler* createSampler(SamplerDesc& desc) noexcept override { return (Sampler*)_createHelper<VulkanSampler>(desc); }
    AXE_PUBLIC [[nodiscard]] RenderTarget* createRenderTarget(RenderTargetDesc& desc) noexcept override { return (RenderTarget*)_createHelper<VulkanRenderTarget>(desc); }
    AXE_PUBLIC [[nodiscard]] Shader* createShader(ShaderDesc& desc) noexcept override { return (Shader*)_createHelper<VulkanShader>(desc); }
    AXE_PUBLIC bool destroySemaphore(Semaphore*& p) noexcept override { return _destroyHelper<VulkanSemaphore>(p); }
    AXE_PUBLIC bool destroyFence(Fence*& p) noexcept override { return _destroyHelper<VulkanFence>(p); }
    AXE_PUBLIC bool releaseQueue(Queue*& p) noexcept override { return _destroyHelper<VulkanQueue>(p); }
    AXE_PUBLIC bool destroySwapChain(SwapChain*& p) noexcept override { return _destroyHelper<VulkanSwapChain>(p); }
    AXE_PUBLIC bool destroyCmdPool(CmdPool*& p) noexcept override { return _destroyHelper<VulkanCmdPool>(p); }
    AXE_PUBLIC bool destroySampler(Sampler*& p) noexcept override { return _destroyHelper<VulkanSampler>(p); }
    AXE_PUBLIC bool destroyCmd(Cmd*& p) noexcept override { return _destroyHelper<VulkanCmd>(p); }
    AXE_PUBLIC bool destroyRenderTarget(RenderTarget*& p) noexcept override { return _destroyHelper<VulkanRenderTarget>(p); }
    AXE_PUBLIC bool destroyShader(Shader*& p) noexcept override { return _destroyHelper<VulkanShader>(p); }

private:
    static constexpr u32 MAX_QUEUE_FLAG = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                                          VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT | VK_QUEUE_PROTECTED_BIT;

    VulkanAdapter* const _mpAdapter = nullptr;

    VkDevice _mpHandle              = VK_NULL_HANDLE;

    struct QueueInfo
    {
        u32 mAvailableCount = 0;
        u32 mUsedCount      = 0;
        u8 mFamilyIndex     = U8_MAX;
    };

    std::array<QueueInfo, MAX_QUEUE_FLAG> _mQueueInfos;  // <familyIndex, count>

    ShaderModel _mShaderModel = SHADER_MODEL_6_7;
};

}  // namespace axe::rhi
