#pragma once
#include "00Core/Config.hpp"
//////////////////////////////////////////////////////////////////////////////////////////////
//                            common config for graphics api
//////////////////////////////////////////////////////////////////////////////////////////////
#define AXE_02RHI_API_FLAG_NULL      0
#define AXE_02RHI_API_FLAG_VULKAN    (1 << 0)
#define AXE_02RHI_API_FLAG_D3D12     (1 << 1)

#define AXE_02RHI_API_FLAG_AVAILABLE AXE_02RHI_API_FLAG_VULKAN  // TODO: move this to CMake

#define AXE_02RHI_API_USED_VULKAN    (AXE_02RHI_API_FLAG_AVAILABLE | AXE_02RHI_API_FLAG_VULKAN)
#define AXE_02RHI_API_USED_D3D12     (AXE_02RHI_API_FLAG_AVAILABLE | AXE_02RHI_API_FLAG_D3D12)

#if !(AXE_02RHI_API_FLAG_AVAILABLE & 0xff)
#error "No available render api, please check it"
#endif

//////////////////////////////////////////////////////////////////////////////////////////////
//                             config for Vulkan
//////////////////////////////////////////////////////////////////////////////////////////////
// #if AXE_02RHI_API_USED_VULKAN
// // #define AXE_RHI_VULKAN_USE_DISPATCH_TABLE 1
// #endif

#if _DEBUG
#define AXE_RHI_VULKAN_ENABLE_DEBUG 1
#endif

#if _WIN32
#define AXE_RHI_VK_CONFIG_OVERRIDE 0
#else
#define AXE_RHI_VK_CONFIG_OVERRIDE 0
#endif

#define VK_SUCCEEDED(result) (AXE_SUCCEEDED(((VkResult)(result)) == VK_SUCCESS))  // return true if result == VK_SUCCESS else return false and print error
#define VK_FAILED(result)    (AXE_FAILED(((VkResult)(result)) == VK_SUCCESS))     // return false if result == VK_SUCCESS else return true and print error
//////////////////////////////////////////////////////////////////////////////////////////////
//                             config for D3D12
//////////////////////////////////////////////////////////////////////////////////////////////
#if _DEBUG
#define AXE_RHI_D3D12_ENABLE_DEBUG 1
#endif

#define DX_SUCCEEDED(hr) (AXE_SUCCEEDED(((HRESULT)(hr)) >= 0))  // return true if hr >= 0 else return false and print error
#define DX_FAILED(hr)    (AXE_FAILED(((HRESULT)(hr)) >= 0))     // return false if hr >= 0 else return true and print error
// clang-format off
#define D3D12_FREE(p) do { AXE_ASSERT(p); p->Release(); p = nullptr; } while(0)
// clang-format on
