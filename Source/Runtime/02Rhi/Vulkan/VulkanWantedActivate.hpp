#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#if !VK_VERSION_1_3  // >=1.3
#error "Please use Vulkan SDK 1.3 or higher"
#endif

namespace axe::rhi
{
// clang-format off
static void filter_unsupported(const std::pmr::vector<const char*>& supported, std::pmr::vector<const char*>& wanted) noexcept
{
    wanted.erase(std::remove_if(wanted.begin(), wanted.end(),
            [supported](const char* wantToActivateLayer)
            {
                bool ret = std::find_if(supported.begin(), supported.end(),
                                [wantToActivateLayer](const char* supportedLayer)
                                { return strcmp(wantToActivateLayer, supportedLayer) == 0; })
                            == supported.end();
                if (ret) { AXE_WARN("Unsupported {}", wantToActivateLayer); }
                return ret;
            }), wanted.end());
}
// clang-format on

static std::pmr::vector<const char*> gsWantedInstanceLayers = {
#if AXE_RHI_VULKAN_ENABLE_DEBUG
    "VK_LAYER_KHRONOS_validation",
#endif
    "VK_LAYER_RENDERDOC_Capture",
};

static std::pmr::vector<const char*> gsWantedInstanceExtensions = {
    // Surface
    VK_KHR_SURFACE_EXTENSION_NAME,
#if _WIN32
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif __linux__
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif __APPLE__
#error "Unsupported platform"
#endif

// For debug
#if AXE_RHI_VULKAN_ENABLE_DEBUG
#if VK_EXT_debug_utils
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#else
#error "Unsupported VK_EXT_debug_utils"
#endif
#endif

    // Memory
    VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,

    // To legally use HDR formats
    VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,

// Multi GPU Extensions
#if VK_KHR_device_group_creation
    VK_KHR_DEVICE_GROUP_CREATION_EXTENSION_NAME,
#endif

    // Property querying extensions
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,

};

static std::pmr::vector<const char*> gWantedDeviceLayers     = {};
static std::pmr::vector<const char*> gWantedDeviceExtensions = {

    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_MAINTENANCE1_EXTENSION_NAME,
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME,
    VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME,
    VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME,
    VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,

#if VK_KHR_draw_indirect_count
    VK_KHR_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
#endif

// Fragment shader interlock extension to be used for ROV type functionality in Vulkan
#if VK_EXT_fragment_shader_interlock
    VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME,
#endif

    // NVIDIA Specific Extensions
    // VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME,

    // AMD Specific Extensions
    VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME,
    VK_AMD_SHADER_BALLOT_EXTENSION_NAME,
    VK_AMD_GCN_SHADER_EXTENSION_NAME,

// Multi GPU Extensions
#if VK_KHR_device_group
    VK_KHR_DEVICE_GROUP_EXTENSION_NAME,
#endif

    // Bindless & None Uniform access Extensions
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
#if VK_KHR_maintenance3  // descriptor indexing depends on this
    VK_KHR_MAINTENANCE3_EXTENSION_NAME,
#endif

// Raytracing
#if VK_KHR_ray_tracing_pipeline && VK_KHR_acceleration_structure
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,

    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

    VK_KHR_RAY_QUERY_EXTENSION_NAME,
#endif

// YCbCr format support
#if VK_KHR_bind_memory2
    // Requirement for VK_KHR_sampler_ycbcr_conversion
    VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
#endif
#if VK_KHR_sampler_ycbcr_conversion
    VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
#if VK_KHR_bind_memory2  // ycbcr conversion depends on this
    VK_KHR_BIND_MEMORY_2_EXTENSION_NAME,
#endif
#endif

    // Nsight Aftermath
    VK_NV_DEVICE_DIAGNOSTICS_CONFIG_EXTENSION_NAME,
    VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,

};

}  // namespace axe::rhi