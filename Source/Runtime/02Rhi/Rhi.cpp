#include "02Rhi/Rhi.hpp"

#include "02Rhi/Vulkan/VulkanBackend.hxx"

#if _WIN32
#include "02Rhi/D3D12/D3D12Backend.hxx"
#endif

#include "00Core/OS/OS.hpp"

namespace axe::rhi
{

Backend* createBackend(GraphicsApi api, BackendDesc& desc) noexcept
{
    switch (api)
    {
        case GRAPHICS_API_VULKAN: return new VulkanBackend(desc);
        case GRAPHICS_API_D3D12: return nullptr;
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

bool create_pipeline_reflection(std::pmr::vector<ShaderReflection>& shaderRefls, PipelineReflection& outPipelineRefl) noexcept
{
    if (shaderRefls.empty()) { return false; }

    auto shaderAllFlags = SHADER_STAGE_FLAG_NONE;
    for (const auto& refl : shaderRefls)
    {
        AXE_ASSERT(std::has_single_bit((u32)refl.mShaderStage));
        if (shaderAllFlags & refl.mShaderStage)
        {
            AXE_ERROR("Duplicate shader stage ({}) was detected in shader reflection array.", reflection::enum_name(refl.mShaderStage));
            return false;
        }
        else
        {
            shaderAllFlags |= refl.mShaderStage;
        }
    }
    outPipelineRefl.mShaderStages = shaderAllFlags;

    //
    auto& outUniqueResources      = outPipelineRefl.mShaderResources;
    auto outUniqueVariables       = outPipelineRefl.mShaderVariables;
    std::pmr::vector<const ShaderResource*> tmpUniqueVariableParents;
    for (auto& srcRefl : shaderRefls)
    {
        outPipelineRefl.mShaderReflections[std::countr_zero((u32)srcRefl.mShaderStage)].reset(&srcRefl);
        for (const auto& shaderRes : srcRefl.mShaderResources)
        {
            // Go through all already added shader resources to see if this shader
            //  resource was already added from a different shader stage. If we find a
            //  duplicate shader resource, we add the shader stage to the shader stage
            //  mask of that resource instead.

            auto iter = std::find_if(outUniqueResources.begin(), outUniqueResources.end(), [&shaderRes](ShaderResource& uniRes)
                                     { return uniRes == shaderRes; });
            if (iter == outUniqueResources.end()) { outUniqueResources.push_back(shaderRes); }  // not found
            else { iter->mUsedShaderStage |= shaderRes.mUsedShaderStage; }                      // fond
        }

        // Loop through all shader variables (constant/uniform buffer members)
        for (const auto& shaderVar : srcRefl.mShaderVariables)
        {
            auto iter = std::find_if(outUniqueVariables.begin(), outUniqueVariables.end(), [&shaderVar](ShaderVariable& uniVar)
                                     { return uniVar == shaderVar; });
            if (iter == outUniqueVariables.end())  // not found
            {
                tmpUniqueVariableParents.push_back(&srcRefl.mShaderResources[shaderVar.mParentIndex]);
                outUniqueVariables.push_back(shaderVar);
            }
            else {}  // do nothing if found
        }
    }

    // look for parent for each variable
    for (u32 i = 0; i < outUniqueVariables.size(); ++i)
    {
        for (u32 j = 0; j < outUniqueResources.size(); ++j)
        {
            if (outUniqueResources[j] == *tmpUniqueVariableParents[i])
            {
                outUniqueVariables[i].mParentIndex = j;
                break;
            }
        }
    }

    return false;
}

}  // namespace axe::rhi