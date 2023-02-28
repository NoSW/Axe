#pragma once
#include "00Core/Config.hpp"

//////////////////////////////////////////////////////////////////////////////////////////////
//                            common config for graphics api
//////////////////////////////////////////////////////////////////////////////////////////////
#define AXE_02RHI_API_FLAG_NULL      0
#define AXE_02RHI_API_FLAG_VULKAN    (1 << 0)
#define AXE_02RHI_API_FLAG_D3D12     (1 << 1)
#define AXE_02RHI_API_FLAG_METAL     (1 << 2)

#define AXE_02RHI_API_FLAG_AVAILABLE AXE_02RHI_API_FLAG_VULKAN  // TODO: move this to CMake

#define AXE_02RHI_API_USED_VULKAN    (AXE_02RHI_API_FLAG_AVAILABLE | AXE_02RHI_API_FLAG_VULKAN)
#define AXE_02RHI_API_USED_D3D12     (AXE_02RHI_API_FLAG_AVAILABLE | AXE_02RHI_API_FLAG_D3D12)
#define AXE_02RHI_API_USED_METAL     (AXE_02RHI_API_FLAG_AVAILABLE | AXE_02RHI_API_FLAG_METAL)

#if !(AXE_02RHI_API_FLAG_AVAILABLE & 0xff)
#error "No available render api, please check it"
#endif

#if _DEBUG
#define AXE_RHI_ENABLE_DEBUG 1
#else
#define AXE_RHI_ENABLE_DEBUG 0
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//                             config for Vulkan
//////////////////////////////////////////////////////////////////////////////////////////////
#if AXE_02RHI_API_USED_VULKAN
#define AXE_02RHI_VULKAN_USE_DISPATCH_TABLE 0  // 0 or 1
#define AXE_02RHI_TARGET_VULKAN_VERSION     VK_API_VERSION_1_3
#endif

#define AXE_RHI_VULKAN_ENABLE_DEBUG AXE_RHI_ENABLE_DEBUG

#define VK_SUCCEEDED(result)        (((VkResult)(result)) == VK_SUCCESS)
#define VK_FAILED(result)           (!VK_SUCCEEDED(result))

//////////////////////////////////////////////////////////////////////////////////////////////
//                             config for D3D12
//////////////////////////////////////////////////////////////////////////////////////////////
#define AXE_RHI_D3D12_ENABLE_DEBUG  AXE_RHI_ENABLE_DEBUG

#define DX_SUCCEEDED(hr)            (((HRESULT)(hr)) >= 0)
#define DX_FAILED(hr)               (!DX_SUCCEEDED(hr))
// clang-format off
#define D3D12_FREE(p) do { AXE_ASSERT(p); p->Release(); p = nullptr; } while(0)
// clang-format on

//////////////////////////////////////////////////////////////////////////////////////////////
//                             config for Metal
//////////////////////////////////////////////////////////////////////////////////////////////