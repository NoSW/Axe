#include "02Rhi/Vulkan/VulkanShader.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include "00Core/IO/IO.hpp"

#include <volk/volk.h>

namespace axe::rhi
{
bool VulkanShader::_create(ShaderDesc& desc) noexcept
{
    // check model version
    if (desc.mShaderMode > _mpDevice->_mShaderModel)
    {
        AXE_ERROR("Requested shader target({:x}) is higher than the shader target that"
                  " the renderer supports({:x}).Shader wont be compiled ",
                  (u32)desc.mShaderMode, (u32)_mpDevice->_mShaderModel);
        return false;
    }

    // create module
    for (const auto& stage : desc.mStages)
    {
        std::vector<u8> byteCode;
        if (io::read_file_binary(stage.mFilePath, byteCode))
        {
            VkShaderModuleCreateInfo createInfo = {
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext    = nullptr,
                .flags    = 0,
                .codeSize = byteCode.size(),
                .pCode    = (u32*)byteCode.data()};

            VkShaderModule shaderModule = VK_NULL_HANDLE;
            if (VK_FAILED(vkCreateShaderModule(_mpDevice->_mpHandle, &createInfo, nullptr, &shaderModule)))
            {
                AXE_ERROR("Failed to create shader module for shader stage: {}", stage.mFilePath);
                return false;
            }

            _mpHandles[stage.mStage]    = shaderModule;
            _mpEntryNames[stage.mStage] = stage.mEntryPoint;
        }
        else
        {
            return false;
        }
    }

    // record constants
    if (desc.mConstants.size())
    {
        u32 totalSize = 0;
        for (u32 i = 0; i < desc.mConstants.size(); ++i) { totalSize += desc.mConstants[i].mBlob.size(); }
        auto* mainEntries = new VkSpecializationMapEntry[desc.mConstants.size()];
        auto* data        = new u8[totalSize];

        u32 offset        = 0;
        u32 mapEntryCount = desc.mConstants.size();
        for (u32 i = 0; i < mapEntryCount; ++i)
        {
            memcpy(data + offset, desc.mConstants[i].mBlob.data(), desc.mConstants[i].mBlob.size());
            mainEntries[i].size       = desc.mConstants[i].mBlob.size();
            mainEntries[i].constantID = desc.mConstants[i].mIndex;
            mainEntries[i].offset     = offset;
            offset += mainEntries[i].size;
        }

        _mpSpecializationInfo                = new VkSpecializationInfo;
        _mpSpecializationInfo->mapEntryCount = mapEntryCount;
        _mpSpecializationInfo->pMapEntries   = mainEntries;
        _mpSpecializationInfo->dataSize      = totalSize;
        _mpSpecializationInfo->pData         = data;
    }

    return true;
}

bool VulkanShader::_destroy() noexcept
{
    for (auto& handle : _mpHandles)
    {
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(_mpDevice->_mpHandle, handle, nullptr);
            handle = VK_NULL_HANDLE;
        }
    }

    if (_mpSpecializationInfo != VK_NULL_HANDLE)
    {
        delete[] _mpSpecializationInfo->pMapEntries;
        delete[](u8*)(_mpSpecializationInfo->pData);
        delete _mpSpecializationInfo;
    }
    return true;
}

}  // namespace axe::rhi