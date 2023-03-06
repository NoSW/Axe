#include "02Rhi/Rhi.hpp"

#include "02Rhi/Vulkan/VulkanBackend.hpp"
#include "02Rhi/D3D12/D3D12Backend.hpp"

namespace axe::rhi
{

template <typename T>
static Backend* createBackend(BackendDesc& desc) noexcept
{
    return new T(desc);
}

template <typename T>
static void destroyBackend(Backend*& backend) noexcept
{
    delete static_cast<T*>(backend);
    backend = nullptr;
}

Backend* createBackend(GraphicsApi api, BackendDesc& desc) noexcept
{
    switch (api)
    {
        case GRAPHICS_API_VULKAN: return new VulkanBackend(desc);
        case GRAPHICS_API_D3D12: return nullptr;
        case GRAPHICS_API_METAL: return nullptr;
        default:
            AXE_ERROR("Unrecognized graphics api")
            return nullptr;
    }
}

void destroyBackend(Backend*& backend) noexcept
{
    delete backend;
    backend = nullptr;
}

}  // namespace axe::rhi