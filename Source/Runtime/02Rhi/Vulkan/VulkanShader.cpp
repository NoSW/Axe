#include "02Rhi/Vulkan/VulkanShader.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"

#include "00Core/IO/IO.hpp"

#include <volk/volk.h>
#include <bit>

namespace axe::rhi
{

static bool create_shader_reflection(const std::vector<u8>& byteCode, ShaderStageFlag shaderStage, ShaderReflection& outReflection) noexcept
{
    // TODO: using SPIRV-Cross
    return false;
}

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
    std::pmr::vector<ShaderReflection> shaderReflections;
    shaderReflections.reserve(desc.mStages.size());
    for (const auto& stageDesc : desc.mStages)
    {
        AXE_ASSERT(std::has_single_bit((u32)stageDesc.mStage));
        u8 index = std::countr_zero((u32)stageDesc.mStage);
        std::vector<u8> byteCode;
        if (io::read_file_binary(stageDesc.mFilePath, byteCode))
        {
            shaderReflections.push_back({});
            AXE_CHECK(create_shader_reflection(byteCode, stageDesc.mStage, shaderReflections.back()));
            VkShaderModuleCreateInfo createInfo = {
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext    = nullptr,
                .flags    = 0,
                .codeSize = byteCode.size(),
                .pCode    = (u32*)byteCode.data()};

            VkShaderModule shaderModule = VK_NULL_HANDLE;
            if (VK_FAILED(vkCreateShaderModule(_mpDevice->_mpHandle, &createInfo, nullptr, &_mpHandles[index])))
            {
                AXE_ERROR("Failed to create shader module for shader stage: {}", stageDesc.mFilePath);
                return false;
            }
            _mpEntryNames[index] = stageDesc.mEntryPoint;
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
        for (const auto& constant : desc.mConstants) { totalSize += constant.mBlob.size(); }
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

    create_pipeline_reflection(shaderReflections, _mReflection);

    return true;
}

bool VulkanShader::_destroy() noexcept
{
    for (auto& handle : _mpHandles)
    {
        if (handle != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(_mpDevice->_mpHandle, handle, nullptr);
        }
    }

    if (_mpSpecializationInfo != nullptr)
    {
        delete[] _mpSpecializationInfo->pMapEntries;
        delete[](u8*)(_mpSpecializationInfo->pData);
        delete _mpSpecializationInfo;
    }
    return true;
}

}  // namespace axe::rhi