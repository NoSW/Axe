#include "VulkanShader.hxx"
#include "VulkanDevice.hxx"

#include "01Resource/Shader/Shader.hpp"
#include "00Core/IO/IO.hpp"
#include "00Core/Reflection/Enum.hpp"

#include <spirv_common.hpp>
#include <spirv_cross.hpp>

#include <volk.h>
#include <bit>
#include <span>

namespace axe::rhi
{

static TextureDimension to_TextureDimension(spirv_cross::SPIRType::ImageType imageType) noexcept
{
    switch (imageType.dim)
    {
        case spv::Dim::Dim1D: return imageType.arrayed ? TextureDimension::DIM_1D_ARRAY : TextureDimension::DIM_1D;
        case spv::Dim::Dim2D: return imageType.ms ?
                                         (imageType.arrayed ? TextureDimension::DIM_2DMS_ARRAY : TextureDimension::DIM_2DMS) :
                                         (imageType.arrayed ? TextureDimension::DIM_2D_ARRAY : TextureDimension::DIM_2D);
        case spv::Dim::Dim3D: return TextureDimension::DIM_3D;
        case spv::Dim::DimCube: return imageType.arrayed ? TextureDimension::DIM_CUBE_ARRAY : TextureDimension::DIM_CUBE;
        case spv::Dim::DimRect: return TextureDimension::DIM_UNDEFINED;
        case spv::Dim::DimBuffer: return TextureDimension::DIM_UNDEFINED;
        case spv::Dim::DimSubpassData: return TextureDimension::DIM_UNDEFINED;
        default: AXE_ERROR("Unknown texture dim"); return TextureDimension::DIM_UNDEFINED;
    }
}

static bool create_shader_reflection(const std::span<u8>& byteCode,  // spv coe
                                     ShaderStageFlagOneBit shaderStage,
                                     std::string_view glslFilePath,                  // used for print error msg
                                     std::pmr::list<std::pmr::string>& outNamePool,  // all strings will from  shader reflection (i.e., output from this func)
                                     ShaderReflection& outReflection) noexcept
{
    // using SPIRV-Cross
    spirv_cross::Compiler compiler((u32*)byteCode.data(), byteCode.size() / 4);
    const spirv_cross::ShaderResources allResources                 = compiler.get_shader_resources();
    const std::unordered_set<spirv_cross::VariableID> usedResources = compiler.get_active_interface_variables();

    // entry point
    std::pmr::string entryPoint                                     = compiler.get_entry_points_and_stages()[0].name.data();

    // check shader stage
    if (!std::has_single_bit((u32)shaderStage))
    {
        AXE_ERROR("Cannot reflect multi shader stages at a time");
        return false;
    }
    spv::ExecutionModel executionModel = compiler.get_execution_model();
    switch (executionModel)
    {
        case spv::ExecutionModel::ExecutionModelVertex: AXE_ASSERT(shaderStage & ShaderStageFlag::VERT); break;
        case spv::ExecutionModel::ExecutionModelFragment: AXE_ASSERT(shaderStage & ShaderStageFlag::FRAG); break;
        case spv::ExecutionModel::ExecutionModelGLCompute: AXE_ASSERT(shaderStage & ShaderStageFlag::COMP); break;
        case spv::ExecutionModel::ExecutionModelTessellationControl: AXE_ASSERT(shaderStage & ShaderStageFlag::TESC); break;
        case spv::ExecutionModel::ExecutionModelTessellationEvaluation: AXE_ASSERT(shaderStage & ShaderStageFlag::TESE); break;
        case spv::ExecutionModel::ExecutionModelGeometry: AXE_ASSERT(shaderStage & ShaderStageFlag::GEOM); break;
        case spv::ExecutionModel::ExecutionModelRayGenerationNV:
        case spv::ExecutionModel::ExecutionModelIntersectionNV:
        case spv::ExecutionModel::ExecutionModelAnyHitNV:
        case spv::ExecutionModel::ExecutionModelClosestHitNV:
        case spv::ExecutionModel::ExecutionModelMissNV:
        case spv::ExecutionModel::ExecutionModelCallableNV: AXE_ASSERT(shaderStage & ShaderStageFlag::RAYTRACING); break;
        case spv::ExecutionModel::ExecutionModelTaskNV: AXE_ASSERT("Not support task shader yet"); break;
        case spv::ExecutionModel::ExecutionModelMeshNV: AXE_ASSERT("Not support mesh shader yet"); break;
        default: AXE_ERROR("Unknown shader stage"); return false;
    }

    // special info for specified shader stage
    std::pmr::vector<VertexInput> vertexInputs;
    if ((bool)(shaderStage & ShaderStageFlag::COMP))
    {
        spirv_cross::SPIREntryPoint& entryPointInfo = compiler.get_entry_point(entryPoint.data(), executionModel);
        outReflection.numThreadsPerGroup[0]         = entryPointInfo.workgroup_size.x;
        outReflection.numThreadsPerGroup[1]         = entryPointInfo.workgroup_size.y;
        outReflection.numThreadsPerGroup[2]         = entryPointInfo.workgroup_size.z;
    }
    else if ((bool)(shaderStage & ShaderStageFlag::TESC))
    {
        spirv_cross::SPIREntryPoint entryPointInfo = compiler.get_entry_point(entryPoint.data(), executionModel);
        outReflection.numControlPoint              = entryPointInfo.output_vertices;
    }
    else if ((bool)(shaderStage & ShaderStageFlag::VERT))  // we dont care about inputs except for vertex shader
    {
        for (const auto& input : allResources.stage_inputs)
        {
            if (usedResources.find(input.id) != usedResources.end())
            {
                spirv_cross::SPIRType type = compiler.get_type(input.type_id);
                u32 byteCount              = (type.width / 8) * type.vecsize;
                outNamePool.push_back(input.name.data());
                vertexInputs.push_back(VertexInput{.name = outNamePool.back().data(), .size = byteCount});
            }
        }
    }

    // common info

    std::pmr::vector<ShaderResource> shaderResources;
    std::pmr::vector<ShaderVariable> shaderVariables;
    std::pmr::vector<u32> parentIndex;
    const auto extractShaderResource = [shaderStage, &glslFilePath, &compiler, &usedResources, &outNamePool, &shaderResources](
                                           const spirv_cross::SmallVector<spirv_cross::Resource>& resources, DescriptorTypeFlag descriptorType)
    {
        for (const auto& res : resources)
        {
            if (usedResources.find(res.id) == usedResources.end())
            {
#if _DEBUG
                AXE_WARN("{} is not used in {}", res.name, glslFilePath);
#endif
                continue;
            }

            spirv_cross::SPIRType type = compiler.get_type(res.type_id);
            if (type.image.dim == spv::Dim::DimBuffer)
            {
                if (descriptorType == DescriptorTypeFlag::TEXTURE)
                {
                    descriptorType = DescriptorTypeFlag::TEXEL_BUFFER_VKONLY;
                }
                else if (descriptorType == DescriptorTypeFlag::RW_TEXTURE)
                {
                    descriptorType = DescriptorTypeFlag::RW_TEXEL_BUFFER_VKONLY;
                }
            }

            outNamePool.push_back(res.name.data());
            shaderResources.push_back(ShaderResource{
                .name            = outNamePool.back().data(),
                .usedShaderStage = shaderStage,
                .dim             = to_TextureDimension(type.image),
                .type            = descriptorType,
                .mSet            = descriptorType == DescriptorTypeFlag::UNDEFINED ?
                                       U32_MAX :
                                       compiler.get_decoration(res.id, spv::DecorationDescriptorSet),
                .bindingLocation = compiler.get_decoration(res.id, spv::DecorationBinding),
                .size            = type.array.size() ? type.array[0] : 1});
        }
    };
    const auto extractShaderVariable = [shaderStage, &compiler, &outNamePool, &shaderVariables](
                                           const spirv_cross::Resource& res, u32 parentIndex)
    {
        spirv_cross::SPIRType type = compiler.get_type(res.type_id);
        for (u32 i = 0; i < type.member_types.size(); ++i)
        {
            shaderVariables.push_back(ShaderVariable{
                .name        = outNamePool.back().data(),
                .parentIndex = 0,  // TODO
                .offset      = compiler.get_member_decoration(res.base_type_id, i, spv::DecorationOffset),
                .size        = (u32)compiler.get_declared_struct_member_size(type, i),
            });
        }
    };

    extractShaderResource(allResources.storage_buffers, DescriptorTypeFlag::RW_BUFFER);
    //  extractShaderResource(allResources.stage_outputs, ); // filtered
    // extractShaderResource(allResources.subpass_inputs, ); // not use
    extractShaderResource(allResources.storage_images, DescriptorTypeFlag::RW_TEXTURE);
    extractShaderResource(allResources.sampled_images, DescriptorTypeFlag::COMBINED_IMAGE_SAMPLER_VKONLY);
    // extractShaderResource(allResources.atomic_counters, ); // not usable in vulkan
    extractShaderResource(allResources.acceleration_structures, DescriptorTypeFlag::RAY_TRACING);
    // extractShaderResource(allResources.shader_record_buffers, ); // TODO
    extractShaderResource(allResources.separate_images, DescriptorTypeFlag::TEXTURE);
    extractShaderResource(allResources.separate_samplers, DescriptorTypeFlag::SAMPLER);
    // extractShaderResource(allResources.builtin_inputs, ); // not use
    // extractShaderResource(allResources.builtin_outputs, ); // not use

    /// uniform buffer
    for (u32 i = 0; i < allResources.uniform_buffers.size(); ++i)
    {
        auto& res = allResources.uniform_buffers[i];
        if (usedResources.find(res.id) == usedResources.end())
        {
#if _DEBUG
            AXE_WARN("{} is not used in {}", res.name, glslFilePath);
#endif
            continue;
        }

        spirv_cross::SPIRType type = compiler.get_type(res.type_id);
        outNamePool.push_back(res.name.data());
        shaderResources.push_back(ShaderResource{
            .name            = outNamePool.back().data(),
            .usedShaderStage = shaderStage,
            .dim             = to_TextureDimension(type.image),
            .type            = DescriptorTypeFlag::UNIFORM_BUFFER,
            .mSet            = compiler.get_decoration(res.id, spv::DecorationDescriptorSet),
            .bindingLocation = compiler.get_decoration(res.id, spv::DecorationBinding),
            .size            = type.array.size() ? type.array[0] : 1});

        extractShaderVariable(res, shaderResources.size() - 1);
    }

    /// push constant
    for (u32 i = 0; i < allResources.push_constant_buffers.size(); ++i)
    {
        auto& res = allResources.push_constant_buffers[i];
        if (usedResources.find(res.id) == usedResources.end()) { continue; }

        spirv_cross::SPIRType type = compiler.get_type(res.type_id);
        outNamePool.push_back(res.name.data());
        shaderResources.push_back(ShaderResource{
            .name            = outNamePool.back().data(),
            .usedShaderStage = shaderStage,
            .dim             = TextureDimension::DIM_UNDEFINED,
            .type            = DescriptorTypeFlag::ROOT_CONSTANT,
            .mSet            = U32_MAX,
            .bindingLocation = U32_MAX,
            .size            = (u32)compiler.get_declared_struct_size(type)});
        extractShaderVariable(res, shaderResources.size() - 1);
    }

    outReflection.shaderStage       = shaderStage;
    outReflection.entryPoint_VkOnly = std::move(entryPoint);
    outReflection.vertexInputs      = std::move(vertexInputs);
    outReflection.shaderResources   = std::move(shaderResources);
    outReflection.shaderVariables   = std::move(shaderVariables);
    return true;
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
        u8 index                     = bit2id(stageDesc.mStage);
        std::span<u8> shaderByteCode = resource::get_spv_byte_code(stageDesc.mRelaFilePath);
        if (!shaderByteCode.empty())
        {
            shaderReflections.push_back({});
            if (AXE_FAILED(create_shader_reflection(shaderByteCode, stageDesc.mStage, stageDesc.mRelaFilePath, _mNamePool, shaderReflections.back()))) { return false; }
            VkShaderModuleCreateInfo createInfo = {
                .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                .pNext    = nullptr,
                .flags    = 0,
                .codeSize = shaderByteCode.size(),  // TODO: /4?
                .pCode    = (u32*)shaderByteCode.data()};

            VkShaderModule shaderModule = VK_NULL_HANDLE;
            if (VK_FAILED(vkCreateShaderModule(_mpDevice->handle(), &createInfo, nullptr, &_mpHandles[index])))
            {
                AXE_ERROR("Failed to create shader module for shader stage: {}", stageDesc.mRelaFilePath);
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
            mainEntries[i].constantID = desc.mConstants[i].index;
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
            vkDestroyShaderModule(_mpDevice->handle(), handle, nullptr);
        }
    }

    if (_mpSpecializationInfo != nullptr)
    {
        delete[] _mpSpecializationInfo->pMapEntries;
        delete[] (u8*)(_mpSpecializationInfo->pData);
        delete _mpSpecializationInfo;
    }
    return true;
}

}  // namespace axe::rhi