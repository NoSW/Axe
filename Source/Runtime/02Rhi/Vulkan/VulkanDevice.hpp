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
#include "02Rhi/Vulkan/VulkanTexture.hpp"
#include "02Rhi/Vulkan/VulkanBuffer.hpp"
#include "02Rhi/Vulkan/VulkanRenderTarget.hpp"
#include "02Rhi/Vulkan/VulkanSwapChain.hpp"
#include "02Rhi/Vulkan/VulkanShader.hpp"
#include "02Rhi/Vulkan/VulkanRootSignature.hpp"
#include "02Rhi/Vulkan/VulkanDescriptorSet.hpp"

#include "00Core/Memory/Memory.hpp"

#include <vk_mem_alloc.h>

static inline void* VKAPI_PTR vk_allocation(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) { return axe::memory::DefaultMemoryResource::get()->new_aligned(size, alignment); }
static inline void* VKAPI_PTR vk_reallocation(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope) { return axe::memory::DefaultMemoryResource::get()->realloc(pOriginal, size); }
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

    bool queryAvailableQueueIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) const noexcept;

    void requestQueueIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) noexcept;

    auto handle() noexcept { return _mpHandle; }

    VkSampler getDefaultSamplerHandle() noexcept { return ((VulkanSampler*)_mNullDescriptors.mpDefaultSampler)->handle(); }
    VkBuffer getDefaultBufferSRVHandle() noexcept { return ((VulkanBuffer*)_mNullDescriptors.mpDefaultBufferSRV)->handle(); }
    VkBuffer getDefaultBufferUAVHandle() noexcept { return ((VulkanBuffer*)_mNullDescriptors.mpDefaultBufferUAV)->handle(); }
    VkImage getDefaultTextureSRVHandle(TextureDimension dim) noexcept { return ((VulkanTexture*)_mNullDescriptors.mpDefaultTextureSRV[(u8)dim])->handle(); }
    VkImage getDefaultTextureUAVHandle(TextureDimension dim) noexcept { return ((VulkanTexture*)_mNullDescriptors.mpDefaultTextureUAV[(u8)dim])->handle(); }

    void initial_transition(Texture* pTexture, ResourceState startState) noexcept;

private:
    void _setDebugLabel(void* handle, VkObjectType, std::string_view) noexcept;

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
            static_assert(std::is_base_of_v<DescBase, Desc>, "Desc must be derived from DescBase for using mLabel");
            p->_mLabel = desc.mLabel;
            _setDebugLabel((void*)p->handle(), T::getVkTypeId(), desc.mLabel);
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
    AXE_PUBLIC ~VulkanDevice() noexcept override;
    AXE_PUBLIC [[nodiscard]] Semaphore* createSemaphore(SemaphoreDesc& desc) noexcept override { return (Semaphore*)_createHelper<VulkanSemaphore>(desc); }
    AXE_PUBLIC [[nodiscard]] Fence* createFence(FenceDesc& desc) noexcept override { return (Fence*)_createHelper<VulkanFence>(desc); }
    AXE_PUBLIC [[nodiscard]] Queue* requestQueue(QueueDesc& desc) noexcept override { return (Queue*)_createHelper<VulkanQueue>(desc); }
    AXE_PUBLIC [[nodiscard]] SwapChain* createSwapChain(SwapChainDesc& desc) noexcept override { return (SwapChain*)_createHelper<VulkanSwapChain>(desc); }
    AXE_PUBLIC [[nodiscard]] CmdPool* createCmdPool(CmdPoolDesc& desc) noexcept override { return (CmdPool*)_createHelper<VulkanCmdPool>(desc); }
    AXE_PUBLIC [[nodiscard]] Cmd* createCmd(CmdDesc& desc) noexcept override { return (Cmd*)_createHelper<VulkanCmd>(desc); }
    AXE_PUBLIC [[nodiscard]] Sampler* createSampler(SamplerDesc& desc) noexcept override { return (Sampler*)_createHelper<VulkanSampler>(desc); }
    AXE_PUBLIC [[nodiscard]] Texture* createTexture(TextureDesc& desc) noexcept override { return (Texture*)_createHelper<VulkanTexture>(desc); }
    AXE_PUBLIC [[nodiscard]] Buffer* createBuffer(BufferDesc& desc) noexcept override { return (Buffer*)_createHelper<VulkanBuffer>(desc); }
    AXE_PUBLIC [[nodiscard]] RenderTarget* createRenderTarget(RenderTargetDesc& desc) noexcept override { return (RenderTarget*)_createHelper<VulkanRenderTarget>(desc); }
    AXE_PUBLIC [[nodiscard]] Shader* createShader(ShaderDesc& desc) noexcept override { return (Shader*)_createHelper<VulkanShader>(desc); }
    AXE_PUBLIC [[nodiscard]] RootSignature* createRootSignature(RootSignatureDesc& desc) noexcept override { return (RootSignature*)_createHelper<VulkanRootSignature>(desc); }
    AXE_PUBLIC [[nodiscard]] DescriptorSet* createDescriptorSet(DescriptorSetDesc& desc) noexcept override { return (DescriptorSet*)_createHelper<VulkanDescriptorSet>(desc); }
    AXE_PUBLIC bool destroySemaphore(Semaphore*& p) noexcept override { return _destroyHelper<VulkanSemaphore>(p); }
    AXE_PUBLIC bool destroyFence(Fence*& p) noexcept override { return _destroyHelper<VulkanFence>(p); }
    AXE_PUBLIC bool releaseQueue(Queue*& p) noexcept override { return _destroyHelper<VulkanQueue>(p); }
    AXE_PUBLIC bool destroySwapChain(SwapChain*& p) noexcept override { return _destroyHelper<VulkanSwapChain>(p); }
    AXE_PUBLIC bool destroyCmdPool(CmdPool*& p) noexcept override { return _destroyHelper<VulkanCmdPool>(p); }
    AXE_PUBLIC bool destroyCmd(Cmd*& p) noexcept override { return _destroyHelper<VulkanCmd>(p); }
    AXE_PUBLIC bool destroySampler(Sampler*& p) noexcept override { return _destroyHelper<VulkanSampler>(p); }
    AXE_PUBLIC bool destroyTexture(Texture*& p) noexcept override { return _destroyHelper<VulkanTexture>(p); }
    AXE_PUBLIC bool destroyBuffer(Buffer*& p) noexcept override { return _destroyHelper<VulkanBuffer>(p); }
    AXE_PUBLIC bool destroyRenderTarget(RenderTarget*& p) noexcept override { return _destroyHelper<VulkanRenderTarget>(p); }
    AXE_PUBLIC bool destroyShader(Shader*& p) noexcept override { return _destroyHelper<VulkanShader>(p); }
    AXE_PUBLIC bool destroyRootSignature(RootSignature*& p) noexcept override { return _destroyHelper<VulkanRootSignature>(p); }
    AXE_PUBLIC bool destroyDescriptorSet(DescriptorSet*& p) noexcept override { return _destroyHelper<VulkanDescriptorSet>(p); }

public:
    constexpr static VkObjectType getVkTypeId() noexcept { return VK_OBJECT_TYPE_DEVICE; }

private:
    static constexpr u32 MAX_QUEUE_FLAG = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT |
                                          VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT | VK_QUEUE_PROTECTED_BIT | VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_OPTICAL_FLOW_BIT_NV;

    // supporting status
    ShaderModel _mShaderModel                         = SHADER_MODEL_6_7;
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
        u32 mAvailableCount = 0;  // total available count
        u32 mUsedCount      = 0;  // already used count
        u8 mFamilyIndex     = U8_MAX;
    };

    std::array<QueueInfo, MAX_QUEUE_FLAG> _mQueueInfos;  // <familyIndex, count>
    std::array<u8, QUEUE_TYPE_COUNT> _mQueueFamilyIndexes{U8_MAX, U8_MAX, U8_MAX};
};

}  // namespace axe::rhi
