#include "VulkanRootSignature.hxx"
#include "VulkanSampler.hxx"
#include "VulkanDevice.hxx"
#include "VulkanShader.hxx"

// #include "00Core/String/String.hpp"

#include <volk.h>

namespace axe::rhi
{

struct UpdateFrequencyLayoutInfo
{
    std::pmr::vector<VkDescriptorSetLayoutBinding> bindings;  // Array of all bindings in the descriptor set
    std::pmr::vector<DescriptorInfo*> descriptorInfos;        // Array of all descriptors in this descriptor set
    std::pmr::vector<DescriptorInfo*> dynamicDescriptors;     // Array of all descriptors marked as dynamic in this descriptor set
    // (applicable to DescriptorTypeFlag::UNIFORM_BUFFER)
};

bool VulkanRootSignature::_create(RootSignatureDesc& desc) noexcept
{
    // Collect all unique shader resources in the given shader reflection
    // Resources are parsed by **name** (two resources named "XYZ" in two shaders will be considered the same resource)
    PipelineType pipelineType = PipelineType::UNDEFINED;
    std::pmr::vector<ShaderResource> addedShaderResources;
    for (const auto* pIShader : desc.mShaders)
    {
        const auto* pShader                   = static_cast<const VulkanShader*>(pIShader);
        PipelineReflection const* pReflection = &pShader->_mReflection;

        // Step1: determine pipeline type(graphics, compute, raytracing)' from reflection
        if ((bool)(pShader->_mReflection.shaderStages & ShaderStageFlag::COMP))
        {
            AXE_ASSERT(std::has_single_bit((u32)pShader->_mReflection.shaderStages), "must be computer shader only");
            pipelineType = PipelineType::COMPUTE;
        }
        else if ((bool)(pShader->_mReflection.shaderStages & ShaderStageFlag::RAYTRACING))
        {
            AXE_ASSERT(std::has_single_bit((u32)pShader->_mReflection.shaderStages), "must be raytracing shader only since we haven't subdivision it yet");
            pipelineType = PipelineType::RAYTRACING;
        }
        else
        {
            AXE_ASSERT(std::popcount((u32)pShader->_mReflection.shaderStages) > 1, "at least two shader stages");
            pipelineType = PipelineType::GRAPHICS;
        }
        _mPipelineType = pipelineType;

        // Step2: collect all shader resources by name from reflection. If same binding with diff name, also be think as same one
        for (const ShaderResource& currRes : pReflection->shaderResources)
        {
            u32 foundIndexByName     = U32_MAX;
            u32 foundIndexByLocation = U32_MAX;
            for (u32 i = 0; i < addedShaderResources.size(); ++i)
            {
                if (addedShaderResources[i].name == currRes.name)
                {
                    foundIndexByName = i;
                }

                if (addedShaderResources[i].type == currRes.type && addedShaderResources[i].usedShaderStage == currRes.usedShaderStage &&
                    addedShaderResources[i].mSet == currRes.mSet && addedShaderResources[i].bindingLocation == currRes.bindingLocation)
                {
                    foundIndexByLocation = i;
                }
            }

            if (foundIndexByName == U32_MAX)  // first add if not found by name
            {
                if (foundIndexByLocation == U32_MAX)  // if also not found by binding, we add it
                {
                    addedShaderResources.push_back(currRes);
                }
                else  // if found by binding, we also think the same one
                {
                    addedShaderResources[foundIndexByLocation].usedShaderStage |= currRes.usedShaderStage;
                }
            }
            else  // already added, we check whether conflicts with existing one
            {
                if (addedShaderResources[foundIndexByName].mSet != currRes.mSet ||
                    addedShaderResources[foundIndexByName].bindingLocation != currRes.bindingLocation)
                {
                    AXE_ERROR("Failed to create root signature: Shared shader resource {} has mismatching set or binding location", currRes.name.data());
                    return false;
                }
                addedShaderResources[foundIndexByName].usedShaderStage |= currRes.usedShaderStage;
            }
        }
    }

    // Using data parsed from reflection to init this object
    constexpr u32 MAX_LAYOUT_COUNT = (u32)DescriptorUpdateFrequency::COUNT;
    UpdateFrequencyLayoutInfo Layouts[MAX_LAYOUT_COUNT]{};
    _mDescriptors.resize(addedShaderResources.size());
    std::pmr::vector<VkPushConstantRange> pushConstants;
    for (u32 i = 0; i < addedShaderResources.size(); ++i)
    {
        // for short
        ShaderResource& fromRes = addedShaderResources[i];
        DescriptorInfo& toInfo  = _mDescriptors[i];
        u32 setIndx             = fromRes.mSet;

        // Step1: Copy the binding information generated from the shader reflection into the descriptor
        toInfo.reg              = fromRes.bindingLocation;
        toInfo.type             = fromRes.type;
        toInfo.size             = fromRes.size;
        toInfo.name             = fromRes.name;
        toInfo.dim              = (u32)fromRes.dim;
        toInfo.updateFrequency  = setIndx;

        // Step2: If descriptor is not a root constant, create a new layout binding for this descriptor and add it to the binding array
        if (fromRes.type == DescriptorTypeFlag::ROOT_CONSTANT)  // is a root constant, just add it to the root constant array
        {
            AXE_INFO("Descriptor ({}) : User specified Push Constant", toInfo.name.data());
            toInfo.isRootDescriptor = true;
            setIndx                 = 0;
            toInfo.handleIndex      = pushConstants.size();
            pushConstants.push_back(VkPushConstantRange{
                .stageFlags = to_vk_enum(fromRes.usedShaderStage),
                .offset     = 0,
                .size       = toInfo.size});
        }
        else
        {
            VkDescriptorSetLayoutBinding binding{
                .binding            = fromRes.bindingLocation,
                .descriptorType     = to_vk_enum(fromRes.type),
                .descriptorCount    = fromRes.size,
                .stageFlags         = to_vk_enum(fromRes.usedShaderStage),
                .pImmutableSamplers = nullptr};
#if AXE_(false, "Do not know purpose yet")
            // if (string::find_substr_case_insensitive(fromRes.name, std::string_view("rootcbv")) < fromRes.name.size())
            if (fromRes.name == "rootcbv")  // FIXME, using above
            {
                if (fromRes.size == 1)
                {
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    AXE_INFO("Descriptor ({}) : User specified VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC", fromRes.name.data());
                }
                else
                {
                    AXE_ERROR("Descriptor ({}) : Cannot use VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC for arrays", fromRes.name.data());
                }
            }
#endif
            // Find if the given descriptor is a static sampler
            auto foundSamIter = desc.mStaticSamplersMap.find(toInfo.name);

            if (foundSamIter != desc.mStaticSamplersMap.end())
            {
                AXE_INFO("Descriptor ({}) : User specified Static Sampler", foundSamIter->first.data());
                binding.pImmutableSamplers = &static_cast<VulkanSampler*>(foundSamIter->second)->_mpHandle;
            }

            // Set the index to an invalid value so we can use this later for error checking if user tries to update a static sampler
            // In case of Combined Image Samplers, skip invalidating the index
            // because we do not to introduce new ways to update the descriptor in the Interface
            if (foundSamIter != desc.mStaticSamplersMap.end() && toInfo.type != DescriptorTypeFlag::COMBINED_IMAGE_SAMPLER_VKONLY)
            {
                toInfo.isStaticSampler = true;
            }
            else
            {
                Layouts[setIndx].descriptorInfos.push_back(&toInfo);
            }

            if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                Layouts[setIndx].dynamicDescriptors.push_back(&toInfo);
                toInfo.isRootDescriptor = true;
            }

            Layouts[setIndx].bindings.push_back(binding);

            // Update descriptor pool size for this descriptor type
            bool foundDescriptorPool = false;
            for (u32 i = 0; i < MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT; ++i)
            {
                if (binding.descriptorType == _mPoolSizes[setIndx]->type && _mPoolSizes[setIndx]->descriptorCount > 0)
                {
                    _mPoolSizes[setIndx][i].descriptorCount += binding.descriptorCount;
                    foundDescriptorPool = true;
                    break;
                }
            }
            if (!foundDescriptorPool)
            {
                VkDescriptorPoolSize& poolSize = _mPoolSizes[setIndx][_mPoolSizeCount[setIndx]++];
                poolSize.type                  = binding.descriptorType;
                poolSize.descriptorCount       = binding.descriptorCount;
            }
        }
    }

    // Step 3: build a hash table for index descriptor by name quickly
    for (u32 i = 0; i < _mDescriptors.size(); ++i)
    {
        if (_mNameHashIds.find(std::pmr::string(_mDescriptors[i].name)) != _mNameHashIds.end())
        {
            AXE_ERROR("Descriptor ({}) : Duplicate descriptor name found", _mDescriptors[i].name.data());
        }
        else
        {
            _mNameHashIds[std::pmr::string(_mDescriptors[i].name)] = i;
        }
    }

    // Step 4: Create descriptor layouts, (Put least frequently changed params first)
    for (i32 layoutIndex = (i32)MAX_LAYOUT_COUNT - 1; layoutIndex >= 0; layoutIndex--)
    {
        UpdateFrequencyLayoutInfo& layout = Layouts[layoutIndex];  // for short

        // sort table by type (CBV/SRV/UAV) by register
        std::sort(layout.bindings.begin(), layout.bindings.end(),
                  [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
                  { return b.descriptorType < a.descriptorType || (b.descriptorType == a.descriptorType && b.binding < a.binding); });  // confusion: why sort in this order?

        bool needToCreateLayout = !layout.bindings.empty();

        // Check if we need to create an empty layout in case there is an empty set between two used sets
        // Example: set = 0 is used, set = 2 is used. In this case, set = 1 needs to exist even if it is empty
        if (!needToCreateLayout && layoutIndex < MAX_LAYOUT_COUNT - 1)
        {
            needToCreateLayout = _mpDescriptorSetLayouts[layoutIndex + 1] != VK_NULL_HANDLE;  // create if next set is not empty (namely, the current is a hole)
            if (needToCreateLayout) { AXE_WARN("there is a hole(set={}) during descriptor binding, (label={})", layoutIndex, desc.getLabel_DebugActiveOnly()); }
        }

        if (needToCreateLayout)
        {
            if (!layout.bindings.empty())
            {
                VkDescriptorSetLayoutCreateInfo layoutInfo{
                    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                    .pNext        = nullptr,
                    .flags        = 0,
                    .bindingCount = (u32)layout.bindings.size(),
                    .pBindings    = layout.bindings.data()};

                if (VK_FAILED(vkCreateDescriptorSetLayout(_mpDevice->handle(), &layoutInfo, nullptr, &_mpDescriptorSetLayouts[layoutIndex]))) { return false; }
            }
            else
            {
                _mpDescriptorSetLayouts[layoutIndex] = VK_NULL_HANDLE;
            }
        }

        if (layout.bindings.empty()) { continue; }

        //
        u32 cumulativeDescriptorCount = 0;
        for (DescriptorInfo* info : layout.descriptorInfos)
        {
            if (!info->isRootDescriptor)
            {
                info->handleIndex = cumulativeDescriptorCount;
                cumulativeDescriptorCount += info->size;
            }
        }

        // vkCmdBindDescriptorSets - pDynamicOffsets - entries are ordered by the binding numbers in the descriptor set layouts
        std::sort(layout.dynamicDescriptors.begin(), layout.dynamicDescriptors.end(),
                  [](DescriptorInfo* a, DescriptorInfo* b)
                  { return a->reg < b->reg; });

        _mDynamicDescriptorCounts[layoutIndex] = layout.dynamicDescriptors.size();
        cumulativeDescriptorCount              = 0;
        for (DescriptorInfo* info : layout.dynamicDescriptors)
        {
            info->handleIndex = cumulativeDescriptorCount;
            cumulativeDescriptorCount += 1;
        }
    }

    // Step 5: Create pipeline layout
    std::pmr::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    for (u32 i = 0; i < (u32)DescriptorUpdateFrequency::COUNT; ++i)
    {
        if (_mpDescriptorSetLayouts[i] != VK_NULL_HANDLE)
        {
            descriptorSetLayouts.push_back(_mpDescriptorSetLayouts[i]);
        }
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = (u32)descriptorSetLayouts.size(),
        .pSetLayouts            = descriptorSetLayouts.data(),
        .pushConstantRangeCount = (u32)pushConstants.size(),
        .pPushConstantRanges    = pushConstants.data()};

    if (VK_FAILED(vkCreatePipelineLayout(_mpDevice->handle(), &pipelineLayoutInfo, nullptr, &_mpPipelineLayout)))
    {
        AXE_ERROR("Failed to create pipeline layout");
        return false;
    }

    // Step 6: add dependencies tracking here

    return true;
}

bool VulkanRootSignature::_destroy() noexcept
{
    for (u32 i = 0; i < (u32)DescriptorUpdateFrequency::COUNT; ++i)
    {
        if (_mpDescriptorSetLayouts[i] != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(_mpDevice->handle(), _mpDescriptorSetLayouts[i], nullptr);
        }
    }

    vkDestroyPipelineLayout(_mpDevice->handle(), _mpPipelineLayout, nullptr);
    _mpPipelineLayout = VK_NULL_HANDLE;
    return true;
}
}  // namespace axe::rhi