#pragma once

#ifndef _WIN32
#error "D3D12 is supported on Windows only!"
#endif

#include <02Rhi/Rhi.hpp>

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
// #include <d3dcompiler.h>
// #include <DirectXMath.h>

// #include <wrl.h>
// #include <shellapi.h>

namespace axe::rhi
{

class D3D12Driver : public Driver
{
public:
    D3D12Driver()                    = default;
    ~D3D12Driver() noexcept override = default;

    bool init(DriverDesc&) noexcept override;

    void exit() noexcept override;

    void* getDriverHandle() noexcept { return nullptr; }  // No need to

private:
    // clang-format off
    template <typename T, typename Desc>
    [[nodiscard]] T* _createHelper(Desc& desc) noexcept
    { auto* p = new T(this); if (!p || !p->_create(desc)) { delete p; p = nullptr;} return p; }

    template <typename T, typename I>
    [[nodiscard]] bool _destroyHelper(I*& p) noexcept
    { if (p && ((T*)p)->_destroy()) { delete p; p = nullptr; } return p == nullptr; }
    // clang-format on

public:
    [[nodiscard]] Semaphore* createSemaphore(SemaphoreDesc& desc) noexcept override { return nullptr; }           //  { return (Semaphore*)_createHelper<D3D12Semaphore>(desc); }
    [[nodiscard]] Fence* createFence(FenceDesc& desc) noexcept override { return nullptr; }                       //  { return (Fence*)_createHelper<D3D12Fence>(desc); }
    [[nodiscard]] Queue* createQueue(QueueDesc& desc) noexcept override { return nullptr; }                       //  { return (Queue*)_createHelper<D3D12Queue>(desc); }
    [[nodiscard]] SwapChain* createSwapChain(SwapChainDesc& desc) noexcept override { return nullptr; }           //  { return (SwapChain*)_createHelper<D3D12SwapChain>(desc); }
    [[nodiscard]] CmdPool* createCmdPool(CmdPoolDesc& desc) noexcept override { return nullptr; }                 //  { return (CmdPool*)_createHelper<D3D12CmdPool>(desc); }
    [[nodiscard]] Cmd* createCmd(CmdDesc& desc) noexcept override { return nullptr; }                             //  { return (Cmd*)_createHelper<D3D12Cmd>(desc); }
    [[nodiscard]] RenderTarget* createRenderTarget(RenderTargetDesc& desc) noexcept override { return nullptr; }  //  { return (RenderTarget*)_createHelper<D3D12RenderTarget>(desc); }
    bool destroySemaphore(Semaphore*& p) noexcept override { return false; }                                      //  { return _destroyHelper<D3D12Semaphore>(p); }
    bool destroyFence(Fence*& p) noexcept override { return false; }                                              //  { return _destroyHelper<D3D12Fence>(p); }
    bool destroyQueue(Queue*& p) noexcept override { return false; }                                              //  { return _destroyHelper<D3D12Queue>(p); }
    bool destroySwapChain(SwapChain*& p) noexcept override { return false; }                                      //  { return _destroyHelper<D3D12SwapChain>(p); }
    bool destroyCmdPool(CmdPool*& p) noexcept override { return false; }                                          //  { return _destroyHelper<D3D12CmdPool>(p); }
    bool destroyCmd(Cmd*& p) noexcept override { return false; }                                                  //  { return _destroyHelper<D3D12Cmd>(p); }
    bool destroyRenderTarget(RenderTarget*& p) noexcept override { return false; }                                //  { return _destroyHelper<D3D12RenderTarget>(p); }

private:
    bool _addDevice() noexcept;

private:
    IDXGIFactory7* _mpDxgiFactory = nullptr;
    IDXGIAdapter1* _mpActiveGPU   = nullptr;
    ID3D12Device* _mpDevice       = nullptr;

    ID3D12Debug* _mpDebug         = nullptr;

    bool _mUseWarpDevice          = false;
};

}  // namespace axe::rhi
