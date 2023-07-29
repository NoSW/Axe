#include "ResourceLoader.hpp"

#include "00Core/Math/Math.hpp"

#include "00Core/OS/OS.hpp"

namespace axe::resource
{

void ResourceLoader::_loop() noexcept
{
    while (_mIsRunning)
    {
        Request req;
        if (_mRequestQueue.try_dequeue(req))
        {
            u32 activeSetIndex = U32_MAX;
            switch (req.type)
            {
                case Request::Type::UPDATE_BUFFER: activeSetIndex = _updateBuffer(req.bufferDesc); break;
                case Request::Type::UPDATE_TEXTURE: activeSetIndex = _updateTexture(req.textureDesc); break;
                default: AXE_ASSERT(0); break;
            }

            if (activeSetIndex != U32_MAX)
            {
                auto& resSet = _mResourceSets[activeSetIndex];
                rhi::QueueSubmitDesc submitDesc;
                if (resSet.isRecording)
                {
                    resSet.pCmd->end();
                    resSet.isRecording = false;
                    submitDesc.cmds.push_back(resSet.pCmd);
                    submitDesc.signalSemaphores.push_back(resSet.pCopyCompletedSemaphore);
                    submitDesc.pSignalFence = resSet.pFence;
                }

                submitDesc.waitSemaphores = std::move(_mpWaitSemaphores);
                _mpQueue->submit(submitDesc);
            }
        }
    }
}

ResourceLoader::ResourceLoader(ResourceLoaderDesc& desc) noexcept
    : _mpDevice(desc.pDevice), _mRequestQueue(desc.estimatedRequestCount), _mStreamerThread(&ResourceLoader::_loop, this)
{
    os::set_thread_name((void*)_mStreamerThread.native_handle(), L"ResourceLoader");

    AXE_ASSERT(desc.pDevice != nullptr);
    AXE_ASSERT(desc.stageBufferSize > 32, "small buffer is meanless");
    AXE_ASSERT(desc.stageBufferCount > 0);

    _mBufferSize = std::max(desc.stageBufferSize, 32u);

    rhi::QueueDesc queueDesc{
        .type     = rhi::QueueTypeFlag::TRANSFER,
        .flag     = rhi::QueueFlag::NONE,
        .priority = rhi::QueuePriority::NORMAL,
    };
    _mpQueue = _mpDevice->requestQueue(queueDesc);

    _mResourceSets.resize(desc.stageBufferCount);
    for (u32 i = 0; i < desc.stageBufferCount; ++i)
    {
        rhi::FenceDesc fenceDesc{.isSignaled = 0};
        fenceDesc.setLabel_DebugActiveOnly("CopyEngineFence");
        _mResourceSets[i].pFence = _mpDevice->createFence(fenceDesc);

        rhi::SemaphoreDesc semaphoreDesc{};
        semaphoreDesc.setLabel_DebugActiveOnly("CopyCompletedSemaphore");
        _mResourceSets[i].pCopyCompletedSemaphore = _mpDevice->createSemaphore(semaphoreDesc);

        rhi::CmdPoolDesc cmdPoolDesc{
            .pUsedForQueue          = _mpQueue,
            .isShortLived           = false,
            .isAllowIndividualReset = false,
        };
        cmdPoolDesc.setLabel_DebugActiveOnly("CopyEngineCmdPool");
        _mResourceSets[i].pCmdPool = _mpDevice->createCmdPool(cmdPoolDesc);

        rhi::CmdDesc cmdDesc{
            .pCmdPool    = _mResourceSets[i].pCmdPool,
            .cmdCount    = 1,
            .isSecondary = false,
        };
        semaphoreDesc.setLabel_DebugActiveOnly("CopyEngineCmd");
        _mResourceSets[i].pCmd = _mpDevice->createCmd(cmdDesc);

        rhi::BufferDesc bufferDesc{
            .size        = _mBufferSize,
            .alignment   = 0,
            .memoryUsage = rhi::ResourceMemoryUsage::CPU_ONLY,
            .flags       = rhi::BufferCreationFlags::PERSISTENT_MAP_BIT,
        };
        bufferDesc.setLabel_DebugActiveOnly("CopyEngineBuffer");
        _mResourceSets[i].pBuffer = _mpDevice->createBuffer(bufferDesc);
    }
}

ResourceLoader::~ResourceLoader() noexcept
{
    while (_mRequestQueue.size_approx() != 0) {}  // wait all pushed requests to be consumed
    _mpQueue->waitIdle();                         // wait all consumed requesst to be submitted
    _mIsRunning = false;                          // skip load loop
                                                  //   _mStreamerThread.join();            // wait ResourceLoaderThread to exit

    for (CopyResourceSet& resSet : _mResourceSets)  // destroy all resources held by this resource loader
    {
        for (rhi::Buffer* pBuffer : resSet.pTempBuffers) { _mpDevice->destroyBuffer(pBuffer); }
        resSet.pFence->wait();  // wait all submitted requests to be done

        _mpDevice->destroyBuffer(resSet.pBuffer);
        _mpDevice->destroyCmd(resSet.pCmd);
        _mpDevice->destroyCmdPool(resSet.pCmdPool);
        _mpDevice->destroySemaphore(resSet.pCopyCompletedSemaphore);
        _mpDevice->destroyFence(resSet.pFence);
    }
    _mpDevice->releaseQueue(_mpQueue);
}

void ResourceLoader::waitIdle() noexcept
{
    while (_mRequestQueue.size_approx() != 0) {}                               // wait all pushed requests to be consumed
    _mpQueue->waitIdle();                                                      // wait all consumed requesst to be submitted
    for (CopyResourceSet& resSet : _mResourceSets) { resSet.pFence->wait(); }  // wait all submitted requests to be done
}

static u32 get_required_size(TinyImageFormat format, u32 width, u32 height, u32 depth, u32 rowStride, u32 sliceStride,
                             u32 baseMipLevel, u32 mipLevels, u32 baseArrayLayer, u32 arrayLayers) noexcept
{
    u32 requiredSize = 0;
    for (u32 s = baseArrayLayer; s < baseArrayLayer + arrayLayers; ++s)
    {
        u32 w = width;
        u32 h = height;
        u32 d = depth;

        for (u32 m = baseMipLevel; m < baseMipLevel + mipLevels; ++m)
        {
            u32 numBytes = 0;
            u32 rowBytes = 0;
            u32 numRows  = 0;

            if (!rhi::get_surface_info(w, h, format, numBytes, rowBytes, numRows)) { return 0; }
            requiredSize += math::round_up(d * math::round_up(rowBytes, rowStride) * numRows, sliceStride);

            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
            d = std::max(1u, d / 2);
        }
    }
    return requiredSize;
}

void ResourceLoader::pushRequest(RequestLoadTextureDesc& loadDesc) noexcept
{
    ImageDDS* pImage = new ImageDDS;
    if (!pImage->reset(loadDesc.filepath)) { return; }

    const u32 arraySize          = pImage->arraySize();
    const u32 mipLevels          = pImage->mipLevels();
    const u32 width              = pImage->width();
    const u32 height             = pImage->height();
    const u32 depth              = pImage->depth();
    const TinyImageFormat format = pImage->format();
    const auto pImageName        = loadDesc.filepath.filename().stem().string();

    // Step 1: Create Texture
    rhi::TextureDesc textureDesc{
        .pNativeHandle = nullptr,
        .pName         = pImageName,
        .clearValue{},
        .flags          = rhi::TextureCreationFlags::SRGB,
        .width          = width,
        .height         = height,
        .depth          = depth,
        .arraySize      = arraySize,
        .mipLevels      = mipLevels,
        .sampleCount    = rhi::MSAASampleCount::COUNT_1,
        .sampleQuality  = 0,
        .format         = format,
        .startState     = rhi::ResourceStateFlags::COMMON,
        .descriptorType = rhi::DescriptorTypeFlag::TEXTURE,
    };
    textureDesc.setLabel_DebugActiveOnly(pImageName);

    if (pImage->isCubemap())
    {
        textureDesc.descriptorType |= rhi::DescriptorTypeFlag::TEXTURE_CUBE;
        textureDesc.arraySize *= 6;
    }
    rhi::Texture* pRegisteredTexture = _mpDevice->createTexture(textureDesc);

    if (pRegisteredTexture != nullptr)
    {
        RequestUpdateTextureDesc desc{
            .pTexture  = pRegisteredTexture,
            .pImageDDS = pImage,
        };
        pushRequest(desc);
    }
    else
    {
        AXE_ERROR("Failed to create texture: {}", pImageName);
    }

    *loadDesc.ppOutRegisteredTexture = pRegisteredTexture;
}

u32 ResourceLoader::_updateTexture(const RequestUpdateTextureDesc& updateDesc) noexcept
{
    AXE_ASSERT(updateDesc.pImageDDS != nullptr);
    AXE_ASSERT(updateDesc.pTexture != nullptr);
    const u32 arraySize                       = updateDesc.pImageDDS->arraySize();
    const u32 mipLevels                       = updateDesc.pImageDDS->mipLevels();
    const u32 width                           = updateDesc.pImageDDS->width();
    const u32 height                          = updateDesc.pImageDDS->height();
    const u32 depth                           = updateDesc.pImageDDS->depth();
    const TinyImageFormat format              = updateDesc.pImageDDS->format();

    // Step 2: Create Staging Buffer
    u32 activeSetIndex                        = _mNextAvailableSet;
    _mNextAvailableSet                        = (_mNextAvailableSet + 1) % _mResourceSets.size();
    CopyResourceSet& resourceSets             = _mResourceSets[activeSetIndex];

    rhi::Buffer* pStagingBuffer               = resourceSets.pBuffer;

    const auto [rowAlignment, sliceAlignment] = rhi::get_alignment(_mpDevice, format);
    const u64 requiredSize                    = get_required_size(format, width, height, depth, rowAlignment, sliceAlignment, 0, mipLevels, 0, arraySize);
    if (pStagingBuffer->size() < requiredSize && pStagingBuffer->addressOfCPU())  // pre-allocated buffer is NOT enough
    {
        AXE_WARN("pre-allocated buffer is not enough or not valid cpu address, requireSize={}, preAllocatedSize={}, cpu address={:#018x}",
                 requiredSize, pStagingBuffer->size(), (u64)pStagingBuffer->addressOfCPU());
        rhi::BufferDesc bufferDesc{
            .size        = requiredSize,
            .alignment   = sliceAlignment,
            .memoryUsage = rhi::ResourceMemoryUsage::CPU_ONLY,
            .flags       = rhi::BufferCreationFlags::PERSISTENT_MAP_BIT,
        };
        bufferDesc.setLabel_DebugActiveOnly("TempStagingBuffer");
        pStagingBuffer = _mpDevice->createBuffer(bufferDesc);
        resourceSets.pTempBuffers.push_back(pStagingBuffer);
    }

    // Step 3: Populate Staging Buffer
    u32 offset = 0;
    for (u32 mip = 0; mip < mipLevels; ++mip)
    {
        u32 mipByteCount = updateDesc.pImageDDS->imageSize(mip);
        memcpy((u8*)pStagingBuffer->addressOfCPU() + offset, updateDesc.pImageDDS->rawData(mip), mipByteCount);
        offset += mipByteCount;
    }

    // Step 4: Upload Data
    rhi::TextureUpdateDesc rhiUpdateDesc = {
        .pSrcBuffer     = pStagingBuffer,
        .pCmd           = resourceSets.pCmd,
        .baseMipLevel   = 0,
        .mipLevels      = mipLevels,
        .baseArrayLayer = 0,
        .layerCount     = arraySize,
    };

    if (!resourceSets.isRecording)
    {
        resourceSets.pFence->wait();     // wait cmd to be done
        resourceSets.pCmdPool->reset();  // rest cmd
        resourceSets.pCmd->begin();
        resourceSets.isRecording = true;
    }

    if (!updateDesc.pTexture->update(rhiUpdateDesc)) { AXE_ERROR("Failed to update"); }
    delete updateDesc.pImageDDS;
    return activeSetIndex;
}

void ResourceLoader::pushRequest(RequestLoadBufferDesc& loadDesc) noexcept
{
    AXE_ASSERT(loadDesc.pData != nullptr);
    AXE_ASSERT(loadDesc.bufferDesc.size > 0);

    rhi::BufferDesc adjustedBufferDesc = loadDesc.bufferDesc;
    adjustedBufferDesc.setLabel_DebugActiveOnly(loadDesc.name);

    if (adjustedBufferDesc.memoryUsage == rhi::ResourceMemoryUsage::GPU_ONLY)
    {
        adjustedBufferDesc.startState = rhi::ResourceStateFlags::COMMON;
    }

    rhi::Buffer* pRegisteredBuffer = _mpDevice->createBuffer(adjustedBufferDesc);

    RequestUpdateBufferDesc updateDesc{
        .pBuffer = pRegisteredBuffer,
        .offset  = 0,
        .pData   = loadDesc.pData,
        .size    = loadDesc.bufferDesc.size,
    };
    pushRequest(updateDesc);
    *loadDesc.ppOutRegisteredBuffer = pRegisteredBuffer;
}

u32 ResourceLoader::_updateBuffer(const RequestUpdateBufferDesc& updateDesc) noexcept
{
    AXE_ASSERT(updateDesc.pBuffer != nullptr);
    AXE_ASSERT(updateDesc.pData != nullptr);
    AXE_ASSERT(updateDesc.size > 0);

    if (updateDesc.size > updateDesc.pBuffer->size() - updateDesc.offset)
    {
        AXE_ERROR("Invalid update buffer size: {} + {} > {}", updateDesc.size, updateDesc.offset, updateDesc.pBuffer->size());
        return U32_MAX;
    }

    u32 activeSetIndex                         = _mNextAvailableSet;
    _mNextAvailableSet                         = (_mNextAvailableSet + 1) % _mResourceSets.size();
    CopyResourceSet& resourceSets              = _mResourceSets[activeSetIndex];
    rhi::Buffer* pCopyFromBuffer               = resourceSets.pBuffer;

    void* pMapped                              = nullptr;
    const u64 stagingBufferSize                = pCopyFromBuffer->size();
    const rhi::ResourceMemoryUsage memoryUsage = updateDesc.pBuffer->memoryUsage();
    bool needToUnmap                           = false;
    if (stagingBufferSize < updateDesc.size || memoryUsage == rhi::ResourceMemoryUsage::GPU_ONLY)
    {
        constexpr u32 DEFAULT_BUFFER_ALIGNMENT = 4;
        rhi::BufferDesc bufferDesc{
            .size        = updateDesc.size,
            .alignment   = DEFAULT_BUFFER_ALIGNMENT,
            .memoryUsage = rhi::ResourceMemoryUsage::CPU_ONLY,
            .flags       = rhi::BufferCreationFlags::PERSISTENT_MAP_BIT,
        };
        bufferDesc.setLabel_DebugActiveOnly("TempStagingBuffer");
        pCopyFromBuffer = _mpDevice->createBuffer(bufferDesc);
        resourceSets.pTempBuffers.push_back(pCopyFromBuffer);
        pMapped = pCopyFromBuffer->addressOfCPU();
    }
    else
    {
        if (updateDesc.pBuffer->addressOfCPU() == nullptr)
        {
            updateDesc.pBuffer->map();
            needToUnmap = true;
        }
        AXE_ASSERT(updateDesc.pBuffer->addressOfCPU() != nullptr);
        pMapped = (u8*)updateDesc.pBuffer->addressOfCPU() + updateDesc.offset;
    }

    AXE_ASSERT(pMapped != nullptr);
    memcpy(pMapped, updateDesc.pData, updateDesc.size);

    if (needToUnmap) { updateDesc.pBuffer->unmap(); }

    if (memoryUsage == rhi::ResourceMemoryUsage::GPU_ONLY || memoryUsage == rhi::ResourceMemoryUsage::GPU_TO_CPU /* CPU_TO_GPU? */)
    {
        if (!resourceSets.isRecording)
        {
            resourceSets.pFence->wait();     // wait cmd to be done
            resourceSets.pCmdPool->reset();  // rest cmd
            resourceSets.pCmd->begin();
            resourceSets.isRecording = true;
        }
        resourceSets.pCmd->copyBuffer(updateDesc.pBuffer, pCopyFromBuffer, 0, updateDesc.offset, updateDesc.size);
    }

    return activeSetIndex;
}

}  // namespace axe::resource
