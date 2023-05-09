#include "VulkanBuffer.hxx"
#include "VulkanDevice.hxx"

#include "00Core/Math/Math.hpp"

#include <volk.h>
#include <vk_mem_alloc.h>

namespace axe::rhi
{
bool VulkanBuffer::_create(const BufferDesc& desc) noexcept
{
    AXE_ASSERT(desc.size);

    // Align the buffer size to multiples of the dynamic uniform buffer minimum size

    u64 allocSize = desc.size;
    if ((bool)(desc.descriptorType & DescriptorTypeFlag::UNIFORM_BUFFER))
    {
        allocSize = math::round_up(allocSize, _mpDevice->_mpAdapter->requestGPUSettings().uniformBufferAlignment);
    }

    VkBufferCreateInfo bufCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size  = allocSize,
        .usage = to_buffer_usage(desc.descriptorType, desc.format != TinyImageFormat_UNDEFINED) |

                 // Buffer can be used as dest in a transfer command (Uploading data to a storage buffer, Readback query data)
                 ((desc.memoryUsage == ResourceMemoryUsage::GPU_ONLY || desc.memoryUsage == ResourceMemoryUsage::GPU_TO_CPU) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0),
        .sharingMode           = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr

    };

    VmaAllocationCreateInfo allocCreateInfo{
        .flags          = 0,
        .usage          = (VmaMemoryUsage)desc.memoryUsage,
        .requiredFlags  = (VkMemoryPropertyFlags)(((bool)(desc.flags & BufferCreationFlags::OWN_MEMORY_BIT) ? VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT : 0) |
                                                 (bool(desc.flags & BufferCreationFlags::PERSISTENT_MAP_BIT) ? VMA_ALLOCATION_CREATE_MAPPED_BIT : 0)),
        .preferredFlags = 0,
        .memoryTypeBits = (VkMemoryPropertyFlags)((bool(desc.flags & BufferCreationFlags::HOST_VISIBLE_VKONLY) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : 0) |
                                                  (bool(desc.flags & BufferCreationFlags::HOST_COHERENT_VKONLY) ? VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : 0)),
        .pool           = nullptr,
        .pUserData      = nullptr,
        .priority       = 0,  // ignored
    };

    VmaAllocationInfo allocInfo{};
    if (VK_FAILED(vmaCreateBuffer(_mpDevice->_mpVmaAllocator, &bufCreateInfo, &allocCreateInfo, &_mpHandle, &_mpVkAllocation, &allocInfo)))
    {
        return false;
    }

    // create buffer view (uniform, storage)
    const VkBufferViewCreateInfo viewCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .pNext  = nullptr,
        .flags  = 0,
        .buffer = _mpHandle,
        .format = to_vk_enum(desc.format),
        .offset = desc.structStride * desc.firstElement,
        .range  = desc.structStride * desc.elementCount,
    };

    VkFormatProperties formatProps{};
    vkGetPhysicalDeviceFormatProperties(_mpDevice->_mpAdapter->handle(), viewCreateInfo.format, &formatProps);

    if (bufCreateInfo.usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT)
    {
        if (!(formatProps.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))
        {
            AXE_WARN("Failed to create uniform texel buffer view for format {}", reflection::enum_name(desc.format));
        }
        else
        {
            if (VK_FAILED(vkCreateBufferView(_mpDevice->handle(), &viewCreateInfo, nullptr, &_mpVkUniformTexelView)))
            {
                return false;
            }
        }
    }
    if (bufCreateInfo.usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
    {
        if (!(formatProps.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
        {
            AXE_WARN("Failed to create storage texel buffer view for format {}", reflection::enum_name(desc.format));
        }
        else
        {
            if (VK_FAILED(vkCreateBufferView(_mpDevice->handle(), &viewCreateInfo, nullptr, &_mpVkStorageTexelView)))
            {
                return false;
            }
        }
    }

    _mSize              = desc.size;
    _mMemoryUsage       = (u32)desc.memoryUsage;
    _mDescriptors       = (u32)desc.descriptorType;
    _mpCPUMappedAddress = allocInfo.pMappedData;
    if ((desc.descriptorType & DescriptorTypeFlag::BUFFER) || (desc.descriptorType & DescriptorTypeFlag::BUFFER_RAW))
    {
        _mOffset = desc.structStride * desc.firstElement;
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