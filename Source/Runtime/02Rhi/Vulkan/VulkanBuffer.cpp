#include "02Rhi/Vulkan/VulkanBuffer.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include "00Core/Math/Math.hpp"

#include <volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace axe::rhi
{
bool VulkanBuffer::_create(const BufferDesc& desc) noexcept
{
    AXE_ASSERT(desc.mSize);

    // Align the buffer size to multiples of the dynamic uniform buffer minimum size

    u64 allocSize = desc.mSize;
    if (desc.mDescriptors & DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        allocSize = math::round_up(allocSize, _mpDevice->_mpAdapter->requestGPUSettings().mUniformBufferAlignment);
    }

    VkBufferCreateInfo bufCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size  = allocSize,
        .usage = to_buffer_usage(desc.mDescriptors, desc.mFormat != TinyImageFormat_UNDEFINED) |

                 // Buffer can be used as dest in a transfer command (Uploading data to a storage buffer, Readback query data)
                 ((desc.mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_ONLY || desc.mMemoryUsage == RESOURCE_MEMORY_USAGE_GPU_TO_CPU) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0),
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr

    };

    VmaAllocationCreateInfo allocCreateInfo{
        .flags          = 0,
        .usage          = (VmaMemoryUsage)desc.mMemoryUsage,
        .requiredFlags  = (VkMemoryPropertyFlags)((desc.mFlags & BUFFER_CREATION_FLAG_OWN_MEMORY_BIT ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : 0) |
                                                 (desc.mFlags & BUFFER_CREATION_FLAG_PERSISTENT_MAP_BIT ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0)),
        .preferredFlags = 0,
        .memoryTypeBits = (VkMemoryPropertyFlags)((desc.mFlags & BUFFER_CREATION_FLAG_HOST_VISIBLE_VKONLY ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0) |
                                                  (desc.mFlags & BUFFER_CREATION_FLAG_HOST_COHERENT_VKONLY ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0)),
        .pool           = nullptr,
        .pUserData      = nullptr,
        .priority       = 0,  // ignored
    };

    VmaAllocationInfo allocInfo{};
    AXE_CHECK(VK_SUCCEEDED(vmaCreateBuffer(_mpDevice->_mpVmaAllocator, &bufCreateInfo, &allocCreateInfo, &_mpHandle, &_mpVkAllocation, &allocInfo)));

    // create buffer view (uniform, storage)
    const VkBufferViewCreateInfo viewCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .pNext  = nullptr,
        .flags  = 0,
        .buffer = _mpHandle,
        .format = to_vk_enum(desc.mFormat),
        .offset = desc.mStructStride * desc.mFirstElement,
        .range  = desc.mStructStride * desc.mElementCount,
    };

    VkFormatProperties formatProps{};
    vkGetPhysicalDeviceFormatProperties(_mpDevice->_mpAdapter->handle(), viewCreateInfo.format, &formatProps);

    if (bufCreateInfo.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    {
        if (!(formatProps.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))
        {
            AXE_WARN("Failed to create uniform texel buffer view for format {}", reflection::enum_name(desc.mFormat));
        }
        else
        {
            AXE_CHECK(VK_SUCCEEDED(vkCreateBufferView(_mpDevice->handle(), &viewCreateInfo, nullptr, &_mpVkUniformTexelView)));
        }
    }
    if (bufCreateInfo.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    {
        if (!(formatProps.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
        {
            AXE_WARN("Failed to create storage texel buffer view for format {}", reflection::enum_name(desc.mFormat));
        }
        else
        {
            AXE_CHECK(VK_SUCCEEDED(vkCreateBufferView(_mpDevice->handle(), &viewCreateInfo, nullptr, &_mpVkStorageTexelView)));
        }
    }

    _mSize              = desc.mSize;
    _mMemoryUsage       = desc.mMemoryUsage;
    _mDescriptors       = desc.mDescriptors;
    _mpCPUMappedAddress = allocInfo.pMappedData;
    if ((desc.mDescriptors & DESCRIPTOR_TYPE_BUFFER) || (desc.mDescriptors & DESCRIPTOR_TYPE_BUFFER_RAW))
    {
        _mOffset = desc.mStructStride * desc.mFirstElement;
    }

    return true;
}

bool VulkanBuffer::_destroy() noexcept
{
    if (_mpVkStorageTexelView != VK_NULL_HANDLE)
    {
        vkDestroyBufferView(_mpDevice->handle(), _mpVkStorageTexelView, nullptr);
        _mpVkStorageTexelView = VK_NULL_HANDLE;
    }
    if (_mpVkUniformTexelView != VK_NULL_HANDLE)
    {
        vkDestroyBufferView(_mpDevice->handle(), _mpVkUniformTexelView, nullptr);
        _mpVkUniformTexelView = VK_NULL_HANDLE;
    }
    vmaDestroyBuffer(_mpDevice->_mpVmaAllocator, _mpHandle, _mpVkAllocation);
    return true;
}

}  // namespace axe::rhi