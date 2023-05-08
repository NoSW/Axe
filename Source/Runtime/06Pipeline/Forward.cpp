#include <06Pipeline/Forward.hpp>
#include <00Core/Window/Window.hpp>

namespace axe::pipeline
{

bool Forward::init(PipelineDesc& desc) noexcept
{
    _mWidth   = desc.pWindow->width();
    _mHeight  = desc.pWindow->height();
    _mpWindow = desc.pWindow;

    // Backend
    rhi::BackendDesc backendDesc{.appName = desc.appName};
    _mpBackend = rhi::createBackend(rhi::GraphicsApiFlag::VULKAN, backendDesc);

    // Adapter
    rhi::AdapterDesc adapterDesc{};
    _mpAdapter = _mpBackend->requestAdapter(adapterDesc);

    // Device
    rhi::DeviceDesc deviceDesc{};
    _mpDevice = _mpAdapter->requestDevice(deviceDesc);

    // Queue
    rhi::QueueDesc queueDesc{
        .type = rhi::QueueTypeFlag::GRAPHICS,
        .flag = rhi::QueueFlag::NONE};

    _mpGraphicsQueue = _mpDevice->requestQueue(queueDesc);
    if (!_mpGraphicsQueue) { return false; }

    for (u32 i = 0; i < _IMAGE_COUNT; ++i)
    {
        // CmdPool
        rhi::CmdPoolDesc cmdPoolDesc{.pUsedForQueue = _mpGraphicsQueue};
        _mpCmdPools[i] = _mpDevice->createCmdPool(cmdPoolDesc);
        if (!_mpCmdPools[i]) { return false; }

        //  Cmd
        rhi::CmdDesc cmdDesc{
            .pCmdPool    = _mpCmdPools[i],
            .cmdCount    = 1,
            .isSecondary = false};
        _mpCmds[i] = _mpDevice->createCmd(cmdDesc);
        if (!_mpCmds[i]) { return false; }

        // Fence
        rhi::FenceDesc fenceDesc{.isSignaled = false};
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
    if ((bool)(loadFlag & LoadFlag::SHADER))
    {
        rhi::ShaderDesc shaderDesc;
        shaderDesc.mStages.push_back(rhi::ShaderStageDesc{.mStage = rhi::ShaderStageFlag::VERT});
        shaderDesc.mStages.push_back(rhi::ShaderStageDesc{.mStage = rhi::ShaderStageFlag::FRAG});

        shaderDesc.mStages[0].mRelaFilePath = "Shaders/Basic.vert.glsl";
        shaderDesc.mStages[1].mRelaFilePath = "Shaders/Basic.frag.glsl";
        shaderDesc.label                    = "Basic";
        _mpBasicShader                      = _mpDevice->createShader(shaderDesc);

        shaderDesc.mStages[0].mRelaFilePath = "Shaders/Skybox/Skybox.vert.glsl";
        shaderDesc.mStages[1].mRelaFilePath = "Shaders/Skybox/Skybox.frag.glsl";
        shaderDesc.label                    = "Skybox";
        _mpSkyboxShader                     = _mpDevice->createShader(shaderDesc);
    }

    if ((bool)(loadFlag & (LoadFlag::RESIZE | LoadFlag::RENDER_TARGET)))
    {
        rhi::SwapChainDesc swapchainDesc{
            .pWindow         = _mpWindow,
            .pPresentQueue   = _mpGraphicsQueue,
            .imageCount      = _IMAGE_COUNT,
            .width           = _mWidth,
            .height          = _mHeight,
            .colorClearValue = rhi::ClearValue{
                .rgba{1.0f, 0.8f, 0.4f, 0.0f},
            }};
        _mpSwapChain = _mpDevice->createSwapChain(swapchainDesc);
        if (_mpSwapChain == nullptr) { return false; }

        rhi::RenderTargetDesc depthRT{
            .flags            = rhi::TextureCreationFlags::ON_TILE | rhi::TextureCreationFlags::VR_MULTIVIEW,
            .width            = _mWidth,
            .height           = _mHeight,
            .depth            = 1,
            .arraySize        = 1,
            .mipLevels        = 0,
            .mMSAASampleCount = rhi::MSAASampleCount::COUNT_1,
            .format           = TinyImageFormat_D32_SFLOAT,
            .startState       = rhi::ResourceStateFlags::DEPTH_WRITE,
            .clearValue{},
            .sampleQuality  = 0,
            .descriptorType = rhi::DescriptorTypeFlag::UNDEFINED,
            .mpNativeHandle = nullptr,
            .mpName         = "DepthBuffer",
        };
        _mpDepthBuffer = _mpDevice->createRenderTarget(depthRT);
        if (_mpDepthBuffer == nullptr) { return false; }
    }

    if ((bool)(loadFlag & (LoadFlag::SHADER | LoadFlag::RENDER_TARGET)))
    {
        // addPipelines();
    }
    // prepareDescriptorSets();

    return true;
}

bool Forward::unload(LoadFlag loadFlag) noexcept
{
    _mpGraphicsQueue->waitIdle();

    if ((bool)(loadFlag & (LoadFlag::SHADER | LoadFlag::RENDER_TARGET)))
    {
        // removePipeline();
    }

    if ((bool)(loadFlag & (LoadFlag::RESIZE | LoadFlag::RENDER_TARGET)))
    {
        _mpDevice->destroySwapChain(_mpSwapChain);
        _mpDevice->destroyRenderTarget(_mpDepthBuffer);
    }

    if ((bool)(loadFlag & LoadFlag::SHADER))
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
    if (fence->status() == rhi::FenceStatus::INCOMPLETE) { fence->wait(); }

    // reset cmd pool for this frame
    cmdPool->reset();

    // begin cmd
    cmd->begin();

    // record the screen cleaning command
    std::vector<rhi::RenderTargetBarrier> renderTargetBarriers = {rhi::RenderTargetBarrier{
        .imageBarrier{
            .barrierInfo{
                .currentState = rhi::ResourceStateFlags::PRESENT,
                .newState     = rhi::ResourceStateFlags::RENDER_TARGET,
            },
        },
        .pRenderTarget = nullptr,
    }};
    cmd->setViewport(0, 0, _mWidth, _mHeight, 0.0, 1.0);
    cmd->setScissor(0, 0, _mWidth, _mHeight);
    cmd->end();

    // submit
    rhi::QueueSubmitDesc quSubmitDesc{.isSubmitDone = false};
    quSubmitDesc.cmds.push_back(cmd);
    quSubmitDesc.signalSemaphores.push_back(semaphore);
    quSubmitDesc.waitSemaphores.push_back(_mpImageAcquiredSemaphore);
    quSubmitDesc.pSignalFence = fence;
    _mpGraphicsQueue->submit(quSubmitDesc);

    // present
    rhi::QueuePresentDesc quPresentDesc{.isSubmitDone = true};
    quPresentDesc.waitSemaphores.push_back(_mpImageAcquiredSemaphore);
    quPresentDesc.pSwapChain = _mpSwapChain;
    _mpGraphicsQueue->present(quPresentDesc);
}

}  // namespace axe::pipeline