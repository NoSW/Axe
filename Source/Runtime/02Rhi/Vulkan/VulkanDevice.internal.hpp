#pragma once

#include "02Rhi/Rhi.hpp"
#include "VulkanEnums.internal.hpp"

#include "VulkanAdapter.internal.hpp"

#include "VulkanSemaphore.internal.hpp"
#include "VulkanFence.internal.hpp"
#include "VulkanQueue.internal.hpp"
#include "VulkanCmdPool.internal.hpp"
#include "VulkanCmd.internal.hpp"
#include "VulkanSampler.internal.hpp"
#include "VulkanTexture.internal.hpp"
#include "VulkanBuffer.internal.hpp"
#include "VulkanRenderTarget.internal.hpp"
#include "VulkanSwapChain.internal.hpp"
#include "VulkanShader.internal.hpp"
#include "VulkanRootSignature.internal.hpp"
#include "VulkanDescriptorSet.internal.hpp"
#include "VulkanPipeline.internal.hpp"

#include "00Core/Memory/Memory.hpp"

#include <vk_mem_alloc.h>

static inline void* VKAPI_PTR
vk_allocation(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    return axe::memory::DefaultMemoryResource::get()->new_aligned(size, alignment);
}
static inline void* VKAPI_PTR vk_reallocation(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
    return axe::memory::DefaultMemoryResource::get()->realloc(pOriginal, size);
}
static inline void VKAPI_PTR vk_free(void* pUserData, void* pMemory) { axe::memory::DefaultMemoryResource::get()->free(pMemory); }
static inline void VKAPI_PTR vk_internal_allocation(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) { AXE_ASSERT(false, "never used"); }
static inline void VKAPI_PTR vk_internal_free(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope) { AXE_ASSERT(false, "never used"); }

namespace axe::rhi
{

inline constexpr VkAllocationCallbacks g_VkAllocationCallbacks{
    .pUserData             = nullptr,
    .pfnAllocation         = vk_allocation,
    .pfnReallocation       = vk_reallocation,
    .pfnFree               = vk_free,
    .pfnInternalAllocation = vk_internal_allocation,
    .pfnInternalFree       = vk_internal_free};

struct DeviceExtension;

class VulkanDevice final : public Device
{
public:
    friend class VulkanSemaphore;
    friend class VulkanFence;
    friend class VulkanQueue;
    friend class VulkanCmdPool;
    friend class VulkanCmd;
    friend class VulkanSampler;
    friend class VulkanTexture;
    friend class VulkanBuffer;
    friend class VulkanRenderTarget;
    friend class VulkanSwapChain;
    friend class VulkanShader;
    friend class VulkanRootSignature;
    friend class VulkanDescriptorSet;

private:
    void _collectQueueInfo() noexcept;

    bool _createLogicalDevice(const DeviceDesc& desc) noexcept;
    bool _initVMA() noexcept;
    void _createDefaultResource() noexcept;
    void _destroyDefaultResource() noexcept;

public:
    VulkanDevice(VulkanAdapter*, DeviceDesc& desc) noexcept;

    AXE_PRIVATE bool queryAvailableQueueIndex(QueueTypeFlag quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) const noexcept;

    AXE_PRIVATE void requestQueueIndex(QueueTypeFlag quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) noexcept;

    AXE_PRIVATE auto handle() noexcept { return _mpHandle; }

    AXE_PRIVATE VkSampler getDefaultSamplerHandle() noexcept { return ((VulkanSampler*)_mNullDescriptors.pDefaultSampler)->handle(); }
    AXE_PRIVATE VkBuffer getDefaultBufferSRVHandle() noexcept { return ((VulkanBuffer*)_mNullDescriptors.pDefaultBufferSRV)->handle(); }
    AXE_PRIVATE VkBuffer getDefaultBufferUAVHandle() noexcept { return ((VulkanBuffer*)_mNullDescriptors.pDefaultBufferUAV)->handle(); }
    AXE_PRIVATE VkImage getDefaultTextureSRVHandle(TextureDimension dim) noexcept { return ((VulkanTexture*)_mNullDescriptors.pDefaultTextureSRV[(u8)dim])->handle(); }
    AXE_PRIVATE VkImage getDefaultTextureUAVHandle(TextureDimension dim) noexcept { return ((VulkanTexture*)_mNullDescriptors.pDefaultTextureUAV[(u8)dim])->handle(); }

    AXE_PRIVATE void initial_transition(Texture* pTexture, ResourceStateFlags startState) noexcept;

    AXE_PRIVATE void setGpuMarker_DebugActiveOnly(void* handle, VkObjectType, std::string_view) noexcept;

private:
    template <typename T, typename Desc>
    [[nodiscard]] T* _createHelper(Desc& desc) noexcept
    {
        auto* p = new T(this);
        if (!p || !p->_create(desc))
        {
            delete p;
            p = nullptr;
        }
        else
        {
#if AXE_RHI_VULKAN_ENABLE_DEBUG
            static_assert(std::is_base_of_v<RhiObjectBase, T>, "T must be derived from RhiObjectBase for using _mLabel");
            static_assert(std::is_base_of_v<DescBase, Desc>, "Desc must be derived from DescBase for using label");
            p->_mLabel = desc.getLabel_DebugActiveOnly();
            setGpuMarker_DebugActiveOnly((void*)p->handle(), T::VK_TYPE_ID, p->_mLabel);
#endif
        }

        return p;
    }

    template <typename T, typename I>
    [[nodiscard]] bool _destroyHelper(I*& p) noexcept
    {
        if (p && ((T*)p)->_destroy())
        {
            delete p;
            p = nullptr;
        }
        return p == nullptr;
    }

public:
    ~VulkanDevice() noexcept override;
    [[nodiscard]] Adapter* getAdapter() noexcept override { return (Adapter*)_mpAdapter; }
    [[nodiscard]] Semaphore* createSemaphore(SemaphoreDesc& desc) noexcept override { return (Semaphore*)_createHelper<VulkanSemaphore>(desc); }
    [[nodiscard]] Fence* createFence(FenceDesc& desc) noexcept override { return (Fence*)_createHelper<VulkanFence>(desc); }
    [[nodiscard]] Queue* requestQueue(QueueDesc& desc) noexcept override { return (Queue*)_createHelper<VulkanQueue>(desc); }
    [[nodiscard]] SwapChain* createSwapChain(SwapChainDesc& desc) noexcept override { return (SwapChain*)_createHelper<VulkanSwapChain>(desc); }
    [[nodiscard]] CmdPool* createCmdPool(CmdPoolDesc& desc) noexcept override { return (CmdPool*)_createHelper<VulkanCmdPool>(desc); }
    [[nodiscard]] Cmd* createCmd(CmdDesc& desc) noexcept override { return (Cmd*)_createHelper<VulkanCmd>(desc); }
    [[nodiscard]] Sampler* createSampler(SamplerDesc& desc) noexcept override { return (Sampler*)_createHelper<VulkanSampler>(desc); }
    [[nodiscard]] Texture* createTexture(TextureDesc& desc) noexcept override { return (Texture*)_createHelper<VulkanTexture>(desc); }
    [[nodiscard]] Buffer* createBuffer(BufferDesc& desc) noexcept override { return (Buffer*)_createHelper<VulkanBuffer>(desc); }
    [[nodiscard]] RenderTarget* createRenderTarget(RenderTargetDesc& desc) noexcept override { return (RenderTarget*)_createHelper<VulkanRenderTarget>(desc); }
    [[nodiscard]] Shader* createShader(ShaderDesc& desc) noexcept override { return (Shader*)_createHelper<VulkanShader>(desc); }
    [[nodiscard]] RootSignature* createRootSignature(RootSignatureDesc& desc) noexcept override { return (RootSignature*)_createHelper<VulkanRootSignature>(desc); }
    [[nodiscard]] DescriptorSet* createDescriptorSet(DescriptorSetDesc& desc) noexcept override { return (DescriptorSet*)_createHelper<VulkanDescriptorSet>(desc); }
    [[nodiscard]] Pipeline* createPipeline(PipelineDesc& desc) noexcept override { return (Pipeline*)_createHelper<VulkanPipeline>(desc); }
    bool destroySemaphore(Semaphore*& p) noexcept override { return _destroyHelper<VulkanSemaphore>(p); }
    bool destroyFence(Fence*& p) noexcept override { return _destroyHelper<VulkanFence>(p); }
    bool releaseQueue(Queue*& p) noexcept override { return _destroyHelper<VulkanQueue>(p); }
    bool destroySwapChain(SwapChain*& p) noexcept override { return _destroyHelper<VulkanSwapChain>(p); }
    bool destroyCmdPool(CmdPool*& p) noexcept override { return _destroyHelper<VulkanCmdPool>(p); }
    bool destroyCmd(Cmd*& p) noexcept override { return _destroyHelper<VulkanCmd>(p); }
    bool destroySampler(Sampler*& p) noexcept override { return _destroyHelper<VulkanSampler>(p); }
    bool destroyTexture(Texture*& p) noexcept override { return _destroyHelper<VulkanTexture>(p); }
    bool destroyBuffer(Buffer*& p) noexcept override { return _destroyHelper<VulkanBuffer>(p); }
    bool destroyRenderTarget(RenderTarget*& p) noexcept override { return _destroyHelper<VulkanRenderTarget>(p); }
    bool destroyShader(Shader*& p) noexcept override { return _destroyHelper<VulkanShader>(p); }
    bool destroyRootSignature(RootSignature*& p) noexcept override { return _destroyHelper<VulkanRootSignature>(p); }
    bool destroyDescriptorSet(DescriptorSet*& p) noexcept override { return _destroyHelper<VulkanDescriptorSet>(p); }
    bool destroyPipeline(Pipeline*& p) noexcept override { return _destroyHelper<VulkanPipeline>(p); }

public:
    constexpr static auto VK_TYPE_ID = VK_OBJECT_TYPE_DEVICE;

private:
    static constexpr u32 MAX_QUEUE_FLAG = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                                          VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT | VK_QUEUE_PROTECTED_BIT | VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_OPTICAL_FLOW_BIT_NV;

    // supporting status
    ShaderModel _mShaderModel                         = ShaderModel::SM_6_7;
    u32 _mExternalMemoryExtension : 1                 = 0;
    u32 _mRaytracingSupported     : 1                 = 0;

    // handles
    VulkanAdapter* const _mpAdapter                   = nullptr;
    VkDevice _mpHandle                                = VK_NULL_HANDLE;
    VmaAllocator _mpVmaAllocator                      = VK_NULL_HANDLE;

    // default resource
    VkDescriptorPool _mpEmptyDescriptorPool           = VK_NULL_HANDLE;
    VkDescriptorSetLayout _mpEmptyDescriptorSetLayout = VK_NULL_HANDLE;  // TODO
    VkDescriptorSet _mpEmptyDescriptorSet             = VK_NULL_HANDLE;
    NullDescriptors _mNullDescriptors{};

    // usage status
    struct QueueInfo
    {
        u32 availableCount = 0;  // total available count
        u32 usedCount      = 0;  // already used count
        u8 familyIndex     = U8_MAX;
    };

    std::array<QueueInfo, MAX_QUEUE_FLAG> _mQueueInfos;  // <familyIndex, count>
    std::array<u8, (u32)QueueTypeFlag::COUNT> _mQueueFamilyIndexes{U8_MAX, U8_MAX, U8_MAX};
};

}  // namespace axe::rhi
