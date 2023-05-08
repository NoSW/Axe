#include "VulkanShader.hxx"
#include "VulkanDevice.hxx"

#include "01Resource/Shader/Shader.hpp"
#include "00Core/IO/IO.hpp"

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
        case spv::Dim::Dim1D: return imageType.arrayed ? TEXTURE_DIM_1D_ARRAY : TEXTURE_DIM_1D;
        case spv::Dim::Dim2D: return imageType.ms ?
                                         (imageType.arrayed ? TEXTURE_DIM_2DMS_ARRAY : TEXTURE_DIM_2DMS) :
                                         (imageType.arrayed ? TEXTURE_DIM_2D_ARRAY : TEXTURE_DIM_2D);
        case spv::Dim::Dim3D: return TEXTURE_DIM_3D;
        case spv::Dim::DimCube: return imageType.arrayed ? TEXTURE_DIM_CUBE_ARRAY : TEXTURE_DIM_CUBE;
        case spv::Dim::DimRect: return TEXTURE_DIM_UNDEFINED;
        case spv::Dim::DimBuffer: return TEXTURE_DIM_UNDEFINED;
        case spv::Dim::DimSubpassData: return TEXTURE_DIM_UNDEFINED;
        default: AXE_ERROR("Unknown texture dim"); return TEXTURE_DIM_UNDEFINED;
    }
}

static bool create_shader_reflection(const std::span<u8>& byteCode, ShaderStageFlag shaderStage, ShaderReflection& outReflection) noexcept
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
        case spv::ExecutionModel::ExecutionModelVertex: AXE_ASSERT(shaderStage & SHADER_STAGE_FLAG_VERT); break;
        case spv::ExecutionModel::ExecutionModelFragment: AXE_ASSERT(shaderStage & SHADER_STAGE_FLAG_FRAG); break;
        case spv::ExecutionModel::ExecutionModelGLCompute: AXE_ASSERT(shaderStage & SHADER_STAGE_FLAG_COMP); break;
        case spv::ExecutionModel::ExecutionModelTessellationControl: AXE_ASSERT(shaderStage & SHADER_STAGE_FLAG_TESC); break;
        case spv::ExecutionModel::ExecutionModelTessellationEvaluation: AXE_ASSERT(shaderStage & SHADER_STAGE_FLAG_TESE); break;
        case spv::ExecutionModel::ExecutionModelGeometry: AXE_ASSERT(shaderStage & SHADER_STAGE_FLAG_GEOM); break;
        case spv::ExecutionModel::ExecutionModelRayGenerationNV:
        case spv::ExecutionModel::ExecutionModelIntersectionNV:
        case spv::ExecutionModel::ExecutionModelAnyHitNV:
        case spv::ExecutionModel::ExecutionModelClosestHitNV:
        case spv::ExecutionModel::ExecutionModelMissNV:
        case spv::ExecutionModel::ExecutionModelCallableNV: AXE_ASSERT(shaderStage & SHADER_STAGE_FLAG_RAYTRACING); break;
        case spv::ExecutionModel::ExecutionModelTaskNV: AXE_ASSERT("Not support task shader yet"); break;
        case spv::ExecutionModel::ExecutionModelMeshNV: AXE_ASSERT("Not support mesh shader yet"); break;
        default: AXE_ERROR("Unknown shader stage"); return false;
    }

    // special info for specified shader stage
    std::pmr::vector<VertexInput> vertexInputs;
    if (shaderStage & SHADER_STAGE_FLAG_COMP)
    {
        spirv_cross::SPIREntryPoint& entryPointInfo = compiler.get_entry_point(entryPoint.data(), executionModel);
        outReflection.mNumThreadsPerGroup[0]        = entryPointInfo.workgroup_size.x;
        outReflection.mNumThreadsPerGroup[1]        = entryPointInfo.workgroup_size.y;
        outReflection.mNumThreadsPerGroup[2]        = entryPointInfo.workgroup_size.z;
    }
    else if (shaderStage & SHADER_STAGE_FLAG_TESC)
    {
        spirv_cross::SPIREntryPoint entryPointInfo = compiler.get_entry_point(entryPoint.data(), executionModel);
        outReflection.mNumControlPoint             = entryPointInfo.output_vertices;
    }
    else if (shaderStage & SHADER_STAGE_FLAG_VERT)  // we dont care about inputs except for vertex shader
    {
        for (const auto& input : allResources.stage_inputs)
        {
            if (usedResources.find(input.id) != usedResources.end())
            {
                spirv_cross::SPIRType type = compiler.get_type(input.type_id);
                u32 byteCount              = (type.width / 8) * type.vecsize;
                vertexInputs.push_back(VertexInput{.mName = input.name.data(), .mSize = byteCount});
            }
        }
    }

    // common info

    std::pmr::vector<ShaderResource> shaderResources;
    std::pmr::vector<ShaderVariable> shaderVariables;
    std::pmr::vector<u32> parentIndex;
    const auto extractShaderResource = [shaderStage, &compiler, &usedResources, &shaderResources](
                                           const spirv_cross::SmallVector<spirv_cross::Resource>& resources, DescriptorType descriptorType)
    {
        for (const auto& res : resources)
        {
            if (usedResources.find(res.id) != usedResources.end()) { continue; }

            spirv_cross::SPIRType type = compiler.get_type(res.type_id);
            if (type.image.dim == spv::Dim::DimBuffer)
            {
                if (descriptorType == DESCRIPTOR_TYPE_TEXTURE)
                {
                    descriptorType = DESCRIPTOR_TYPE_TEXEL_BUFFER_VKONLY;
                }
                else if (descriptorType == DESCRIPTOR_TYPE_RW_TEXTURE)
                {
                    descriptorType = DESCRIPTOR_TYPE_RW_TEXEL_BUFFER_VKONLY;
                }
            }

            shaderResources.push_back(ShaderResource{
                .mName            = res.name,
                .mUsedShaderStage = shaderStage,
                .mDim             = to_TextureDimension(type.image),
                .mType            = descriptorType,
                .mSet             = descriptorType == DESCRIPTOR_TYPE_UNDEFINED ?
                                        U32_MAX :
                                        compiler.get_decoration(res.id, spv::DecorationDescriptorSet),
                .mBindingLocation = compiler.get_decoration(res.id, spv::DecorationBinding),
                .mSize            = type.array.size() ? type.array[0] : 1});
        }
    };
    const auto extractShaderVariable = [shaderStage, &compiler, &shaderVariables](
                                           const spirv_cross::Resource& res, u32 parentIndex)
    {
        spirv_cross::SPIRType type = compiler.get_type(res.type_id);
        for (u32 i = 0; i < type.member_types.size(); ++i)
        {
            shaderVariables.push_back(ShaderVariable{
                .mName        = res.name,
                .mParentIndex = 0,  // TODO
                .mOffset      = compiler.get_member_decoration(res.base_type_id, i, spv::DecorationOffset),
                .mSize        = (u32)compiler.get_declared_struct_member_size(type, i),
            });
        }
    };

    extractShaderResource(allResources.storage_buffers, DESCRIPTOR_TYPE_RW_BUFFER);
    extractShaderResource(allResources.stage_outputs, DESCRIPTOR_TYPE_UNDEFINED);
    // extractShaderResource(allResources.subpass_inputs, ); // not use
    extractShaderResource(allResources.storage_images, DESCRIPTOR_TYPE_RW_TEXTURE);
    extractShaderResource(allResources.sampled_images, DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER_VKONLY);
    // extractShaderResource(allResources.atomic_counters, ); // not usable in vulkan
    extractShaderResource(allResources.acceleration_structures, DESCRIPTOR_TYPE_RAY_TRACING);
    // extractShaderResource(allResources.shader_record_buffers, ); // TODO
    extractShaderResource(allResources.separate_images, DESCRIPTOR_TYPE_TEXTURE);
    extractShaderResource(allResources.separate_samplers, DESCRIPTOR_TYPE_SAMPLER);
    // extractShaderResource(allResources.builtin_inputs, ); // not use
    // extractShaderResource(allResources.builtin_outputs, ); // not use

    /// uniform buffer
    for (u32 i = 0; i < allResources.uniform_buffers.size(); ++i)
    {
        auto& res = allResources.uniform_buffers[i];
        if (usedResources.find(res.id) != usedResources.end()) { continue; }

        spirv_cross::SPIRType type = compiler.get_type(res.type_id);
        shaderResources.push_back(ShaderResource{
            .mName            = res.name,
            .mUsedShaderStage = shaderStage,
            .mDim             = to_TextureDimension(type.image),
            .mType            = DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .mSet             = compiler.get_decoration(res.id, spv::DecorationDescriptorSet),
            .mBindingLocation = compiler.get_decoration(res.id, spv::DecorationBinding),
            .mSize            = type.array.size() ? type.array[0] : 1});

        extractShaderVariable(res, shaderResources.size() - 1);
    }

    /// push constant
    for (u32 i = 0; i < allResources.push_constant_buffers.size(); ++i)
    {
        auto& res = allResources.push_constant_buffers[i];
        if (usedResources.find(res.id) == usedResources.end()) { continue; }

        spirv_cross::SPIRType type = compiler.get_type(res.type_id);
        shaderResources.push_back(ShaderResource{
            .mName            = res.name,
            .mUsedShaderStage = shaderStage,
            .mDim             = TEXTURE_DIM_UNDEFINED,
            .mType            = DESCRIPTOR_TYPE_ROOT_CONSTANT,
            .mSet             = U32_MAX,
            .mBindingLocation = U32_MAX,
            .mSize            = (u32)compiler.get_declared_struct_size(type)});

        extractShaderVariable(res, shaderResources.size() - 1);
    }

    outReflection.mShaderStage       = shaderStage;
    outReflection.mEntryPoint_VkOnly = std::move(entryPoint);
    outReflection.mVertexInputs      = std::move(vertexInputs);
    outReflection.mShaderResources   = std::move(shaderResources);
    outReflection.mShaderVariables   = std::move(shaderVariables);
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
        AXE_ASSERT(std::has_single_bit((u32)stageDesc.mStage));
        u8 index                     = std::countr_zero((u32)stageDesc.mStage);
        std::span<u8> shaderByteCode = resource::get_spv_byte_code(stageDesc.mRelaFilePath);
        if (!shaderByteCode.empty())
        {
            shaderReflections.push_back({});
            if (AXE_FAILED(create_shader_reflection(shaderByteCode, stageDesc.mStage, shaderReflections.back()))) { return false; }
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