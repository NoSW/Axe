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

    rhi::Semaphore* _mpImageAcquiredSemaphore = nullptr;
    rhi::SwapChain* _mpSwapChain              = nullptr;
    rhi::RenderTarget* _mpDepthBuffer         = nullptr;

    rhi::Shader* _mpBasicShader               = nullptr;
    rhi::Shader* _mpSkyboxShader              = nullptr;
};

}  // namespace axe::pipeline