#include <06Pipeline/Forward.hpp>
#include <00Core/Window/Window.hpp>

namespace axe::pipeline
{

bool Forward::init(PipelineDesc& desc) noexcept
{
    _mWidth   = desc.mpWindow->width();
    _mHeight  = desc.mpWindow->height();
    _mpWindow = desc.mpWindow;

    // Driver
    _mpDriver = std::make_unique<rhi::VulkanDriver>();
    rhi::DriverDesc driverDesc{.m_appName = desc.mAppName};
    if (!_mpDriver->init(driverDesc)) { return false; }

    // Queue
    rhi::QueueDesc queueDesc{
        .mType = rhi::QUEUE_TYPE_GRAPHICS,
        .mFlag = rhi::QUEUE_FLAG_NONE};
    _mpGraphicsQueue = _mpDriver->createQueue(queueDesc);
    if (!_mpGraphicsQueue) { return false; }

    for (u32 i = 0; i < _IMAGE_COUNT; ++i)
    {
        // CmdPool
        rhi::CmdPoolDesc cmdPoolDesc{.mpUsedForQueue = _mpGraphicsQueue};
        _mpCmdPools[i] = _mpDriver->createCmdPool(cmdPoolDesc);
        if (!_mpCmdPools[i]) { return false; }

        //  Cmd
        rhi::CmdDesc cmdDesc{
            .mpCmdPool  = _mpCmdPools[i],
            .mCmdCount  = 1,
            .mSecondary = false};
        _mpCmds[i] = _mpDriver->createCmd(cmdDesc);
        if (!_mpCmds[i]) { return false; }

        // Fence
        rhi::FenceDesc fenceDesc{.mIsSignaled = false};
        _mpRenderCompleteFences[i] = _mpDriver->createFence(fenceDesc);
        if (!_mpRenderCompleteFences[i]) { return false; }

        // Semaphore
        rhi::SemaphoreDesc semaphoreDesc{};
        _mpRenderCompleteSemaphores[i] = _mpDriver->createSemaphore(semaphoreDesc);
        if (!_mpRenderCompleteSemaphores[i]) { return false; }
    }
    rhi::SemaphoreDesc semaphoreDesc{};
    _mpImageAcquiredSemaphore = _mpDriver->createSemaphore(semaphoreDesc);
    if (!_mpImageAcquiredSemaphore) { return false; }

    // frameIndex
    _mFrameIndex = 0;
    return true;
}

bool Forward::exit() noexcept
{
    bool succ = true;
    _mpDriver->destroySemaphore(_mpImageAcquiredSemaphore);
    for (u32 i = 0; i < _IMAGE_COUNT; ++i)
    {
        succ = _mpDriver->destroySemaphore(_mpRenderCompleteSemaphores[i]) ? succ : false;
        succ = _mpDriver->destroyFence((_mpRenderCompleteFences[i])) ? succ : false;
        succ = _mpDriver->destroyCmd(_mpCmds[i]) ? succ : false;
        succ = _mpDriver->destroyCmdPool(_mpCmdPools[i]) ? succ : false;
    }
    succ = _mpDriver->destroyQueue(_mpGraphicsQueue) ? succ : false;

    _mpDriver->exit();
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
            .mImageCount      = _IMAGE_COUNT,
            .mWidth           = _mWidth,
            .mHeight          = _mHeight,
            .mColorClearValue = rhi::ClearValue{.rgba{1.0f, 0.8f, 0.4f, 0.0f}},
        };
        _mpSwapChain = _mpDriver->createSwapChain(swapchainDesc);
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
        succ = !_mpDriver->destroySwapChain(_mpSwapChain) ? succ : false;

        // succ = !_mpDriver->removeRenderTarget(depthBuffer) ? succ : false;
    }

    if (loadFlag & LOAD_FLAG_SHADER)
    {
        // removeDescriptorSets();
        // removeRootSignatures();
        // removeShaders();
    }

    _mpDriver->destroySwapChain(_mpSwapChain);

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
        .mpRenderTarget = nullptr,
        .mCurrentFlag   = rhi::RESOURCE_FLAG_PRESENT,
        .mNewFlag       = rhi::RESOURCE_FLAG_RENDER_TARGET,
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