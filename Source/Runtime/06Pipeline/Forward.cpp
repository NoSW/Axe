#include <06Pipeline/Forward.hpp>
#include <00Core/Window/Window.hpp>

namespace axe::pipeline
{

bool Forward::init(PipelineDesc& desc) noexcept
{
    _mWidth   = desc.mpWindow->width();
    _mHeight  = desc.mpWindow->height();
    _mpWindow = desc.mpWindow;

    // Backend
    rhi::BackendDesc backendDesc{.mAppName = desc.mAppName};
    _mpBackend = rhi::createBackend(rhi::GRAPHICS_API_VULKAN, backendDesc);

    // Adapter
    rhi::AdapterDesc adapterDesc{};
    _mpAdapter = _mpBackend->requestAdapter(adapterDesc);

    // Device
    rhi::DeviceDesc deviceDesc{};
    _mpDevice = _mpAdapter->requestDevice(deviceDesc);

    // Queue
    rhi::QueueDesc queueDesc{
        .mType = rhi::QUEUE_TYPE_GRAPHICS,
        .mFlag = rhi::QUEUE_FLAG_NONE};

    _mpGraphicsQueue = _mpDevice->requestQueue(queueDesc);
    if (!_mpGraphicsQueue) { return false; }

    for (u32 i = 0; i < _IMAGE_COUNT; ++i)
    {
        // CmdPool
        rhi::CmdPoolDesc cmdPoolDesc{.mpUsedForQueue = _mpGraphicsQueue};
        _mpCmdPools[i] = _mpDevice->createCmdPool(cmdPoolDesc);
        if (!_mpCmdPools[i]) { return false; }

        //  Cmd
        rhi::CmdDesc cmdDesc{
            .mpCmdPool  = _mpCmdPools[i],
            .mCmdCount  = 1,
            .mSecondary = false};
        _mpCmds[i] = _mpDevice->createCmd(cmdDesc);
        if (!_mpCmds[i]) { return false; }

        // Fence
        rhi::FenceDesc fenceDesc{.mIsSignaled = false};
        _mpRenderCompleteFences[i] = _mpDevice->createFence(fenceDesc);
        if (!_mpRenderCompleteFences[i]) { return false; }

        // Semaphore
        rhi::SemaphoreDesc semaphoreDesc{};
        _mpRenderCompleteSemaphores[i] = _mpDevice->createSemaphore(semaphoreDesc);
        if (!_mpRenderCompleteSemaphores[i]) { return false; }
    }
    rhi::SemaphoreDesc semaphoreDesc{};
    _mpImageAcquiredSemaphore = _mpDevice->createSemaphore(semaphoreDesc);
    if (!_mpImageAcquiredSemaphore) { return false; }

    rhi::SamplerDesc samplerDesc{};
    auto* sampler = _mpDevice->createSampler(samplerDesc);
    _mpDevice->destroySampler(sampler);
    AXE_ASSERT(sampler == nullptr);
    // frameIndex
    _mFrameIndex = 0;

    // set textures

    // set vertex buffers

    return true;
}

bool Forward::exit() noexcept
{
    bool succ = true;
    _mpDevice->destroySemaphore(_mpImageAcquiredSemaphore);
    for (u32 i = 0; i < _IMAGE_COUNT; ++i)
    {
        succ = _mpDevice->destroySemaphore(_mpRenderCompleteSemaphores[i]) ? succ : false;
        succ = _mpDevice->destroyFence((_mpRenderCompleteFences[i])) ? succ : false;
        succ = _mpDevice->destroyCmd(_mpCmds[i]) ? succ : false;
        succ = _mpDevice->destroyCmdPool(_mpCmdPools[i]) ? succ : false;
    }
    succ = _mpDevice->releaseQueue(_mpGraphicsQueue) ? succ : false;

    _mpAdapter->releaseDevice(_mpDevice);
    _mpBackend->releaseAdapter(_mpAdapter);
    rhi::destroyBackend(_mpBackend);
    return succ;
}

bool Forward::load(LoadFlag loadFlag) noexcept
{
    if (loadFlag & LOAD_FLAG_SHADER)
    {
        // addShaders();
        // addRootSignatures();
        // addDescriptorSets();
    }

    if (loadFlag & (LOAD_FLAG_RESIZE | LOAD_FLAG_RENDER_TARGET))
    {
        rhi::SwapChainDesc swapchainDesc{
            .mpWindow         = _mpWindow,
            .mpPresentQueue   = _mpGraphicsQueue,
            .mImageCount      = _IMAGE_COUNT,
            .mWidth           = _mWidth,
            .mHeight          = _mHeight,
            .mColorClearValue = rhi::ClearValue{
                .rgba{1.0f, 0.8f, 0.4f, 0.0f},
            }};
        _mpSwapChain = _mpDevice->createSwapChain(swapchainDesc);
        if (!_mpSwapChain) { return false; }

        // if (!addDepthBuffer())
        // {
        //     return false;
        // }
    }

    if (loadFlag & (LOAD_FLAG_SHADER | LOAD_FLAG_RENDER_TARGET))
    {
        // addPipelines();
    }
    // prepareDescriptorSets();

    return true;
}

bool Forward::unload(LoadFlag loadFlag) noexcept
{
    bool succ = true;
    _mpGraphicsQueue->waitIdle();

    if (loadFlag & (LOAD_FLAG_SHADER | LOAD_FLAG_RENDER_TARGET))
    {
        // removePipeline();
    }

    if (loadFlag & (LOAD_FLAG_RESIZE | LOAD_FLAG_RENDER_TARGET))
    {
        succ = _mpDevice->destroySwapChain(_mpSwapChain) ? succ : false;

        // succ = _mpBackend->removeRenderTarget(depthBuffer) ? succ : false;
    }

    if (loadFlag & LOAD_FLAG_SHADER)
    {
        // removeDescriptorSets();
        // removeRootSignatures();
        // removeShaders();
    }

    return true;
}

void Forward::update() noexcept
{
}

void Forward::draw() noexcept
{
    // swapchain
    u32 swapchainImageIndex = 0;
    _mpSwapChain->acquireNextImage(_mpImageAcquiredSemaphore, swapchainImageIndex);

    if (swapchainImageIndex == -1)
    {
        AXE_ERROR("Resized, but not reload swapchain");
    }

    auto* cmdPool   = _mpCmdPools[_mFrameIndex];
    auto* cmd       = _mpCmds[_mFrameIndex];
    auto* fence     = _mpRenderCompleteFences[_mFrameIndex];
    auto* semaphore = _mpRenderCompleteSemaphores[_mFrameIndex];

    // Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
    if (fence->status() == rhi::FENCE_STATUS_INCOMPLETE) { fence->wait(); }

    // reset cmd pool for this frame
    cmdPool->reset();

    // begin cmd
    cmd->begin();

    // record the screen cleaning command
    std::vector<rhi::RenderTargetBarrier> renderTargetBarriers = {rhi::RenderTargetBarrier{
        .mImageBarrier{
            .mBarrierInfo{
                .mCurrentState = rhi::RESOURCE_STATE_PRESENT,
                .mNewState     = rhi::RESOURCE_STATE_RENDER_TARGET,
            },
        },
        .mpRenderTarget = nullptr,
    }};
    cmd->setViewport(0, 0, _mWidth, _mHeight, 0.0, 1.0);  // TODO: width & height
    cmd->setScissor(0, 0, _mWidth, _mHeight);             // TODO: width & height
    cmd->end();

    // submit
    rhi::QueueSubmitDesc quSubmitDesc{.mSubmitDone = false};
    quSubmitDesc.mCmds.push_back(cmd);
    quSubmitDesc.mSignalSemaphores.push_back(semaphore);
    quSubmitDesc.mWaitSemaphores.push_back(_mpImageAcquiredSemaphore);
    quSubmitDesc.mpSignalFence = fence;
    _mpGraphicsQueue->submit(quSubmitDesc);

    // present
    rhi::QueuePresentDesc quPresentDesc{.mSubmitDone = true};
    quPresentDesc.mWaitSemaphores.push_back(_mpImageAcquiredSemaphore);
    quPresentDesc.mpSwapChain = _mpSwapChain;
    _mpGraphicsQueue->present(quPresentDesc);
}

}  // namespace axe::pipeline