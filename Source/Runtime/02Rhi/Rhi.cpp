#include "02Rhi/Rhi.hpp"

#include "02Rhi/Vulkan/VulkanBackend.hxx"

#if _WIN32
#include "02Rhi/D3D12/D3D12Backend.hxx"
#endif

#include "00Core/OS/OS.hpp"
#include "00Core/Math/Math.hpp"

#include <tiny_imageformat/tinyimageformat_query.h>

namespace axe::rhi
{

u32 byte_count_of_format(TinyImageFormat format) noexcept { return TinyImageFormat_BitSizeOfBlock(format) / 8; }

bool get_surface_info(const u32 width, const u32 height, const TinyImageFormat fmt,
                      u32& outNumBytes, u32& outRowBytes, u32& outNumRows) noexcept
{
    u64 numBytes = 0;
    u64 rowBytes = 0;
    u64 numRows  = 0;

    u32 bpp      = TinyImageFormat_BitSizeOfBlock(fmt);
    if (bpp == 0) { return false; }

    if (TinyImageFormat_IsCompressed(fmt))
    {
        u32 blockWidth    = TinyImageFormat_WidthOfBlock(fmt);
        u32 blockHeight   = TinyImageFormat_HeightOfBlock(fmt);
        u32 numBlocksWide = 0;
        u32 numBlocksHigh = 0;
        if (width > 0) { numBlocksWide = std::max(1U, (width + (blockWidth - 1)) / blockWidth); }
        if (height > 0) { numBlocksHigh = std::max(1u, (height + (blockHeight - 1)) / blockHeight); }

        rowBytes = numBlocksWide * (bpp >> 3);
        numRows  = numBlocksHigh;
        numBytes = rowBytes * numBlocksHigh;
    }
    else if (TinyImageFormat_IsPlanar(fmt))
    {
        u32 numOfPlanes = TinyImageFormat_NumOfPlanes(fmt);

        for (u32 i = 0; i < numOfPlanes; ++i)
        {
            numBytes += TinyImageFormat_PlaneWidth(fmt, i, width) * TinyImageFormat_PlaneHeight(fmt, i, height) * TinyImageFormat_PlaneSizeOfBlock(fmt, i);
        }

        numRows  = 1;
        rowBytes = numBytes;
    }
    else
    {
        rowBytes = (u64(width) * bpp + 7u) / 8u;  // round up to nearest byte
        numRows  = u64(height);
        numBytes = rowBytes * height;
    }

    if (numBytes > U32_MAX || rowBytes > U32_MAX || numRows > U32_MAX) { return false; }

    outNumBytes = (u32)numBytes;
    outRowBytes = (u32)rowBytes;
    outNumRows  = (u32)numRows;
    return true;
}

std::pair<u32, u32> get_alignment(rhi::Device* pDevice, TinyImageFormat format) noexcept
{
    u32 blockBytes                      = rhi::byte_count_of_format(format);
    const rhi::GPUSettings& gpuSettings = pDevice->getAdapter()->requestGPUSettings();
    u32 rowAlignment                    = gpuSettings.uploadBufferTextureRowAlignment;
    u32 sliceAlignment                  = math::round_up(math::round_up(gpuSettings.uploadBufferTextureAlignment, blockBytes), rowAlignment);
    return {rowAlignment, sliceAlignment};
}

Backend* createBackend(GraphicsApiFlag api, BackendDesc& desc) noexcept
{
    switch (api)
    {
        case GraphicsApiFlag::VULKAN: return new VulkanBackend(desc);
        case GraphicsApiFlag::D3D12: return nullptr;
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

    auto shaderAllFlags = ShaderStageFlag::NONE;
    for (const auto& refl : shaderRefls)
    {
        AXE_ASSERT(std::has_single_bit((u32)refl.shaderStage));
        if ((bool)(shaderAllFlags & refl.shaderStage))
        {
            AXE_ERROR("Duplicate shader stage ({}) was detected in shader reflection array.", reflection::enum_name(refl.shaderStage));
            return false;
        }
        else
        {
            shaderAllFlags |= refl.shaderStage;
        }
    }
    outPipelineRefl.shaderStages = shaderAllFlags;

    //
    auto& outUniqueResources     = outPipelineRefl.shaderResources;
    auto outUniqueVariables      = outPipelineRefl.shaderVariables;
    std::pmr::vector<const ShaderResource*> tmpUniqueVariableParents;
    for (auto& srcRefl : shaderRefls)
    {
        outPipelineRefl.shaderReflections[bit2id(srcRefl.shaderStage)] = srcRefl;
        for (const auto& shaderRes : srcRefl.shaderResources)
        {
            // Go through all already added shader resources to see if this shader
            //  resource was already added from a different shader stage. If we find a
            //  duplicate shader resource, we add the shader stage to the shader stage
            //  mask of that resource instead.

            auto iter = std::find_if(outUniqueResources.begin(), outUniqueResources.end(), [&shaderRes](ShaderResource& uniRes)
                                     { return uniRes == shaderRes; });
            if (iter == outUniqueResources.end()) { outUniqueResources.push_back(shaderRes); }  // not found
            else { iter->usedShaderStage |= shaderRes.usedShaderStage; }                        // fond
        }

        // Loop through all shader variables (constant/uniform buffer members)
        for (const auto& shaderVar : srcRefl.shaderVariables)
        {
            auto iter = std::find_if(outUniqueVariables.begin(), outUniqueVariables.end(), [&shaderVar](ShaderVariable& uniVar)
                                     { return uniVar == shaderVar; });
            if (iter == outUniqueVariables.end())  // not found
            {
                tmpUniqueVariableParents.push_back(&srcRefl.shaderResources[shaderVar.parentIndex]);
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
                outUniqueVariables[i].parentIndex = j;
                break;
            }
        }
    }

    return false;
}

}  // namespace axe::rhi