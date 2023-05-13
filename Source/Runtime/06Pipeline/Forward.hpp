#pragma once
#include <06Pipeline/Pipeline.hpp>

#include <02Rhi/Rhi.hpp>
namespace axe::pipeline
{
class Forward : public Pipeline
{
    AXE_NON_COPYABLE(Forward);

public:
    Forward()                            = default;
    virtual ~Forward() noexcept override = default;
    bool init(PipelineDesc&) noexcept override;
    bool exit() noexcept override;
    bool load(LoadFlag) noexcept override;
    bool unload(LoadFlag) noexcept override;
    void update() noexcept override;
    void draw() noexcept override;

private:
    void _addPipelines() noexcept;
    void _removePipelines() noexcept;

private:
    u32 _mWidth                       = 0;
    u32 _mHeight                      = 0;
    window::Window* _mpWindow         = nullptr;

    static constexpr u32 _IMAGE_COUNT = 3;
    u32 _mFrameIndex                  = 0;
    rhi::Backend* _mpBackend          = nullptr;

    rhi::Adapter* _mpAdapter          = nullptr;
    rhi::Device* _mpDevice            = nullptr;
    rhi::Queue* _mpGraphicsQueue      = nullptr;
    std::array<rhi::CmdPool*, _IMAGE_COUNT> _mpCmdPools{};
    std::array<rhi::Cmd*, _IMAGE_COUNT> _mpCmds{};
    std::array<rhi::Fence*, _IMAGE_COUNT> _mpRenderCompleteFences{};
    std::array<rhi::Semaphore*, _IMAGE_COUNT> _mpRenderCompleteSemaphores{};

    rhi::Semaphore* _mpImageAcquiredSemaphore   = nullptr;
    rhi::SwapChain* _mpSwapChain                = nullptr;
    rhi::RenderTarget* _mpDepthBuffer           = nullptr;

    rhi::Sampler* _mpStaticSampler              = nullptr;

    rhi::Shader* _mpBasicShader                 = nullptr;
    rhi::Shader* _mpSkyboxShader                = nullptr;

    rhi::RootSignature* _mpRootSignature        = nullptr;
    rhi::DescriptorSet* _mpDescriptorSetTexture = nullptr;
    rhi::DescriptorSet* _mpDescriptorSetUniform = nullptr;

    rhi::Pipeline* _mpBasicPipeline             = nullptr;
    rhi::Pipeline* _mpSkyboxPipeline            = nullptr;
};

}  // namespace axe::pipeline