#include "VulkanPipeline.hxx"
#include "VulkanDevice.hxx"
#include "VulkanShader.hxx"
#include "VulkanRootSignature.hxx"

#include <volk.h>

namespace axe::rhi
{

struct RenderPassDesc
{
    TinyImageFormat* pColorFormats                     = nullptr;
    bool* pIsSrgbValues                                = nullptr;
    u32 renderTargetCount                              = 0;
    MSAASampleCount sampleCount                        = MSAASampleCount::COUNT_1;
    TinyImageFormat depthStencilFormat                 = TinyImageFormat_UNDEFINED;
    nullable<const LoadActionType> pLoadActionsColor   = nullptr;  // if nullptr, all set to LoadActionType::DONT_CARE by default
    nullable<const StoreActionType> pStoreActionsColor = nullptr;  // if nullptr, all set to StoreActionType::DONT_CARE by default
    LoadActionType loadActionDepth                     = LoadActionType::COUNT;
    LoadActionType loadActionStencil                   = LoadActionType::COUNT;
    StoreActionType storeActionDepth                   = StoreActionType::COUNT;
    StoreActionType storeActionStencil                 = StoreActionType::COUNT;
};

struct RenderPass
{
    RenderPassDesc desc;
    VulkanDevice* const pDevice = nullptr;
    VkRenderPass handle         = VK_NULL_HANDLE;

    ~RenderPass() noexcept { vkDestroyRenderPass(pDevice->handle(), handle, &g_VkAllocationCallbacks); }

    RenderPass(const RenderPassDesc& renderPassDesc, VulkanDevice* const pDevice) : desc(renderPassDesc), pDevice(pDevice)
    {
        VkAttachmentDescription attachments[VK_MAX_ATTACHMENT_ARRAY_COUNT]{};
        VkAttachmentReference colorAttachmentRefs[MAX_RENDER_TARGET_ATTACHMENTS]{};
        VkAttachmentReference depthStencilAttachmentRef{};

        u32 attachmentCount               = 0;
        VkSampleCountFlagBits sampleCount = to_vk_enum(desc.sampleCount);

        // Fill out attachment descriptions and references
        for (u32 i = 0; i < desc.renderTargetCount; ++i)
        {
            // descriptions
            u32 idx                              = attachmentCount++;
            VkAttachmentDescription* attachment  = &attachments[idx];
            attachment->flags                    = 0;
            attachment->format                   = (VkFormat)to_vk_enum(desc.pColorFormats[i]);
            attachment->samples                  = sampleCount;
            attachment->loadOp                   = to_vk_enum(desc.pLoadActionsColor ? desc.pLoadActionsColor[i] : LoadActionType::DONT_CARE);
            attachment->storeOp                  = to_vk_enum(desc.pStoreActionsColor ? desc.pStoreActionsColor[i] : StoreActionType::DONT_CARE);
            attachment->stencilLoadOp            = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment->stencilStoreOp           = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment->initialLayout            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment->finalLayout              = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // references
            VkAttachmentReference* attachmentRef = &colorAttachmentRefs[i];
            attachmentRef->attachment            = idx;
            attachmentRef->layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        // Fill depth stencil
        const bool hasDepthAttachmentCount = (desc.depthStencilFormat != TinyImageFormat_UNDEFINED);
        if (hasDepthAttachmentCount)
        {
            u32 idx                              = attachmentCount++;
            attachments[idx].flags               = 0;
            attachments[idx].format              = to_vk_enum(desc.depthStencilFormat);
            attachments[idx].samples             = sampleCount;
            attachments[idx].loadOp              = to_vk_enum(desc.loadActionDepth);
            attachments[idx].storeOp             = to_vk_enum(desc.storeActionDepth);
            attachments[idx].stencilLoadOp       = to_vk_enum(desc.loadActionStencil);
            attachments[idx].stencilStoreOp      = to_vk_enum(desc.storeActionStencil);
            attachments[idx].initialLayout       = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachments[idx].finalLayout         = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthStencilAttachmentRef.attachment = idx;
            depthStencilAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }

        // create subpass, render pass
        {
            VkSubpassDescription subDesc{
                .flags                   = 0,
                .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .inputAttachmentCount    = 0,
                .pInputAttachments       = nullptr,
                .colorAttachmentCount    = desc.renderTargetCount,
                .pColorAttachments       = desc.renderTargetCount ? colorAttachmentRefs : nullptr,
                .pResolveAttachments     = nullptr,  // Each element from this array corresponds to an element from a color attachments array;
                                                     // any such color attachment will be resolved to a given resolve attachment
                .pDepthStencilAttachment = hasDepthAttachmentCount ? &depthStencilAttachmentRef : nullptr,
                .preserveAttachmentCount = 0,
                .pPreserveAttachments    = nullptr,  // When we have multiple subpasses not all of them will use all attachments.
                                                     // If a subpass doesn’t use some of the attachments but we need their contents
                                                     // in the later subpasses, we must specify these attachments here.
            };
            VkRenderPassCreateInfo createInfo{
                .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                .pNext           = nullptr,
                .flags           = 0,
                .attachmentCount = attachmentCount,
                .pAttachments    = attachments,
                .subpassCount    = 1,
                .pSubpasses      = &subDesc,
                .dependencyCount = 0,
                .pDependencies   = nullptr,
            };
            vkCreateRenderPass(pDevice->handle(), &createInfo, &g_VkAllocationCallbacks, &handle);
        }
    }
};

bool VulkanPipeline::_crateGraphicsPipeline(const PipelineDesc& desc) noexcept
{
    AXE_ASSERT(desc.pGraphicsDesc);
    AXE_ASSERT(desc.pGraphicsDesc->pShaderProgram);
    AXE_ASSERT(desc.pGraphicsDesc->pRootSignature);
    AXE_ASSERT(desc.pGraphicsDesc->pVertexLayout);

    auto* pShader        = (VulkanShader*)desc.pGraphicsDesc->pShaderProgram;
    auto* pRootSignature = (VulkanRootSignature*)desc.pGraphicsDesc->pRootSignature;

    for (u32 i = 0; i < pShader->_mpHandles.size(); ++i)
    {
        if (pShader->containStage(id2bit<ShaderStageFlag>(i))) { AXE_ASSERT(pShader->_mpHandles[i]); }
    }

    // render pass
    RenderPassDesc renderPassDesc{
        .pColorFormats      = desc.pGraphicsDesc->pColorFormats,
        .pIsSrgbValues      = nullptr,
        .renderTargetCount  = desc.pGraphicsDesc->renderTargetCount,
        .sampleCount        = desc.pGraphicsDesc->sampleCount,
        .depthStencilFormat = desc.pGraphicsDesc->depthStencilFormat,
        .pLoadActionsColor  = nullptr,  // set default load action
        .pStoreActionsColor = nullptr,  // set default store action
        .loadActionDepth    = LoadActionType::DONT_CARE,
        .loadActionStencil  = LoadActionType::DONT_CARE,
        .storeActionDepth   = StoreActionType::DONT_CARE,
        .storeActionStencil = StoreActionType::DONT_CARE,
    };
    RenderPass renderPass(renderPassDesc, _mpDevice);

    // pipeline shader stages
    AXE_ASSERT(pShader->stagesCount() > 0);
    std::pmr::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    shaderStages.reserve(pShader->stagesCount());

    for (u32 i = 0; i < (u32)ShaderStageFlag::COUNT; ++i)
    {
        if (pShader->_mpHandles[i] != VK_NULL_HANDLE)
        {
            AXE_ASSERT(pShader->containStage((ShaderStageFlag)(1 << i)));

            shaderStages.push_back(VkPipelineShaderStageCreateInfo{
                .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .pNext               = nullptr,
                .flags               = 0,
                .stage               = (VkShaderStageFlagBits)to_vk_enum((ShaderStageFlag)(1 << i)),
                .module              = pShader->_mpHandles[i],
                .pName               = pShader->_mpEntryNames[i].data(),
                .pSpecializationInfo = pShader->_mpSpecializationInfo});
        }
    }

    // vertex inputs
    std::pmr::vector<VkVertexInputBindingDescription> inputBindings;
    std::pmr::vector<VkVertexInputAttributeDescription> inputAttributes;
    inputBindings.reserve(MAX_VERTEX_BINDINGS);
    inputAttributes.reserve(MAX_VERTEX_ATTRIBS);

    u32 attrCount = desc.pGraphicsDesc->pVertexLayout == nullptr ? 0 : std::min(desc.pGraphicsDesc->pVertexLayout->attrCount, (u32)MAX_VERTEX_ATTRIBS);
    if (desc.pGraphicsDesc->pVertexLayout && desc.pGraphicsDesc->pVertexLayout->attrCount > MAX_VERTEX_ATTRIBS) { AXE_ERROR("Too many vertex attributes, (current={}, max={})", desc.pGraphicsDesc->pVertexLayout->attrCount, MAX_VERTEX_ATTRIBS); }

    for (u32 i = 0; i < attrCount; ++i)
    {
        const VertexAttrib& attrib = desc.pGraphicsDesc->pVertexLayout->attrs[i];

        inputAttributes.push_back(VkVertexInputAttributeDescription{
            .location = attrib.location,
            .binding  = attrib.binding,
            .format   = to_vk_enum(attrib.format),
            .offset   = attrib.offset,
        });

        auto foundIter = std::find_if(inputBindings.begin(), inputBindings.end(), [&attrib, &inputBindings](const VkVertexInputBindingDescription& b)
                                      { return b.binding == attrib.binding; });

        if (foundIter == inputBindings.end())
        {
            inputBindings.push_back(VkVertexInputBindingDescription{
                .binding   = attrib.binding,
                .stride    = byte_count_of_format(attrib.format),
                .inputRate = to_vk_enum(attrib.rate),
            });
        }
        else
        {
            foundIter->stride += byte_count_of_format(attrib.format);
            if (foundIter->inputRate != to_vk_enum(attrib.rate))
            {
                AXE_ERROR("Vertex input rate mismatch({} != {}, name={})", reflection::enum_name(foundIter->inputRate), reflection::enum_name(to_vk_enum(attrib.rate)), attrib.semanticName.data());
            }
        }
    }
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = (u32)inputBindings.size(),
        .pVertexBindingDescriptions      = inputBindings.data(),
        .vertexAttributeDescriptionCount = (u32)inputAttributes.size(),
        .pVertexAttributeDescriptions    = inputAttributes.data(),
    };

    // IA
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .topology               = to_vk_enum(desc.pGraphicsDesc->primitiveTopology),
        .primitiveRestartEnable = VK_FALSE,  // some advanced features that'll not be used
    };

    // Tessellation
    VkPipelineTessellationStateCreateInfo teseCreateInfo = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .patchControlPoints = pShader->_mReflection.shaderReflections[bit2id(ShaderStageFlag::TESC)].numControlPoint,
    };

    // Viewport
    VkPipelineViewportStateCreateInfo viewportCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .viewportCount = 1,
        .pViewports    = nullptr,
        .scissorCount  = 1,
        .pScissors     = nullptr,
    };

    // MS
    VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .rasterizationSamples  = to_vk_enum(desc.pGraphicsDesc->sampleCount),
        .sampleShadingEnable   = VK_FALSE,
        .minSampleShading      = 0.0f,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };

    // Rasterizer
    constexpr RasterizerStateDesc DEFAULT_RASTER{};
    const RasterizerStateDesc* pRasterizerStateDesc = desc.pGraphicsDesc->pRasterizerState ? desc.pGraphicsDesc->pRasterizerState : &DEFAULT_RASTER;

    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .depthClampEnable        = pRasterizerStateDesc->enableDepthClamp,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = to_vk_enum(pRasterizerStateDesc->fillMode),
        .cullMode                = to_vk_enum(pRasterizerStateDesc->cullMode),
        .frontFace               = to_vk_enum(pRasterizerStateDesc->frontFace),
        .depthBiasEnable         = pRasterizerStateDesc->depthBias == 0 ? VK_FALSE : VK_TRUE,
        .depthBiasConstantFactor = (float)pRasterizerStateDesc->depthBias,
        .depthBiasClamp          = 0.0f,
        .depthBiasSlopeFactor    = pRasterizerStateDesc->slopeScaledDepthBias,
        .lineWidth               = 1.0f,
    };

    // DepthStencil
    constexpr DepthStateDesc DEFAULT_DEPTH_STENCIL{};
    const DepthStateDesc* pDepthStencilStateDesc = desc.pGraphicsDesc->pDepthState ? desc.pGraphicsDesc->pDepthState : &DEFAULT_DEPTH_STENCIL;

    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo{
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = pDepthStencilStateDesc->enableDepthTest ? VK_TRUE : VK_FALSE,
        .depthWriteEnable      = pDepthStencilStateDesc->enableDepthWrite ? VK_TRUE : VK_FALSE,
        .depthCompareOp        = to_vk_enum(pDepthStencilStateDesc->depthFunc),
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable     = pDepthStencilStateDesc->enableStencilTest ? VK_TRUE : VK_FALSE,
        .front{
            .failOp      = to_vk_enum(pDepthStencilStateDesc->stencilFrontFail),
            .passOp      = to_vk_enum(pDepthStencilStateDesc->stencilFrontPass),
            .depthFailOp = to_vk_enum(pDepthStencilStateDesc->depthFrontFail),
            .compareOp   = to_vk_enum(pDepthStencilStateDesc->stencilFrontFunc),
            .compareMask = pDepthStencilStateDesc->stencilReadMask,
            .writeMask   = pDepthStencilStateDesc->stencilWriteMask,
            .reference   = 0,
        },
        .back{
            .failOp      = to_vk_enum(pDepthStencilStateDesc->stencilBackFail),
            .passOp      = to_vk_enum(pDepthStencilStateDesc->stencilBackPass),
            .depthFailOp = to_vk_enum(pDepthStencilStateDesc->depthBackFail),
            .compareOp   = to_vk_enum(pDepthStencilStateDesc->stencilBackFunc),
            .compareMask = pDepthStencilStateDesc->stencilReadMask,
            .writeMask   = pDepthStencilStateDesc->stencilWriteMask,
            .reference   = 0,
        },
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
    };

    // Blend
    constexpr BlendStateDesc DEFAULT_BLEND{};
    const BlendStateDesc* pBlendStateDesc = desc.pGraphicsDesc->pBlendState ? desc.pGraphicsDesc->pBlendState : &DEFAULT_BLEND;
    std::pmr::vector<VkPipelineColorBlendAttachmentState> attachmentSates(desc.pGraphicsDesc->renderTargetCount);

    for (u32 i = 0; i < attachmentSates.size(); ++i)
    {
        if ((u32)(pBlendStateDesc->renderTargetMask) & (u32)(1 << i))
        {
            u32 idx                                = pBlendStateDesc->isIndependentBlend ? i : 0;

            bool disableBlend                      = (pBlendStateDesc->perRenderTarget[idx].srcAlphaFactor == BlendConstant::ONE &&
                                 pBlendStateDesc->perRenderTarget[idx].dstAlphaFactor == BlendConstant::ZERO &&
                                 pBlendStateDesc->perRenderTarget[idx].srcFactor == BlendConstant::ONE &&
                                 pBlendStateDesc->perRenderTarget[idx].dstFactor == BlendConstant::ZERO);

            attachmentSates[i].blendEnable         = disableBlend ? VK_FALSE : VK_TRUE;
            attachmentSates[i].srcColorBlendFactor = to_vk_enum(pBlendStateDesc->perRenderTarget[idx].srcFactor);
            attachmentSates[i].dstColorBlendFactor = to_vk_enum(pBlendStateDesc->perRenderTarget[idx].dstFactor);
            attachmentSates[i].colorBlendOp        = to_vk_enum(pBlendStateDesc->perRenderTarget[idx].blendMode);
            attachmentSates[i].srcAlphaBlendFactor = to_vk_enum(pBlendStateDesc->perRenderTarget[idx].srcAlphaFactor);
            attachmentSates[i].dstAlphaBlendFactor = to_vk_enum(pBlendStateDesc->perRenderTarget[idx].dstAlphaFactor);
            attachmentSates[i].alphaBlendOp        = to_vk_enum(pBlendStateDesc->perRenderTarget[idx].blendAlphaMode);
            attachmentSates[i].colorWriteMask      = to_vk_enum(pBlendStateDesc->perRenderTarget[idx].mask);
        }
    }

    VkPipelineColorBlendStateCreateInfo blendCreateInfo{
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = 0,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_CLEAR,
        .attachmentCount = (u32)attachmentSates.size(),
        .pAttachments    = attachmentSates.data(),
        .blendConstants{0.0f, 0.0f, 0.0f, 0.0f}};

    // dynamic states
    auto dynamicStates = std::array{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = 0,
        .dynamicStateCount = dynamicStates.size(),
        .pDynamicStates    = dynamicStates.data()};

    // prepare for pipeline creation
    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stageCount          = (u32)shaderStages.size(),
        .pStages             = shaderStages.data(),
        .pVertexInputState   = &vertexInputCreateInfo,
        .pInputAssemblyState = &inputAssemblyCreateInfo,
        .pTessellationState  = pShader->hasTessellation() ? &teseCreateInfo : nullptr,
        .pViewportState      = &viewportCreateInfo,
        .pRasterizationState = &rasterizerCreateInfo,
        .pMultisampleState   = &multisampleCreateInfo,
        .pDepthStencilState  = &depthStencilCreateInfo,
        .pColorBlendState    = &blendCreateInfo,
        .pDynamicState       = &dynamicStateCreateInfo,
        .layout              = pRootSignature->handle(),
        .renderPass          = renderPass.handle,
        .subpass             = 0,  // index of the subpass in the render pass where this pipeline will be used.

        .basePipelineHandle  = VK_NULL_HANDLE,  // can’t be used with basePipelineIndex at same time
                                                // we don't inherit from another pipeline here, so set it null

        // index of pipeline creation info in this very array (just one element here).
        // The “parent” pipeline must be earlier (must have a smaller index) in this array and
        // it must be created with the “allow derivatives” flag set.
        // we don't inherit from another pipeline here, so set it an invalid index
        .basePipelineIndex   = -1};

    // Pipeline Cache
    VkPipelineCache pPipelineCache = desc.pCache ? (VkPipelineCache)desc.pCache->pCacheData : VK_NULL_HANDLE;

    // return
    return VK_SUCCEEDED(vkCreateGraphicsPipelines(_mpDevice->handle(), pPipelineCache, 1, &pipelineCreateInfo, &g_VkAllocationCallbacks, &_mpHandle));
}

bool VulkanPipeline::_crateComputePipeline(const PipelineDesc& desc) noexcept
{
    AXE_ERROR("Unsupported yet");
    return false;
}

bool VulkanPipeline::_create(const PipelineDesc& desc) noexcept
{
    _mType = desc.type;
    switch (desc.type)
    {
        case PipelineType::GRAPHICS: return _crateGraphicsPipeline(desc);
        case PipelineType::COMPUTE: return _crateComputePipeline(desc);
        default: AXE_ASSERT(false, "Invalid Type"); return false;
    }
}

bool VulkanPipeline::_destroy() noexcept
{
    AXE_ASSERT(_mpDevice);
    AXE_ASSERT(_mpHandle);
    vkDestroyPipeline(_mpDevice->handle(), _mpHandle, &g_VkAllocationCallbacks);
    return true;
}

}  // namespace axe::rhi