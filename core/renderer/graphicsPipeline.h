#pragma once
#include <vulkan/vulkan.h>
#include <VkBootstrap.h>
#include "core/globals.h"
#include <vector>
class GraphicsPipelineBuilder {
    VkPipelineCreateFlags flags = 0;
    VkPipelineLayout layout = nullptr;
    VkPipelineLayoutCreateInfo layoutInfo;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineTessellationStateCreateInfo tessellationState;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
    VkPipelineDynamicStateCreateInfo dynamicState;
    uint32_t subpass = 0;
    VkPipeline basePipelineHandle = nullptr;
    int32_t basePipelineIndex = 0;

    std::vector<VkPushConstantRange> pushConstantRanges{};
    std::vector<VkDescriptorSetLayout> descriptorLayouts{};
    std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions{};
    std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions{};
    std::vector<VkViewport> viewportStateViewports{};
    std::vector<VkRect2D> viewportScissors{};
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments{};
    std::vector<VkDynamicState> dynamicStates{};
    public:
    GraphicsPipelineBuilder() {
        stages = {};
        layoutInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        vertexInputState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        inputAssemblyState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        tessellationState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
        viewportState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        rasterizationState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        multisampleState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        depthStencilState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        colorBlendState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        dynamicState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    }

    GraphicsPipelineBuilder AddShaderStage(VkShaderStageFlagBits stage, VkShaderModule sModule) {
        VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
        shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageCreateInfo.stage = stage;
        shaderStageCreateInfo.module = sModule;
        shaderStageCreateInfo.pName = "main";
        stages.push_back(shaderStageCreateInfo);
        return *this;
    }

    GraphicsPipelineBuilder SetVertexInputState(VkPipelineVertexInputStateCreateFlags flags, std::vector<VkVertexInputAttributeDescription> attributes, std::vector<VkVertexInputBindingDescription> bindings) {
        vertexInputAttributeDescriptions = std::vector<VkVertexInputAttributeDescription>(attributes);
        vertexInputBindingDescriptions = std::vector<VkVertexInputBindingDescription>(bindings);
        
        vertexInputState.flags = flags;
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributeDescriptions.size());
        vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindingDescriptions.size());
        vertexInputState.pVertexAttributeDescriptions = vertexInputAttributeDescriptions.data();
        vertexInputState.pVertexBindingDescriptions = vertexInputBindingDescriptions.data();
        return *this;
    }

    GraphicsPipelineBuilder SetInputAssemblyState(VkPipelineInputAssemblyStateCreateFlags flags, bool primRestartEnable, VkPrimitiveTopology topology) {
        inputAssemblyState.flags = flags;
        inputAssemblyState.primitiveRestartEnable = primRestartEnable;
        inputAssemblyState.topology = topology;
        return *this;
    }

    GraphicsPipelineBuilder SetTessellationState(VkPipelineTessellationStateCreateFlags flags, uint32_t patchControlPoints) {
        tessellationState.flags = flags;
        tessellationState.patchControlPoints = patchControlPoints;
        return *this;
    }

    GraphicsPipelineBuilder SetViewportState(VkPipelineViewportStateCreateFlags flags, std::vector<VkViewport> viewports, std::vector<VkRect2D> scissors, uint32_t viewportOverrideSize=0, uint32_t scissorOverrideSize=0) {
        viewportStateViewports = std::vector<VkViewport>(viewports);
        viewportScissors = std::vector<VkRect2D>(scissors);
        viewportState.flags = flags;
        viewportState.viewportCount = viewportOverrideSize != 0 ? viewportOverrideSize : static_cast<uint32_t>(viewportStateViewports.size());
        viewportState.scissorCount = scissorOverrideSize != 0 ? scissorOverrideSize : static_cast<uint32_t>(viewportScissors.size());
        viewportState.pScissors = viewportScissors.data();
        viewportState.pViewports = viewportStateViewports.data();
        return *this;
    }

    struct RasterizationDepthBias {
        bool enabled = false;
        float constantFactor = 0;
        float slopeFactor = 0;
        float clamp = 0;
    };

    GraphicsPipelineBuilder SetRasterizationState(VkPipelineRasterizationStateCreateFlags flags, VkCullModeFlags cullMode, VkPolygonMode polyMode, VkFrontFace frontFace, bool discardEnable, bool depthClampEnable, float lineWidth, RasterizationDepthBias depthBias) {
        rasterizationState.flags = flags;
        rasterizationState.cullMode = cullMode;
        rasterizationState.polygonMode = polyMode;
        rasterizationState.frontFace = frontFace;
        rasterizationState.rasterizerDiscardEnable = discardEnable;
        rasterizationState.depthClampEnable = depthClampEnable;
        rasterizationState.lineWidth = lineWidth;
        rasterizationState.depthBiasEnable = depthBias.enabled;
        rasterizationState.depthBiasSlopeFactor = depthBias.slopeFactor;
        rasterizationState.depthBiasConstantFactor = depthBias.constantFactor;
        rasterizationState.depthBiasClamp = depthBias.clamp;

        return *this;
    }

    GraphicsPipelineBuilder SetMultisampleState(VkPipelineMultisampleStateCreateFlags flags, bool alphaToCoverage, bool alphaToOne, bool sampleShading, float minSampleShading, VkSampleCountFlagBits sampleCount, VkSampleMask* sampleMask) {
        multisampleState.flags = flags;
        multisampleState.alphaToCoverageEnable = alphaToCoverage;
        multisampleState.alphaToOneEnable = alphaToOne;
        multisampleState.sampleShadingEnable = sampleShading;
        multisampleState.minSampleShading = minSampleShading;
        multisampleState.rasterizationSamples = sampleCount;
        multisampleState.pSampleMask = sampleMask;
        return *this;
    }

    GraphicsPipelineBuilder SetDepthStencilState(VkPipelineDepthStencilStateCreateFlags flags) {
        depthStencilState.flags = flags;
        return *this;
    }

    GraphicsPipelineBuilder SetColorBlendState(VkPipelineColorBlendStateCreateFlags flags, bool logicOpEnable, VkLogicOp logicOp, std::vector<VkPipelineColorBlendAttachmentState> attachments) {
        colorBlendAttachments = std::vector<VkPipelineColorBlendAttachmentState>(attachments);
        colorBlendState.flags = flags;
        colorBlendState.logicOpEnable = logicOpEnable;
        colorBlendState.logicOp = logicOp;
        colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
        colorBlendState.pAttachments = colorBlendAttachments.data();
        return *this;
    }
    GraphicsPipelineBuilder SetDynamicStates(VkPipelineDynamicStateCreateFlags flags, std::vector<VkDynamicState> states) {
        dynamicStates = std::vector<VkDynamicState>(states);
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.flags = flags;
        dynamicState.pDynamicStates = dynamicStates.data();
        return *this;
    }

    GraphicsPipelineBuilder SetPipelineLayout(VkPipelineLayoutCreateFlags flags, std::vector<VkPushConstantRange> pushConstants, std::vector<VkDescriptorSetLayout> descSetLayouts) {
        pushConstantRanges = std::vector<VkPushConstantRange>(pushConstants);
        descriptorLayouts = std::vector<VkDescriptorSetLayout>(descSetLayouts);
        layoutInfo.pPushConstantRanges = pushConstantRanges.data();
        layoutInfo.pSetLayouts = descriptorLayouts.data();
        layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
        layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorLayouts.size());
        return *this;
    }

    VkPipelineLayout BuildLayout(vkb::Device* device) {
        if (vkCreatePipelineLayout(*device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
            Log.FatalError("Vulkan", "Failed to build graphics pipeline layout.");
        }
        return layout;
    }
    VkPipeline BuildPipeline(vkb::Device* device, VkRenderPass renderPass) {

        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.flags = flags;
        info.stageCount = static_cast<uint32_t>(stages.size());
        info.pStages = stages.data();
        info.pVertexInputState = &vertexInputState;
        info.pInputAssemblyState = &inputAssemblyState;
        info.pTessellationState = &tessellationState;
        info.pViewportState = &viewportState;
        info.pRasterizationState = &rasterizationState;
        info.pMultisampleState = &multisampleState;
        info.pDepthStencilState = &depthStencilState;
        info.pColorBlendState = &colorBlendState;
        info.pDynamicState = &dynamicState;
        info.layout = layout;
        info.renderPass = renderPass;
        info.subpass = subpass;
        info.basePipelineHandle = basePipelineHandle;
        info.basePipelineIndex = basePipelineIndex;
        VkPipeline pipeline;
        
        if (vkCreateGraphicsPipelines(*device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline) != VK_SUCCESS) {
            Log.FatalError("Vulkan", "Failed to build graphics pipeline.");
        }
        return pipeline;
    }
};