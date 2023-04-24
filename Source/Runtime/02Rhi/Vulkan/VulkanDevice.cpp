#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include <volk.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#if !(VMA_VULKAN_VERSION >= 1003000)
#error "need vulkan>=1.3"
#endif

namespace axe::rhi
{

static constexpr bool ALREADY_PROMPTED_TO_VK_VERSION_1_2_OR_HIGHER = true;

bool VulkanDevice::_createLogicalDevice(const DeviceDesc& desc) noexcept
{
    // Query layers and extensions
    u32 layerCount = 0;
    vkEnumerateDeviceLayerProperties(_mpAdapter->handle(), &layerCount, nullptr);
    std::pmr::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateDeviceLayerProperties(_mpAdapter->handle(), &layerCount, layers.data());
    std::pmr::vector<const char*> layerNames(layerCount, nullptr);
    for (u32 i = 0; i < layers.size(); ++i) { layerNames[i] = layers[i].layerName; }
    // if (strcmp(layerNames[i], "VK_LAYER_RENDERDOC_Capture") == 0) { true; }

    u32 extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(_mpAdapter->handle(), nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(_mpAdapter->handle(), nullptr, &extensionCount, extensions.data());
    std::pmr::vector<const char*> extensionNames(extensionCount, nullptr);
    for (u32 i = 0; i < extensions.size(); ++i) { extensionNames[i] = extensions[i].extensionName; }

    std::pmr::vector<const char*> readyLayers;
    std::pmr::vector<const char*> readyExts;

    const auto sup = [&readyExts, &extensionNames](std::pmr::vector<const char*> wantExtNames)
    {
        return find_intersect_set(extensionNames, wantExtNames, readyExts, true);
    };

    VkPhysicalDeviceFeatures2 gpuFeatures2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkBaseOutStructure* pBaseOutStructure  = (VkBaseOutStructure*)&gpuFeatures2;
    const auto addIntoExtChain             = [&pBaseOutStructure](bool cond, VkBaseOutStructure* pNext)
    {
        if (cond)
        {
            pBaseOutStructure->pNext = pNext;
            pBaseOutStructure        = (VkBaseOutStructure*)pBaseOutStructure->pNext;
        }
        return cond;
    };

    // allow device-local memory allocations to be paged in and out by the operating system and **may** transparently move
    // device-local memory allocations to host-local memory to better share device-local memory with other applications,if enabled
#if VK_EXT_pageable_device_local_memory & VK_EXT_memory_priority
    VkPhysicalDevicePageableDeviceLocalMemoryFeaturesEXT pageableDeviceLocalMemoryFeatures{
        .sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PAGEABLE_DEVICE_LOCAL_MEMORY_FEATURES_EXT,
        .pNext                     = nullptr,
        .pageableDeviceLocalMemory = VK_TRUE};
    if (sup({VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME}))  // extension dependency of VK_EXT_pageable_device_local_memory
    {
        addIntoExtChain(sup({VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME}), (VkBaseOutStructure*)&pageableDeviceLocalMemoryFeatures);
    }
#endif

#if VK_EXT_fragment_shader_interlock  // used for ROV type functionality in Vulkan
    VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT};
    addIntoExtChain(sup({VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME}), (VkBaseOutStructure*)&fragmentShaderInterlockFeatures);
#endif
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT};
    addIntoExtChain(ALREADY_PROMPTED_TO_VK_VERSION_1_2_OR_HIGHER, (VkBaseOutStructure*)&descriptorIndexingFeatures);
    VkPhysicalDeviceSamplerYcbcrConversionFeatures ycbcrFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES};
    addIntoExtChain(ALREADY_PROMPTED_TO_VK_VERSION_1_2_OR_HIGHER, (VkBaseOutStructure*)&ycbcrFeatures);
    VkPhysicalDeviceBufferDeviceAddressFeatures enabledBufferDeviceAddressFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
    addIntoExtChain(ALREADY_PROMPTED_TO_VK_VERSION_1_2_OR_HIGHER, (VkBaseOutStructure*)&enabledBufferDeviceAddressFeatures);
#if VK_KHR_ray_tracing_pipeline
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    _mRaytracingSupported = addIntoExtChain(sup({VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME}), (VkBaseOutStructure*)&enabledRayTracingPipelineFeatures);
#endif
#if VK_KHR_acceleration_structure
    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    _mRaytracingSupported &= addIntoExtChain(sup({VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME}), (VkBaseOutStructure*)&enabledAccelerationStructureFeatures);
#endif
#if VK_KHR_ray_query
    VkPhysicalDeviceRayQueryFeaturesKHR enabledRayQueryFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
    _mRaytracingSupported &= addIntoExtChain(sup({VK_KHR_RAY_QUERY_EXTENSION_NAME}), (VkBaseOutStructure*)&enabledRayQueryFeatures);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    _mExternalMemoryExtension = sup({VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME});
#endif
    _mRaytracingSupported &= sup({VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME});

    sup({VK_KHR_SWAPCHAIN_EXTENSION_NAME});

    vkGetPhysicalDeviceFeatures2(_mpAdapter->handle(), &gpuFeatures2);

    // TODO?
    // VkPhysicalDeviceVulkan13Features deviceFeatures13 = {
    //     .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    //     .pNext = nullptr};
    // deviceFeatures13.dynamicRendering = VK_TRUE;

    // Query queues
    u32 quFamPropCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(_mpAdapter->handle(), &quFamPropCount, nullptr);
    std::vector<VkQueueFamilyProperties2> quFamProp2(quFamPropCount, {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2});
    vkGetPhysicalDeviceQueueFamilyProperties2(_mpAdapter->handle(), &quFamPropCount, quFamProp2.data());

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
            AXE_INFO("Ready to be create: FamilyIndex[{}/{}] with flag {}(grap({}), comp({}), trans({})), requested queueCount is {} (total count is {}) (No specified priorities)",
                     i, quFamProp2.size(), queueFamilyFlags,
                     (bool)(queueFamilyFlags & VK_QUEUE_GRAPHICS_BIT),
                     (bool)(queueFamilyFlags & VK_QUEUE_COMPUTE_BIT),
                     (bool)(queueFamilyFlags & VK_QUEUE_TRANSFER_BIT),
                     quCount, quFamProp2[i].queueFamilyProperties.queueCount);

            _mQueueInfos[queueFamilyFlags] = QueueInfo{
                .mAvailableCount = quCount, .mUsedCount = 0, .mFamilyIndex = (u8)i};
        }
    }

    for (u8 i = 0; i < QUEUE_TYPE_COUNT; ++i)
    {
        u8 dontCareOutput1, dontCareOutput2;
        queryAvailableQueueIndex((QueueType)i, _mQueueFamilyIndexes[i], dontCareOutput1, dontCareOutput2);
    }

    // create a logical device
    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &gpuFeatures2,
        .flags                   = 0,
        .queueCreateInfoCount    = (u32)queueCreateInfos.size(),
        .pQueueCreateInfos       = queueCreateInfos.data(),
        .enabledLayerCount       = (u32)readyLayers.size(),
        .ppEnabledLayerNames     = readyLayers.data(),
        .enabledExtensionCount   = (u32)readyExts.size(),
        .ppEnabledExtensionNames = readyExts.data(),
        .pEnabledFeatures        = nullptr};

    auto result = vkCreateDevice(_mpAdapter->handle(), &deviceCreateInfo, nullptr, &_mpHandle);
    AXE_ASSERT(VK_SUCCEEDED(result));
    volkLoadDevice(_mpHandle);
    return true;
}

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
        .vkGetDeviceImageMemoryRequirements      = vkGetDeviceImageMemoryRequirements};

    VmaAllocatorCreateInfo vmaCreateInfo = {
        .flags                       = VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT,  // to be continue
        .physicalDevice              = _mpAdapter->handle(),
        .device                      = _mpHandle,
        .preferredLargeHeapBlockSize = 0,  // default value
        .pAllocationCallbacks        = &g_VkAllocationCallbacks,
        .pDeviceMemoryCallbacks      = nullptr,
        .pHeapSizeLimit              = nullptr,
        .pVulkanFunctions            = &vmaVkFunctions,
        .instance                    = _mpAdapter->backendHandle(),
        .vulkanApiVersion            = 0,  // VK_API_VERSION_1_3,// TODO: load 1.3 func failed
#if VMA_EXTERNAL_MEMORY
        .pTypeExternalMemoryHandleTypes = nullptr,
#endif
    };
    if constexpr (ALREADY_PROMPTED_TO_VK_VERSION_1_2_OR_HIGHER) { vmaCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT; }
    if constexpr (ALREADY_PROMPTED_TO_VK_VERSION_1_2_OR_HIGHER) { vmaCreateInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; }
    bool succ = VK_SUCCEEDED(vmaCreateAllocator(&vmaCreateInfo, &_mpVmaAllocator));
    AXE_CHECK(succ);
    return succ;
}

void VulkanDevice::_createDefaultResource() noexcept
{
    {
        //// Texture
        TextureDesc texDesc{};
        texDesc.pNativeHandle          = nullptr;
        texDesc.pName                  = "Dummy Texture";
        texDesc.mClearValue.rgba       = {0.0, 0.0, 0.0, 0.0};
        texDesc.mFlags                 = TEXTURE_CREATION_FLAG_NONE;
        texDesc.mMipLevels             = 1;
        texDesc.mSampleQuality         = 0;
        texDesc.mFormat                = TinyImageFormat_R8G8B8A8_UNORM;
        texDesc.mStartState            = RESOURCE_STATE_COMMON;

        ///// 1D, 2D, 3D
        const auto createTextureHelper = [this, &texDesc](MSAASampleCount sampleCount, u32 arraySize, u32 width, u32 height, u32 depth, TextureDimension dim)
        {
            texDesc.mWidth                                   = width;
            texDesc.mHeight                                  = height;
            texDesc.mDepth                                   = depth;
            texDesc.mArraySize                               = arraySize;
            texDesc.mSampleCount                             = sampleCount;

            texDesc.mDescriptors                             = DESCRIPTOR_TYPE_TEXTURE;
            this->_mNullDescriptors.mpDefaultTextureSRV[dim] = this->createTexture(texDesc);

            if (sampleCount == MSAA_SAMPLE_COUNT_1)
            {
                texDesc.mDescriptors                             = DESCRIPTOR_TYPE_RW_TEXTURE;
                this->_mNullDescriptors.mpDefaultTextureUAV[dim] = this->createTexture(texDesc);
            }
        };
        createTextureHelper(MSAA_SAMPLE_COUNT_1, 1, 1, 1, 1, TEXTURE_DIM_1D);
        createTextureHelper(MSAA_SAMPLE_COUNT_1, 2, 1, 1, 1, TEXTURE_DIM_1D_ARRAY);
        createTextureHelper(MSAA_SAMPLE_COUNT_1, 1, 2, 2, 1, TEXTURE_DIM_2D);
        createTextureHelper(MSAA_SAMPLE_COUNT_1, 2, 2, 2, 1, TEXTURE_DIM_2D_ARRAY);
        createTextureHelper(MSAA_SAMPLE_COUNT_4, 1, 2, 2, 1, TEXTURE_DIM_2DMS);
        createTextureHelper(MSAA_SAMPLE_COUNT_4, 2, 2, 2, 1, TEXTURE_DIM_2DMS_ARRAY);
        createTextureHelper(MSAA_SAMPLE_COUNT_1, 1, 2, 2, 2, TEXTURE_DIM_3D);

        ///// cube
        texDesc.mWidth                                                      = 2;
        texDesc.mHeight                                                     = 2;
        texDesc.mDepth                                                      = 1;
        texDesc.mSampleCount                                                = MSAA_SAMPLE_COUNT_1;
        texDesc.mDescriptors                                                = DESCRIPTOR_TYPE_TEXTURE_CUBE;

        texDesc.mArraySize                                                  = 6;
        this->_mNullDescriptors.mpDefaultTextureSRV[TEXTURE_DIM_CUBE]       = this->createTexture(texDesc);
        texDesc.mArraySize                                                  = 12;
        this->_mNullDescriptors.mpDefaultTextureSRV[TEXTURE_DIM_CUBE_ARRAY] = this->createTexture(texDesc);

        //// Buffer
        BufferDesc bufDesc{
            .mSize               = sizeof(u32),
            .mpCounterBuffer     = nullptr,
            .mFirstElement       = 0,
            .mElementCount       = 1,
            .mStructStride       = sizeof(u32),
            .mName               = "Dummy Buffer",
            .mAlignment          = 0,
            .mMemoryUsage        = RESOURCE_MEMORY_USAGE_GPU_ONLY,
            .mFlags              = BUFFER_CREATION_FLAG_NONE,
            .mQueueType          = QUEUE_TYPE_MAX,
            .mStartState         = RESOURCE_STATE_COMMON,
            .mICBDrawType        = INDIRECT_ARG_INVALID,
            .mICBMaxCommandCount = 0,
            .mFormat             = TinyImageFormat_R32_UINT,
        };

        bufDesc.mDescriptors                 = DESCRIPTOR_TYPE_BUFFER | DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        _mNullDescriptors.mpDefaultBufferSRV = this->createBuffer(bufDesc);
        bufDesc.mDescriptors                 = DESCRIPTOR_TYPE_RW_BUFFER;
        _mNullDescriptors.mpDefaultBufferUAV = this->createBuffer(bufDesc);
    }

    // TODO: mutex _mNullDescriptors.mSubmitMutex
    /// sampler
    SamplerDesc sampleDesc{};
    _mNullDescriptors.mpDefaultSampler = this->createSampler(sampleDesc);

    // command buffer to transition resources to the correct state
    QueueDesc queueDesc{};
    auto* pQueue = this->requestQueue(queueDesc);

    CmdPoolDesc cmdPoolDesc{};
    cmdPoolDesc.mpUsedForQueue = (Queue*)pQueue;
    auto* pCmdPool             = this->createCmdPool(cmdPoolDesc);

    CmdDesc cmdDesc{};
    cmdDesc.mpCmdPool = pCmdPool;
    auto* pCmd        = this->createCmd(cmdDesc);

    FenceDesc fenceDesc{};
    auto* pFence                                 = this->createFence(fenceDesc);

    _mNullDescriptors.mpInitialTransitionQueue   = pQueue;
    _mNullDescriptors.mpInitialTransitionCmdPool = pCmdPool;
    _mNullDescriptors.mpInitialTransitionCmd     = pCmd;
    _mNullDescriptors.mpInitialTransitionFence   = pFence;

    // TODO: mutex _mNullDescriptors.mInitialTransitionMutex

    for (uint32_t dim = 0; dim < TEXTURE_DIM_COUNT; ++dim)
    {
        if (_mNullDescriptors.mpDefaultTextureSRV[dim])
        {
            this->initial_transition(_mNullDescriptors.mpDefaultTextureSRV[dim], RESOURCE_STATE_SHADER_RESOURCE);
        }
        if (_mNullDescriptors.mpDefaultTextureUAV[dim])
        {
            this->initial_transition(_mNullDescriptors.mpDefaultTextureUAV[dim], RESOURCE_STATE_UNORDERED_ACCESS);
        }
    }
}

void VulkanDevice::_destroyDefaultResource() noexcept
{
    for (Texture* pTex : _mNullDescriptors.mpDefaultTextureSRV) { destroyTexture(pTex); }
    for (Texture* pTex : _mNullDescriptors.mpDefaultTextureUAV) { destroyTexture(pTex); }
    destroyBuffer(_mNullDescriptors.mpDefaultBufferSRV);
    destroyBuffer(_mNullDescriptors.mpDefaultBufferUAV);
    destroySampler(_mNullDescriptors.mpDefaultSampler);
    destroyFence(_mNullDescriptors.mpInitialTransitionFence);
    releaseQueue(_mNullDescriptors.mpInitialTransitionQueue);
    destroyCmd(_mNullDescriptors.mpInitialTransitionCmd);
    destroyCmdPool(_mNullDescriptors.mpInitialTransitionCmdPool);
}

VulkanDevice::VulkanDevice(VulkanAdapter* pAdapter, DeviceDesc& desc) noexcept
    : _mpAdapter(pAdapter), _mShaderModel(desc.mShaderModel)
{
    if (_createLogicalDevice(desc))
    {
        if (_initVMA())
        {
            _createDefaultResource();
        }
    }
}

VulkanDevice::~VulkanDevice() noexcept
{
    AXE_ASSERT(_mpHandle);
    {
        _destroyDefaultResource();
    }
    {
        vmaDestroyAllocator(_mpVmaAllocator);
    }
    {
        vkDeviceWaitIdle(_mpHandle);          // block until all processing on all queues of a given device is finished
        vkDestroyDevice(_mpHandle, nullptr);  // all queues associated with it are destroyed automatically
    }
}

void VulkanDevice::requestQueueIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) noexcept
{
    bool isConsume = queryAvailableQueueIndex(quType, outQuFamIndex, outQuIndex, outFlag);
    if (isConsume) { _mQueueInfos[outFlag].mUsedCount++; }
}

bool VulkanDevice::queryAvailableQueueIndex(QueueType quType, u8& outQuFamIndex, u8& outQuIndex, u8& outFlag) const noexcept
{
    bool found                   = false;
    bool willConsumeDedicatedOne = false;
    u8 quFamIndex = U8_MAX, quIndex = U8_MAX;
    auto requiredFlagBit                = VK_QUEUE_FLAG_BITS_MAX_ENUM;
    const auto allSupportedVkQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT;
    switch (quType)
    {
        case QUEUE_TYPE_GRAPHICS: requiredFlagBit = VK_QUEUE_GRAPHICS_BIT; break;
        case QUEUE_TYPE_TRANSFER: requiredFlagBit = VK_QUEUE_TRANSFER_BIT; break;
        case QUEUE_TYPE_COMPUTE: requiredFlagBit = VK_QUEUE_COMPUTE_BIT; break;
        default: AXE_ERROR("Unsupported queue type {}", reflection::enum_name(quType)); break;
    }

    // Try to find a dedicated queue of this type
    u32 minSupportTypeCount = U32_MAX;
    for (u32 flag = 0; flag < _mQueueInfos.size(); ++flag)
    {
        auto& info = _mQueueInfos[flag];
        if ((requiredFlagBit & flag) && info.mUsedCount < info.mAvailableCount)
        {
            u32 curTypeCount = std::popcount(flag & allSupportedVkQueueFlags);
            if (requiredFlagBit & VK_QUEUE_GRAPHICS_BIT)
            {
                minSupportTypeCount     = curTypeCount;
                quFamIndex              = info.mFamilyIndex;
                quIndex                 = 0;
                willConsumeDedicatedOne = false;
                outFlag                 = flag;
                found                   = true;
                break;  // always return same one if graphics queue
            }

            if (minSupportTypeCount > curTypeCount)
            {
                minSupportTypeCount     = curTypeCount;
                quFamIndex              = info.mFamilyIndex;
                quIndex                 = info.mUsedCount;
                willConsumeDedicatedOne = true;
                outFlag                 = flag;
                found                   = true;

                if (minSupportTypeCount == 1) { break; }  // found dedicated one
            }
        }
    }

    // Choose default queue if all tries fail.
    if (!found)
    {
        for (u32 flag = 0; flag < _mQueueInfos.size(); ++flag)
        {
            if (_mQueueInfos[flag].mFamilyIndex == 0)
            {
                outFlag = flag;
                break;
            }
        }
        quFamIndex = 0;
        quIndex    = 0;
        AXE_WARN("Not found queue of {}, using default one(familyIndex=0.queueIndex=0)", reflection::enum_name(quType));
    }
    else
    {
        AXE_INFO("Found queue of {} (familyIndex={}, flag={}, isDedicated={}, queueIndex={}/{})",
                 reflection::enum_name(quType), quFamIndex, outFlag, (bool)(minSupportTypeCount == 1),
                 quIndex, _mQueueInfos[outFlag].mAvailableCount);
    }
    outQuFamIndex = quFamIndex;
    outQuIndex    = quIndex;
    return willConsumeDedicatedOne;
}

void VulkanDevice::initial_transition(Texture* pTexture, ResourceState startState) noexcept
{
    // TODO acquire_mutex()
    _mNullDescriptors.mpInitialTransitionCmdPool->reset();

    _mNullDescriptors.mpInitialTransitionCmd->begin();

    TextureBarrier texBarrier;
    texBarrier.mpTexture                                = pTexture;
    texBarrier.mImageBarrier.mBarrierInfo.mCurrentState = RESOURCE_STATE_UNDEFINED,
    texBarrier.mImageBarrier.mBarrierInfo.mNewState     = startState;
    std::pmr::vector<TextureBarrier> texBarriers{texBarrier};
    _mNullDescriptors.mpInitialTransitionCmd->resourceBarrier(&texBarriers, nullptr, nullptr);

    _mNullDescriptors.mpInitialTransitionCmd->end();

    QueueSubmitDesc submitDesc;
    submitDesc.mCmds.push_back(_mNullDescriptors.mpInitialTransitionCmd);
    submitDesc.mpSignalFence = _mNullDescriptors.mpInitialTransitionFence;
    _mNullDescriptors.mpInitialTransitionQueue->submit(submitDesc);
    _mNullDescriptors.mpInitialTransitionFence->wait();
    // TODO: release mutex
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