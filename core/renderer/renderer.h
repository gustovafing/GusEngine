#pragma once
#include <vulkan/vulkan.h>
#include <array>
#include <GLFW/glfw3.h>
#include <optional>
#include <limits> 
#include <algorithm>
#include <stdexcept>
#include <set>
#include <map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include "filesystem/resource_loader.h"
#include "core/globals.h"
#include <VkBootstrap.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "vkAllocator.h"
#include "descriptorBuilder.h"
#include "utils/uniqueId.h"

using namespace vkAllocator;
const int32_t MAX_FRAMES_IN_FLIGHT = 2;

using QueueType = vkb::QueueType;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
constexpr bool _USE_VK_VALIDATION_LAYERS = false;
#else
constexpr bool _USE_VK_VALIDATION_LAYERS = true;
#endif

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

};

const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
};

struct FrameData {
	VkCommandBuffer commandBuffer;
	VkSemaphore imageSemaphore, renderSemaphore;
	VkFence renderFence;
	VkDescriptorSet descriptorSet;
	BufferAlloc uniformBuffer;
	vkb::Device* _device;
	void Destroy() const {
		vkDestroySemaphore(*_device, imageSemaphore, nullptr);
		vkDestroySemaphore(*_device, renderSemaphore, nullptr);
		vkDestroyFence(*_device, renderFence, nullptr);
	}
};

struct SwapchainImage {
	VkImage image;
	VkImageView imageView;
	VkFramebuffer framebuffer;
};
class Renderer {
	public:

	void Init(GLFWwindow* window);
	void RefreshFramebuffer();
	void ProcessFrame();
	void Cleanup();
	private:

	FrameData _frames[MAX_FRAMES_IN_FLIGHT];
	FrameData& get_current_frame() { return _frames[frameNum % MAX_FRAMES_IN_FLIGHT]; }

	GLFWwindow* _window;
	Allocator* _allocator = nullptr;
	vkb::Instance _instance;
	vkb::PhysicalDevice _physicalDevice;
	vkb::Device _device;
	VkSurfaceKHR _surface = nullptr;
	vkb::Swapchain _swapchain;

	DescriptorAllocator* _descriptorAllocator = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	VkRenderPass renderPass = nullptr;
	VkPipeline graphicsPipeline = nullptr;
	uint32_t frameNum = 0;
	
	std::map<QueueType, VkQueue> queues{};
	std::map<QueueType, VkCommandPool> commandPools{};
	BufferAlloc vertexBuffer;
	BufferAlloc indexBuffer;
	std::vector<SwapchainImage> swapchainImages;

	//void initImGUI();

	/// Rendering init and cleanup
	void initVulkan();
	void createInstanceAndDevice();
	void createCommandPools();

	void createSwapchain();
	void createFrameObjects();
	void createVertexBuffer();
	void createIndexBuffer();

	void createRenderPass();
	void createDescriptorAllocator();
	void createGraphicsPipeline();
	void createFramebuffers();

	void cleanupSwapChain(bool destroySwapchain = true);
	void recreateSwapChain();

	void drawFrame();

	VkCommandBuffer createOneTimeCommandBuffer(QueueType queue);
	void submitOneTimeCommandBuffer(VkCommandBuffer buffer, QueueType queue);

	void recordFrameCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void updateUniformBuffer(uint32_t currentImage);
};