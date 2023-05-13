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

    // frameIndex
    _mFrameIndex = 0;

    // set textures
    rhi::SamplerDesc samplerDesc{
        .minFilter  = rhi::FilterType::LINEAR,
        .magFilter  = rhi::FilterType::LINEAR,
        .mipMapMode = rhi::MipMapMode::NEAREST,
        .addressU   = rhi::AddressMode::CLAMP_TO_EDGE,
        .addressV   = rhi::AddressMode::CLAMP_TO_EDGE,
        .addressW   = rhi::AddressMode::CLAMP_TO_EDGE,
    };
    _mpStaticSampler = _mpDevice->createSampler(samplerDesc);

    // set vertex buffers

    return true;
}

bool Forward::exit() noexcept
{
    bool succ = true;
    _mpDevice->destroySampler(_mpStaticSampler);
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
        // addShader
        {
            rhi::ShaderDesc shaderDesc;
            shaderDesc.mStages.push_back(rhi::ShaderStageDesc{.mStage = rhi::ShaderStageFlag::VERT});
            shaderDesc.mStages.push_back(rhi::ShaderStageDesc{.mStage = rhi::ShaderStageFlag::FRAG});

            shaderDesc.mStages[0].mRelaFilePath = "Shaders/Basic.vert.glsl";
            shaderDesc.mStages[1].mRelaFilePath = "Shaders/Basic.frag.glsl";
            shaderDesc.setDebugLabel("Basic");
            _mpBasicShader                      = _mpDevice->createShader(shaderDesc);

            shaderDesc.mStages[0].mRelaFilePath = "Shaders/Skybox/Skybox.vert.glsl";
            shaderDesc.mStages[1].mRelaFilePath = "Shaders/Skybox/Skybox.frag.glsl";
            shaderDesc.setDebugLabel("Skybox");
            _mpSkyboxShader = _mpDevice->createShader(shaderDesc);
        }

        // addRootSignature
        {
            rhi::RootSignatureDesc rootDesc;
            rootDesc.mShaders.push_back(_mpBasicShader);
            rootDesc.mShaders.push_back(_mpSkyboxShader);
            rootDesc.mStaticSamplersMap["uSampler0"] = _mpStaticSampler;
            _mpRootSignature                         = _mpDevice->createRootSignature(rootDesc);
        }

        // addDescriptorSet
        {
            rhi::DescriptorSetDesc descSetDesc{
                .mpRootSignature = _mpRootSignature,
                .mMaxSet         = 1,
                .updateFrequency = rhi::DescriptorUpdateFrequency::NONE};
            descSetDesc.setDebugLabel("Texture");

            _mpDescriptorSetTexture     = _mpDevice->createDescriptorSet(descSetDesc);

            descSetDesc.mMaxSet         = _IMAGE_COUNT * 2;
            descSetDesc.updateFrequency = rhi::DescriptorUpdateFrequency::PER_FRAME;
            descSetDesc.setDebugLabel("Uniform");
            _mpDescriptorSetUniform = _mpDevice->createDescriptorSet(descSetDesc);
        }
    }

    if ((bool)(loadFlag & (LoadFlag::RESIZE | LoadFlag::RENDER_TARGET)))
    {
        // addSwapchain
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
        }

        // addDepthBuffer
        {
            rhi::RenderTargetDesc depthRT{
                .flags            = rhi::TextureCreationFlags::ON_TILE,
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
                .pNativeHandle  = nullptr,
                .name           = "DepthBuffer",
            };
            _mpDepthBuffer = _mpDevice->createRenderTarget(depthRT);
            if (_mpDepthBuffer == nullptr) { return false; }
        }
    }

    if ((bool)(loadFlag & (LoadFlag::SHADER | LoadFlag::RENDER_TARGET)))
    {
        _addPipelines();
    }
    // prepareDescriptorSets();

    return true;
}

bool Forward::unload(LoadFlag loadFlag) noexcept
{
    _mpGraphicsQueue->waitIdle();

    if ((bool)(loadFlag & (LoadFlag::SHADER | LoadFlag::RENDER_TARGET)))
    {
        _removePipelines();
    }

    if ((bool)(loadFlag & (LoadFlag::RESIZE | LoadFlag::RENDER_TARGET)))
    {
        _mpDevice->destroySwapChain(_mpSwapChain);
        _mpDevice->destroyRenderTarget(_mpDepthBuffer);
    }

    if ((bool)(loadFlag & LoadFlag::SHADER))
    {
        _mpDevice->destroyDescriptorSet(_mpDescriptorSetUniform);
        _mpDevice->destroyDescriptorSet(_mpDescriptorSetTexture);

        _mpDevice->destroyRootSignature(_mpRootSignature);

        _mpDevice->destroyShader(_mpBasicShader);
        _mpDevice->destroyShader(_mpSkyboxShader);
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

void Forward::_addPipelines() noexcept
{
    // layout and pipeline for sphere draw
    u32 offset   = 0;
    u32 location = 0;
    rhi::VertexLayout vertexLayout{};
    vertexLayout.attrCount         = 6;
    vertexLayout.attrs[0].semantic = rhi::ShaderSemantic::POSITION;
    vertexLayout.attrs[0].format   = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.attrs[0].binding  = 0;
    vertexLayout.attrs[0].location = location++;
    vertexLayout.attrs[0].offset   = offset;
    vertexLayout.attrs[0].rate     = rhi::VertexAttribRate::VERTEX;
    offset += 3 * sizeof(float);

    vertexLayout.attrs[1].semantic = rhi::ShaderSemantic::NORMAL;
    vertexLayout.attrs[1].format   = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.attrs[1].binding  = 0;
    vertexLayout.attrs[1].location = location++;
    vertexLayout.attrs[1].offset   = offset;
    vertexLayout.attrs[1].rate     = rhi::VertexAttribRate::VERTEX;
    offset += 3 * sizeof(float);

    vertexLayout.attrs[2].semantic = rhi::ShaderSemantic::TANGENT;
    vertexLayout.attrs[2].format   = TinyImageFormat_R32G32B32_SFLOAT;
    vertexLayout.attrs[2].binding  = 0;
    vertexLayout.attrs[2].location = location++;
    vertexLayout.attrs[2].offset   = offset;
    vertexLayout.attrs[2].rate     = rhi::VertexAttribRate::VERTEX;
    offset += 3 * sizeof(float);

    vertexLayout.attrs[3].semantic = rhi::ShaderSemantic::TEXCOORD0;
    vertexLayout.attrs[3].format   = TinyImageFormat_R32G32_SFLOAT;
    vertexLayout.attrs[3].binding  = 0;
    vertexLayout.attrs[3].location = location++;
    vertexLayout.attrs[3].offset   = offset;
    vertexLayout.attrs[3].rate     = rhi::VertexAttribRate::VERTEX;
    offset += 2 * sizeof(float);

    vertexLayout.attrs[4].semantic = rhi::ShaderSemantic::TEXCOORD1;
    vertexLayout.attrs[4].format   = TinyImageFormat_R32G32_SFLOAT;
    vertexLayout.attrs[4].binding  = 0;
    vertexLayout.attrs[4].location = location++;
    vertexLayout.attrs[4].offset   = offset;
    vertexLayout.attrs[4].rate     = rhi::VertexAttribRate::VERTEX;
    offset += 2 * sizeof(float);

    vertexLayout.attrs[5].semantic = rhi::ShaderSemantic::COLOR;
    vertexLayout.attrs[5].format   = TinyImageFormat_R32G32B32A32_SFLOAT;
    vertexLayout.attrs[5].binding  = 0;
    vertexLayout.attrs[5].location = location++;
    vertexLayout.attrs[5].offset   = offset;
    vertexLayout.attrs[5].rate     = rhi::VertexAttribRate::VERTEX;
    offset += 4 * sizeof(float);

    rhi::RasterizerStateDesc rasterizerStateDesc{
        .cullMode = rhi::CullMode::FRONT,
    };

    rhi::DepthStateDesc depthStateDesc{
        .enableDepthTest   = true,
        .enableDepthWrite  = true,
        .enableStencilTest = false,
        .depthFunc         = rhi::CompareMode::GEQUAL};

    auto colorFormats = std::array{_mpSwapChain->getRenderTarget(0)->format()};

    rhi::GraphicsPipelineDesc graphicsDesc{
        .pShaderProgram               = _mpBasicShader,
        .pRootSignature               = _mpRootSignature,
        .pVertexLayout                = &vertexLayout,
        .pBlendState                  = nullptr,
        .pDepthState                  = &depthStateDesc,
        .pRasterizerState             = &rasterizerStateDesc,
        .pColorFormats                = colorFormats.data(),
        .renderTargetCount            = colorFormats.size(),
        .sampleCount                  = _mpSwapChain->getRenderTarget(0)->sampleCount(),
        .sampleQuality                = _mpSwapChain->getRenderTarget(0)->sampleQuality(),
        .depthStencilFormat           = _mpDepthBuffer->format(),
        .primitiveTopology            = rhi::PrimitiveTopology::TRI_LIST,
        .supportIndirectCommandBuffer = false,
    };

    nullable<rhi::VertexLayout> pVertexInput = &vertexLayout;
    rhi::VertexLayout* pDDEBug               = pVertexInput;

    rhi::PipelineDesc desc                   = {
                          .name   = "Basic Pipeline",
                          .type   = rhi::PipelineType::GRAPHICS,
                          .pCache = nullptr};
    desc.pGraphicsDesc             = &graphicsDesc;
    _mpBasicPipeline               = _mpDevice->createPipeline(desc);

    vertexLayout                   = {};
    vertexLayout.attrCount         = 1;
    vertexLayout.attrs[0].semantic = rhi::ShaderSemantic::POSITION;
    vertexLayout.attrs[0].format   = TinyImageFormat_R32G32B32A32_SFLOAT;
    vertexLayout.attrs[0].binding  = 0;
    vertexLayout.attrs[0].location = 0;
    vertexLayout.attrs[0].offset   = 0;
    vertexLayout.attrs[0].rate     = rhi::VertexAttribRate::VERTEX;

    rasterizerStateDesc.cullMode   = rhi::CullMode::NONE;

    graphicsDesc.pShaderProgram    = _mpSkyboxShader;
    graphicsDesc.pDepthState       = nullptr;

    _mpSkyboxPipeline              = _mpDevice->createPipeline(desc);
}

void Forward::_removePipelines() noexcept
{
    _mpDevice->destroyPipeline(_mpSkyboxPipeline);
    _mpDevice->destroyPipeline(_mpBasicPipeline);
}

}  // namespace axe::pipeline