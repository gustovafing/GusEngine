#pragma once
#include <vulkan/vulkan.h>
#include "core/globals.h"
#include <vector>

// Descriptor sets and pools will all be unqiue to a single layout
class DescriptorAllocator {
	private:
	std::vector<VkDescriptorSetLayoutBinding> _bindings;
	
	VkDevice _device;
	VkDescriptorSetLayout _layout;
	VkDescriptorPool _pool = nullptr;

	public:
	DescriptorAllocator(VkDevice device, std::vector<VkDescriptorSetLayoutBinding> layoutBindings, VkDescriptorSetLayoutCreateFlags layoutCreateFlags = 0) {
		_device = device;
		_bindings = std::vector<VkDescriptorSetLayoutBinding>(layoutBindings);
		
		for (uint32_t i = 0; i < _bindings.size(); i++)
		{
			_bindings[i].binding = i;
		}
		
		VkDescriptorSetLayoutCreateInfo info = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.flags = layoutCreateFlags;
		info.pBindings = _bindings.data();
		info.bindingCount = static_cast<uint32_t>(_bindings.size());

		if (vkCreateDescriptorSetLayout(_device, &info, nullptr, &_layout) != VK_SUCCESS) {
			Log.FatalError("Vulkan", "Failed to build descriptor set layout");
		}

	}

	void CreatePool(uint32_t totalLayouts=1) {

		std::vector<VkDescriptorPoolSize> poolSizes{};
		for (uint32_t i = 0; i < _bindings.size(); i++)
		{
			VkDescriptorPoolSize poolSize{};
			poolSize.type = _bindings[i].descriptorType;
			poolSize.descriptorCount = _bindings[i].descriptorCount * totalLayouts;
			
			poolSizes.push_back(poolSize);

		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = totalLayouts;
		if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_pool) != VK_SUCCESS) {
			Log.FatalError("Vulkan", "failed to create descriptor pool!");
		}
	}

	std::vector<VkDescriptorSet> AllocateDescriptorSets(uint32_t setCount) const {

		std::vector<VkDescriptorSetLayout> layouts(setCount);
		for (uint32_t i = 0; i < setCount; i++)
		{
			layouts[i] = _layout;
		}

		std::vector<VkDescriptorSet> sets(setCount);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.pSetLayouts = layouts.data();
		allocInfo.descriptorPool = _pool;
		allocInfo.descriptorSetCount = setCount;
		vkAllocateDescriptorSets(_device, &allocInfo, sets.data());
		return sets;
	}

	VkDescriptorSetLayout GetLayoutObj() const {
		return _layout;
	}

	~DescriptorAllocator() {
		vkDestroyDescriptorPool(_device, _pool, nullptr);
		vkDestroyDescriptorSetLayout(_device, _layout, nullptr);
	}
};