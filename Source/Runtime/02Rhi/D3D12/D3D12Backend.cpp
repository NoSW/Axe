#include "02Rhi/D3D12/D3D12Backend.hpp"

namespace axe::rhi
{

D3D12Backend::D3D12Backend(BackendDesc& desc) noexcept
{
    u32 dxgiFactoryFlags = 0;
#if AXE_RHI_D3D12_ENABLE_DEBUG
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        if (DX_SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&_mpDebug))))
        {
            _mpDebug->EnableDebugLayer();

            ID3D12Debug1* pDebug1 = NULL;
            if (DX_SUCCEEDED(_mpDebug->QueryInterface(IID_PPV_ARGS(&pDebug1))))
            {
                pDebug1->SetEnableGPUBasedValidation(true);
                pDebug1->Release();
            }

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    DX_SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&_mpDxgiFactory)));
}

D3D12Backend::~D3D12Backend() noexcept
{
    D3D12_FREE(_mpDxgiFactory);
    D3D12_FREE(_mpDevice);
#if AXE_RHI_D3D12_ENABLE_DEBUG
    D3D12_FREE(_mpDebug);
#endif
}

static void get_hardware_adapter(IDXGIFactory7* pFactory, IDXGIAdapter1*& pOutAdapter, bool requestHighPerformanceAdapter)
{
    IDXGIAdapter1* adapter  = nullptr;
    IDXGIFactory7* factory7 = nullptr;
    if (DX_SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory7))))
    {
        for (
            UINT adapterIndex = 0;
            DXGI_ERROR_NOT_FOUND != factory7->EnumAdapterByGpuPreference(
                                        adapterIndex,
                                        requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                                        IID_PPV_ARGS(&adapter));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Don't select the Basic Render Backend adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }

            // Check to see whether the adapter supports Direct3D 12, but don't create the actual device yet.
            if (DX_SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) { break; }
        }
    }
    else
    {
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, (IDXGIAdapter1**)&adapter); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            // Don't select the Basic Render Backend adapter.
            // If you want a software adapter, pass in "/warp" on the command line.
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { continue; }

            // Check to see whether the adapter supports Direct3D 12, but don't create the actual device yet.
            if (DX_SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))) { break; }
        }
    }

    pOutAdapter = adapter;
}

bool D3D12Backend::_addDevice() noexcept
{
    // select gpu
    if (_mUseWarpDevice)
    {
        DX_SUCCEEDED(_mpDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&_mpActiveGPU)));
    }
    else
    {
        get_hardware_adapter(_mpDxgiFactory, _mpActiveGPU, true);
    }

    // create logical device
    if (DX_SUCCEEDED(D3D12CreateDevice(_mpActiveGPU, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_mpDevice))))
    {
        AXE_ERROR("Failed to create d3d12 device");
        return false;
    }

    // check hardware support
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{};
    if (DX_FAILED(_mpDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel))) || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        AXE_ERROR("Shader Model 6.0 is not supported");
        return false;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 features{};
    if (DX_FAILED(_mpDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &features, sizeof(features))))
    {
        AXE_ERROR("Features aren't supported");
        return false;
    }

    return true;
}

}  // namespace axe::rhi