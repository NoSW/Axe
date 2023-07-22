#pragma once

#ifndef _WIN32
#error "Error cmake config: D3D12 is supported on Windows only!"
#endif

#include "02Rhi/Rhi.hpp"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
// #include <d3dcompiler.h>
// #include <DirectXMath.h>

// #include <wrl.h>
// #include <shellapi.h>

namespace axe::rhi
{

class D3D12Backend : public Backend
{
public:
    explicit D3D12Backend(BackendDesc& desc) noexcept;
    ~D3D12Backend() noexcept override;
    Adapter* requestAdapter(AdapterDesc&) noexcept override { return nullptr; }
    void releaseAdapter(Adapter*&) noexcept override {}

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
