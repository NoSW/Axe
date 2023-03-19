#include "02Rhi/Vulkan/VulkanRootSignature.hpp"
#include "02Rhi/Vulkan/VulkanSampler.hpp"
#include "02Rhi/Vulkan/VulkanDevice.hpp"
#include "02Rhi/Vulkan/VulkanShader.hpp"

// #include "00Core/String/String.hpp"

#include <volk/volk.h>

namespace axe::rhi
{

struct UpdateFrequencyLayoutInfo
{
    std::pmr::vector<VkDescriptorSetLayoutBinding> mBindings;  // Array of all bindings in the descriptor set
    std::pmr::vector<DescriptorInfo*> mDescriptors;            // Array of all descriptors in this descriptor set
    std::pmr::vector<DescriptorInfo*> mDynamicDescriptors;     // Array of all descriptors marked as dynamic in this descriptor set
    // (applicable to DESCRIPTOR_TYPE_UNIFORM_BUFFER)
};

bool VulkanRootSignature::_create(RootSignatureDesc& desc) noexcept
{
    // Collect all unique shader resources in the given shader reflection
    // Resources are parsed by name (two resources named "XYZ" in two shaders will be considered the same resource)
    PipelineType pipelineType = PIPELINE_TYPE_UNDEFINED;
    std::pmr::vector<ShaderResource> addedShaderResources;
    for (const auto* shader : desc.mShaders)
    {
        const auto* pShader                   = static_cast<const VulkanShader*>(shader);
        PipelineReflection const* pReflection = &pShader->_mReflection;

        // Step1: determine pipeline type(graphics, compute, raytracing)' from reflection
        if (pShader->_mReflection.mShaderStages & SHADER_STAGE_FLAG_COMP) { pipelineType = PIPELINE_TYPE_COMPUTE; }
        else if (pShader->_mReflection.mShaderStages & SHADER_STAGE_FLAG_RAYTRACING) { pipelineType = PIPELINE_TYPE_RAYTRACING; }
        else { pipelineType = PIPELINE_TYPE_GRAPHICS; }

        // Step2: collect all shader resources by name from reflection. If same binding with diff name, also be think as same one
        for (const auto& currRes : pReflection->mShaderResources)
        {
            u32 foundIndexByName = U32_MAX, foundIndexByLocation = U32_MAX;
            for (u32 i = 0; i < addedShaderResources.size(); ++i)
            {
                if (addedShaderResources[i].mName == currRes.mName)
                {
                    foundIndexByName = i;
                }

                if (addedShaderResources[i].mType == currRes.mType && addedShaderResources[i].mUsedShaderStage == currRes.mUsedShaderStage &&
                    addedShaderResources[i].mSet == currRes.mSet && addedShaderResources[i].mBindingLocation == currRes.mBindingLocation)
                {
                    foundIndexByLocation = i;
                }
            }

            if (foundIndexByName == U32_MAX)  // first add
            {
                if (foundIndexByLocation == U32_MAX)  // if also not found by binding, we add it
                {
                    addedShaderResources.push_back(currRes);
                }
                else  // if found by binding, we also think the same one
                {
                    addedShaderResources[foundIndexByLocation].mUsedShaderStage |= currRes.mUsedShaderStage;
                }
            }
            else  // already added, we check whether conflicts
            {
                if (addedShaderResources[foundIndexByName].mSet != currRes.mSet ||
                    addedShaderResources[foundIndexByName].mBindingLocation != currRes.mBindingLocation)
                {
                    AXE_ERROR("Failed to create root signature: Shared shader resource {} has mismatching set or binding location", currRes.mName.data());
                    return false;
                }
                addedShaderResources[foundIndexByName].mUsedShaderStage |= currRes.mUsedShaderStage;
            }
        }
    }

    // Collect all unique static samplers in the given static samplers
    std::pmr::unordered_map<std::string_view, VulkanSampler*> staticSamplerMap;  // <name, sampler>
    AXE_ASSERT(desc.mStaticSamplerNames.size() == desc.mStaticSamplers.size(), "Static sampler name count must match static sampler count");
    for (u32 i = 0; i < desc.mStaticSamplers.size(); ++i)
    {
        auto foundIter = staticSamplerMap.find(desc.mStaticSamplerNames[i]);
        if (foundIter == staticSamplerMap.end()) { staticSamplerMap[desc.mStaticSamplerNames[i]] = static_cast<VulkanSampler*>(desc.mStaticSamplers[i]); }
        else { AXE_ERROR("Name conflicts among static samplers, {}", desc.mStaticSamplerNames[i].data()); }
    }

    // Using data parsed from reflection to init this object
    _mPipelineType                                      = pipelineType;
    constexpr u32 MAX_LAYOUT_COUNT                      = DESCRIPTOR_UPDATE_FREQ_COUNT;
    UpdateFrequencyLayoutInfo Layouts[MAX_LAYOUT_COUNT] = {};
    _mDescriptors.resize(addedShaderResources.size());
    std::pmr::vector<VkPushConstantRange> pushConstants;
    for (u32 i = 0; i < addedShaderResources.size(); ++i)
    {
        // for short
        auto& fromRes = addedShaderResources[i];
        auto& toInfo  = _mDescriptors[i];
        u32 setIndx   = fromRes.mSet;

        // Step1: Copy the binding information generated from the shader reflection into the descriptor
        toInfo.mReg   = fromRes.mBindingLocation;
        toInfo.mSize  = fromRes.mSize;
        toInfo.mType  = fromRes.mType;
        toInfo.mName  = fromRes.mName;
        toInfo.mDim   = fromRes.mDim;

        // Step2: If descriptor is not a root constant, create a new layout binding for this descriptor and add it to the binding array
        if (fromRes.mType != DESCRIPTOR_TYPE_ROOT_CONSTANT)
        {
            VkDescriptorSetLayoutBinding binding{
                .binding            = fromRes.mBindingLocation,
                .descriptorType     = to_vk_enum(fromRes.mType),
                .descriptorCount    = fromRes.mSize,
                .stageFlags         = to_vk_enum(fromRes.mUsedShaderStage),
                .pImmutableSamplers = nullptr};

            // if (string::find_substr_case_insensitive(fromRes.mName, std::string_view("rootcbv")) < fromRes.mName.size())
            if (fromRes.mName.find_first_of("rootcbv") < fromRes.mName.size())  // FIXME, using above
            {
                if (fromRes.mSize == 1)
                {
                    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    AXE_INFO("Descriptor ({}) : User specified VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC", fromRes.mName.data());
                }
                else
                {
                    AXE_ERROR("Descriptor ({}) : Cannot use VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC for arrays", fromRes.mName.data());
                }
            }

            // record enum value of vulkan
            toInfo.mType            = binding.descriptorType;
            toInfo.mVkStages        = binding.stageFlags;
            toInfo.mUpdateFrequency = (DescriptorUpdateFrequency)setIndx;

            // Find if the given descriptor is a static sampler
            auto foundSamIter       = staticSamplerMap.find(toInfo.mName);

            if (foundSamIter != staticSamplerMap.end())
            {
                AXE_INFO("Descriptor ({}) : User specified Static Sampler", foundSamIter->first.data());
                binding.pImmutableSamplers = &foundSamIter->second->_mpHandle;
            }

            // Set the index to an invalid value so we can use this later for error checking if user tries to update a static sampler
            // In case of Combined Image Samplers, skip invalidating the index
            // because we do not to introduce new ways to update the descriptor in the Interface
            if (foundSamIter != staticSamplerMap.end() && toInfo.mType != DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER_VKONLY)
            {
                toInfo.mStaticSampler = true;
            }
            else
            {
                Layouts[setIndx].mDescriptors.push_back(&toInfo);
            }

            if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            {
                Layouts[setIndx].mDynamicDescriptors.push_back(&toInfo);
                toInfo.mIsRootDescriptor = true;
            }

            Layouts[setIndx].mBindings.push_back(binding);

            // Update descriptor pool size for this descriptor type
            VkDescriptorPoolSize* pPoolSize = nullptr;
            for (u32 i = 0; i < MAX_DESCRIPTOR_POOL_SIZE_ARRAY_COUNT; ++i)
            {
                if (binding.descriptorType == _mPoolSizes[setIndx]->type && _mPoolSizes[setIndx]->descriptorCount > 0)
                {
                    pPoolSize = &_mPoolSizes[setIndx][i];
                    break;
                }
            }
            if (pPoolSize == nullptr)
            {
                pPoolSize       = &_mPoolSizes[setIndx][_mPoolSizeCount[setIndx]++];
                pPoolSize->type = binding.descriptorType;
            }
            pPoolSize->descriptorCount += binding.descriptorCount;
        }
        else  // is a root constant, just add it to the root constant array
        {
            AXE_INFO("Descriptor ({}) : User specified Push Constant", toInfo.mName.data());
            toInfo.mIsRootDescriptor = true;
            toInfo.mVkStages         = to_vk_enum(fromRes.mUsedShaderStage);
            setIndx                  = 0;
            toInfo.mHandleIndex      = pushConstants.size();
            pushConstants.push_back(VkPushConstantRange{
                .stageFlags = toInfo.mVkStages,
                .offset     = 0,
                .size       = toInfo.mSize});
        }
    }

    // Create descriptor layouts, (Put least frequently changed params first)
    for (i32 layoutIndex = (i32)MAX_LAYOUT_COUNT; layoutIndex >= 0; layoutIndex--)
    {
        // for short
        auto& layout = Layouts[layoutIndex];

        // sort table by type (CBV/SRV/UAV) by register
        std::sort(layout.mBindings.begin(), layout.mBindings.end(),
                  [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
                  { return b.descriptorType < a.descriptorType || (b.descriptorType == a.descriptorType && b.binding > a.binding); });

        bool isCreateLayout = !layout.mBindings.empty();

        // Check if we need to create an empty layout in case there is an empty set between two used sets
        // Example: set = 0 is used, set = 2 is used. In this case, set = 1 needs to exist even if it is empty
        if (!isCreateLayout && layoutIndex < MAX_LAYOUT_COUNT - 1)
        {
            isCreateLayout = _mpDescriptorSetLayouts[layoutIndex + 1] != VK_NULL_HANDLE;
        }

        if (isCreateLayout)
        {
            if (!layout.mBindings.empty())
            {
                VkDescriptorSetLayoutCreateInfo layoutInfo{
                    .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                    .pNext        = nullptr,
                    .flags        = 0,
                    .bindingCount = (u32)layout.mBindings.size(),
                    .pBindings    = layout.mBindings.data()};

                AXE_CHECK(VK_SUCCEEDED(vkCreateDescriptorSetLayout(_mpDevice->handle(), &layoutInfo, nullptr, &_mpDescriptorSetLayouts[layoutIndex])));
            }
            else
            {
                _mpDescriptorSetLayouts[layoutIndex] = _mpDevice->_mpEmptyDescriptorSetLayout;
            }
        }

        if (layout.mBindings.empty()) { continue; }

        u32 cumulativeDescriptorCount = 0;
        for (DescriptorInfo* info : layout.mDescriptors)
        {
            if (!info->mIsRootDescriptor)
            {
                info->mHandleIndex = cumulativeDescriptorCount;
                cumulativeDescriptorCount += info->mSize;
            }
        }

        cumulativeDescriptorCount = 0;

        // vkCmdBindDescriptorSets - pDynamicOffsets - entries are ordered by the binding numbers in the descriptor set layouts
        std::sort(layout.mDynamicDescriptors.begin(), layout.mDynamicDescriptors.end(),
                  [](DescriptorInfo* a, DescriptorInfo* b)
                  { return a->mReg < b->mReg; });

        _mDynamicDescriptorCounts[layoutIndex] = layout.mDynamicDescriptors.size();
        for (DescriptorInfo* info : layout.mDynamicDescriptors)
        {
            info->mHandleIndex = cumulativeDescriptorCount;
            cumulativeDescriptorCount += 1;
        }
    }

    // Create pipeline layout
    std::pmr::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    for (u32 i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
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

    AXE_CHECK(VK_SUCCEEDED(vkCreatePipelineLayout(_mpDevice->handle(), &pipelineLayoutInfo, nullptr, &_mpPipelineLayout)));

    // Update template
    for (u32 setIndex = 0; setIndex < DESCRIPTOR_UPDATE_FREQ_COUNT; ++setIndex)
    {
        const UpdateFrequencyLayoutInfo& layout = Layouts[setIndex];
        if (layout.mDescriptors.empty() && _mpDescriptorSetLayouts[setIndex] != VK_NULL_HANDLE)
        {
            _mpEmptyDescriptorSet[setIndex] = _mpDevice->_mpEmptyDescriptorSet;
            if (_mpDescriptorSetLayouts[setIndex] != _mpDevice->_mpEmptyDescriptorSetLayout)
            {
                VkDescriptorPoolCreateInfo poolCreateInfo{
                    .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                    .pNext         = nullptr,
                    .flags         = 0,
                    .maxSets       = 1,
                    .poolSizeCount = _mPoolSizeCount[setIndex],
                    .pPoolSizes    = _mPoolSizes[setIndex]};
                AXE_CHECK(VK_SUCCEEDED(vkCreateDescriptorPool(_mpDevice->handle(), &poolCreateInfo, nullptr, &_mpEmptyDescriptorPool[setIndex])));

                VkDescriptorSetAllocateInfo allocInfo{
                    .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
                    .pNext              = nullptr,
                    .descriptorPool     = _mpEmptyDescriptorPool[setIndex],
                    .descriptorSetCount = 1,
                    .pSetLayouts        = &_mpDescriptorSetLayouts[setIndex]};

                AXE_CHECK(VK_SUCCEEDED(vkAllocateDescriptorSets(_mpDevice->handle(), &allocInfo, &_mpEmptyDescriptorSet[setIndex])));
            }
        }
    }

    // add dependencies tracking here

    return true;
}

bool VulkanRootSignature::_destroy() noexcept
{
    for (u32 i = 0; i < DESCRIPTOR_UPDATE_FREQ_COUNT; ++i)
    {
        if (_mpDescriptorSetLayouts[i] != _mpDevice->_mpEmptyDescriptorSetLayout)
        {
            vkDestroyDescriptorSetLayout(_mpDevice->handle(), _mpDescriptorSetLayouts[i], nullptr);
        }
        if (_mpEmptyDescriptorPool[i] != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(_mpDevice->handle(), _mpEmptyDescriptorPool[i], nullptr);
        }
    }

    vkDestroyPipelineLayout(_mpDevice->handle(), _mpPipelineLayout, nullptr);
    _mpPipelineLayout = VK_NULL_HANDLE;
    return true;
}
}  // namespace axe::rhi