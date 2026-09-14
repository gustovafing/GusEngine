#include "renderer.h"
#include <VkBootstrap.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "graphicsPipeline.h"
#include "descriptorBuilder.h"

#define VK_ASSERT(x) if (x != VK_SUCCESS) Log.FatalError("Vulkan", "Call failure: " +  std::string(#x))

void Renderer::Init(GLFWwindow* window) {
	_window = window;
	initVulkan();
	Log.Info("Renderer", "Vulkan: Init Done");
}


/*
void Renderer::initImGUI()
{

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui_ImplGlfw_InitForVulkan(_window, false);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _physicalDevice;
	init_info.Device = _device;
	init_info.Queue = queues[QueueType::graphics];
	init_info.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE+2;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.UseDynamicRendering = true;

	ImGui_ImplVulkan_Init(&init_info);

	ImGui_ImplVulkan_CreateFontsTexture();

}*/

void Renderer::initVulkan() {
	createInstanceAndDevice();
	createCommandPools();

	_allocator = new Allocator(&_instance.instance, &_physicalDevice.physical_device, &_device.device);
	
	createSwapchain();
	createFrameObjects();
	createVertexBuffer();
	createIndexBuffer();

	createRenderPass();
	createDescriptorAllocator();
	createGraphicsPipeline();
	createFramebuffers();

	//initImGUI();
}

void Renderer::RefreshFramebuffer() {
	recreateSwapChain();
}

void Renderer::ProcessFrame() {
	bool demoshow = true;
	//ImGui_ImplVulkan_NewFrame();
	//ImGui_ImplGlfw_NewFrame();
	//ImGui::NewFrame();

	//ImGui::ShowDemoWindow(&demoshow);

	drawFrame();
	//ImGui::Render();
	VK_ASSERT(vkDeviceWaitIdle(_device));
}

void Renderer::drawFrame() {

	FrameData current_frame = get_current_frame();
	VK_ASSERT(vkWaitForFences(_device, 1, &current_frame.renderFence, VK_TRUE, UINT64_MAX));

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, current_frame.imageSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		Log.FatalError("Vulkan", "Failed to acquire swap chain image.");
	}
	vkResetFences(_device, 1, &current_frame.renderFence);
	vkResetCommandBuffer(current_frame.commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	if (vkBeginCommandBuffer(current_frame.commandBuffer, &beginInfo) != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to begin recording command buffer.");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapchainImages[imageIndex].framebuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = _swapchain.extent;

	VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(current_frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	recordFrameCmdBuffer(current_frame.commandBuffer, imageIndex);

	updateUniformBuffer(frameNum % MAX_FRAMES_IN_FLIGHT);
	
	vkCmdEndRenderPass(current_frame.commandBuffer);
	if (vkEndCommandBuffer(current_frame.commandBuffer) != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to record command buffer.");
	}

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { current_frame.imageSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &current_frame.commandBuffer;

	VkSemaphore signalSemaphores[] = { current_frame.renderSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(queues[QueueType::graphics], 1, &submitInfo, current_frame.renderFence) != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to submit draw command buffer.");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	result = vkQueuePresentKHR(queues[QueueType::present], &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to present swap chain image!");
	}

	frameNum++;
}

void Renderer::updateUniformBuffer(uint32_t current) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), _swapchain.extent.width / (float)_swapchain.extent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;
	memcpy(get_current_frame().uniformBuffer.info.pMappedData, &ubo, sizeof(ubo));

}

void Renderer::cleanupSwapChain(bool destroySwapchain) {

	for (size_t i = 0; i < swapchainImages.size(); i++)
	{
		vkDestroyFramebuffer(_device, swapchainImages[i].framebuffer, nullptr);
		vkDestroyImageView(_device, swapchainImages[i].imageView, nullptr);

	}

	if (destroySwapchain) vkb::destroy_swapchain(_swapchain);
}

void Renderer::Cleanup() {
	cleanupSwapChain();

	vkDestroyPipeline(_device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(_device, pipelineLayout, nullptr);
	vkDestroyRenderPass(_device, renderPass, nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		_allocator->destroy(&_frames[i].uniformBuffer);
	}

	delete _descriptorAllocator;

	_allocator->destroy(&vertexBuffer);
	_allocator->destroy(&indexBuffer);

	resources::Shader* vertShader = ResourceLoader::Load<resources::Shader>("shaders/shader.vert");
	resources::Shader* fragShader = ResourceLoader::Load<resources::Shader>("shaders/shader.frag");
	vkDestroyShaderModule(_device, vertShader->GetShaderModule(_device), nullptr);
	vkDestroyShaderModule(_device, fragShader->GetShaderModule(_device), nullptr);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		_frames[i].Destroy();
	}

	for (std::pair<const QueueType, VkCommandPool> var: commandPools)
	{
		vkDestroyCommandPool(_device, var.second, nullptr);
	}

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	delete _allocator;
	vkb::destroy_device(_device);

	vkDestroySurfaceKHR(_instance, _surface, nullptr);
	vkb::destroy_instance(_instance);
}

void Renderer::createSwapchain() {
	uint32_t imageCount = MAX_FRAMES_IN_FLIGHT;
	vkb::SwapchainBuilder swapchain_builder{ _device };
	swapchain_builder.use_default_format_selection().use_default_present_mode_selection().use_default_image_usage_flags();
	swapchain_builder.set_desired_min_image_count(imageCount);
	swapchain_builder.set_clipped(true);
	swapchain_builder.set_image_array_layer_count(1);
	if (_swapchain != nullptr) {
		swapchain_builder.set_old_swapchain(_swapchain);
	}
	auto swap_ret = swapchain_builder.build();
	if (!swap_ret) {
		Log.FatalError("Vulkan", "Failed to create swapchain.");
	}

	if (_swapchain != nullptr) {
		vkb::destroy_swapchain(_swapchain);
	}
	_swapchain = swap_ret.value();

	// Setup swapchain images
	std::vector<VkImage> images{};
	vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, nullptr);
	images.resize(imageCount);
	vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, images.data());

	swapchainImages.resize(imageCount);
	for (size_t i = 0; i < imageCount; i++)
	{
		swapchainImages[i].image = images[i];
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapchainImages[i].image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = _swapchain.image_format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(_device, &createInfo, nullptr, &swapchainImages[i].imageView) != VK_SUCCESS) {
			Log.FatalError("Vulkan", "Failed to create swapchain image views.");
		}

	}
}

void Renderer::recreateSwapChain() {
	Log.Debug("Vulkan", "Recreating suboptimal swapchain.");
	int32_t width = 0, height = 0;
	glfwGetFramebufferSize(_window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(_window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(_device);
	cleanupSwapChain(false);
	createSwapchain();
	createFramebuffers();
}


void Renderer::createRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = _swapchain.image_format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to create render pass.");
	}
}

void Renderer::createGraphicsPipeline() {
	resources::Shader* vertShader = ResourceLoader::Load<resources::Shader>("shaders/shader.vert");
	resources::Shader* fragShader = ResourceLoader::Load<resources::Shader>("shaders/shader.frag");

	GraphicsPipelineBuilder pipeline;
	auto attributeDescs = Vertex::getAttributeDescriptions();

	pipeline.AddShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vertShader->GetShaderModule(_device));
	pipeline.AddShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragShader->GetShaderModule(_device));
	pipeline.SetDynamicStates(0, {VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR});
	pipeline.SetVertexInputState(0, {attributeDescs[0], attributeDescs[1]}, {Vertex::getBindingDescription()});
	pipeline.SetInputAssemblyState(0, false, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipeline.SetViewportState(0, {}, {}, 1, 1);
	pipeline.SetRasterizationState(0, VK_CULL_MODE_BACK_BIT, VK_POLYGON_MODE_FILL, VK_FRONT_FACE_COUNTER_CLOCKWISE, false, false, 1.0f, {});
	pipeline.SetMultisampleState(0, false, false, false, 0, VK_SAMPLE_COUNT_1_BIT, nullptr);

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	
	pipeline.SetColorBlendState(0, false, VK_LOGIC_OP_MAX_ENUM, { colorBlendAttachment });
	pipeline.SetPipelineLayout(0, {}, { _descriptorAllocator->GetLayoutObj()});

	pipelineLayout = pipeline.BuildLayout(&_device);
	graphicsPipeline = pipeline.BuildPipeline(&_device, renderPass);
}

void Renderer::createFramebuffers() {
	for (size_t i = 0; i < swapchainImages.size(); i++) {
		VkImageView attachments[] = {
			swapchainImages[i].imageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = _swapchain.extent.width;
		framebufferInfo.height = _swapchain.extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &swapchainImages[i].framebuffer) != VK_SUCCESS) {
			Log.FatalError("Vulkan", "Failed to create framebuffer.");
		}
	}
}

void Renderer::createCommandPools() {
	VkCommandPoolCreateInfo graphicsPoolInfo{};
	graphicsPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	graphicsPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	graphicsPoolInfo.queueFamilyIndex = _device.get_queue_index(QueueType::graphics).value();
	if (vkCreateCommandPool(_device, &graphicsPoolInfo, nullptr, &commandPools[QueueType::graphics]) != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to create graphics command pool.");
	}
	VkCommandPoolCreateInfo transferPoolInfo{};
	transferPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	transferPoolInfo.queueFamilyIndex = _device.get_queue_index(QueueType::transfer).value();
	if (vkCreateCommandPool(_device, &transferPoolInfo, nullptr, &commandPools[QueueType::transfer]) != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to create command pool.");
	}
}

void Renderer::createFrameObjects() {
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		_frames[i]._device = &_device;
		// Command buffer creation
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPools[QueueType::graphics];
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(_device, &allocInfo, &_frames[i].commandBuffer) != VK_SUCCESS) {
			Log.FatalError("Vulkan", "Failed to allocate command buffer.");
		}

		// Uniform buffer creation

		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		_allocator->createBuffer(&_frames[i].uniformBuffer, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);


		// Semaphore and fence creation

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_frames[i].imageSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_frames[i].renderSemaphore) != VK_SUCCESS ||
			vkCreateFence(_device, &fenceInfo, nullptr, &_frames[i].renderFence) != VK_SUCCESS) {

			Log.FatalError("Vulkan", "Failed to create frame synchronization objects.");
		}

	}
}

void Renderer::recordFrameCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {


	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(_swapchain.extent.width);
	viewport.height = static_cast<float>(_swapchain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = _swapchain.extent;
	
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	VkBuffer vertexBuffers[] = { vertexBuffer.buffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &_frames[frameNum % MAX_FRAMES_IN_FLIGHT].descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
}

void Renderer::createInstanceAndDevice() {

	// Create instance

	vkb::InstanceBuilder ibuilder;
	auto system_info_ret = vkb::SystemInfo::get_system_info();
	if (!system_info_ret) { Log.FatalError("Vulkan", "Failed to query system info."); }
	vkb::SystemInfo system_info = system_info_ret.value();

	if (_USE_VK_VALIDATION_LAYERS && !system_info.validation_layers_available) {
		Log.FatalError("Vulkan", "Validation layers requested, but not available.");
	}

	ibuilder.set_app_name("GusEngine");
	ibuilder.set_app_version(1, 0, 0);
	ibuilder.require_api_version(VK_API_VERSION_1_3);
	if constexpr (_USE_VK_VALIDATION_LAYERS) {
		ibuilder.enable_validation_layers(true);
		ibuilder.use_default_debug_messenger();
	}
	
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	for (size_t i = 0; i < extensions.size(); i++)
	{
		if (!system_info.is_extension_available(extensions[i])) { Log.FatalError("Vulkan", "Required instance extension not available:" + string(extensions[i])); }
		ibuilder.enable_extension(extensions[i]);
	}

	// Other extensions

	auto instance_ret = ibuilder.build();
	if (!instance_ret) {
		Log.FatalError("Vulkan", "Failed to create Vulkan instance. " + instance_ret.error().message());
		return;
	}

	_instance = instance_ret.value();
	
	// Create surface

	VkResult err = glfwCreateWindowSurface(instance_ret.value(), _window, NULL, &_surface);
	if (err != VK_SUCCESS) { 
		Log.FatalError("GLFW", "Failed to create window surface.");
	}

	// Select physical device

	vkb::PhysicalDeviceSelector device_selector = vkb::PhysicalDeviceSelector(instance_ret.value());
	device_selector.set_surface(_surface);
	for (size_t i = 0; i < deviceExtensions.size(); i++)
	{
		device_selector.add_required_extension(deviceExtensions[i]);
	}

	VkPhysicalDeviceVulkan13Features required_features{};
	required_features.dynamicRendering = true;
	device_selector.set_required_features_13(required_features);


	auto pdevice_ret = device_selector.select();
	if (!pdevice_ret) {
		Log.FatalError("Vulkan", "Failed to find a suitable GPU. " + pdevice_ret.error().message());
	}

	_physicalDevice = pdevice_ret.value();

	// Create logical device

	vkb::DeviceBuilder device_builder(pdevice_ret.value());
	auto dev_ret = device_builder.build();
	if (!dev_ret) {
		// error
	}
	_device = dev_ret.value();
	
	auto graphics_queue_ret = dev_ret.value().get_queue(QueueType::graphics);
	auto present_queue_ret = dev_ret.value().get_queue(QueueType::present);
	auto transfer_queue_ret = dev_ret.value().get_dedicated_queue(QueueType::transfer);
	if (!graphics_queue_ret || !present_queue_ret || !transfer_queue_ret) {
		Log.FatalError("Vulkan", "Failed to create queues.");
	}
	queues[QueueType::present] = present_queue_ret.value();
	queues[QueueType::graphics] = graphics_queue_ret.value();
	queues[QueueType::transfer] = transfer_queue_ret.value();
}

VkCommandBuffer Renderer::createOneTimeCommandBuffer(QueueType queue)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPools[queue];
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void Renderer::submitOneTimeCommandBuffer(VkCommandBuffer buffer, QueueType queueType)
{
	vkEndCommandBuffer(buffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &buffer;

	vkQueueSubmit(queues[queueType], 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queues[queueType]);

	vkFreeCommandBuffers(_device, commandPools[queueType], 1, &buffer);
}

void Renderer::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
	BufferAlloc stagingBuffer;
	_allocator->createBuffer(&stagingBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
		
	_allocator->copyIntoAllocation(&stagingBuffer, (void*)vertices.data(), 0, bufferSize);

	_allocator->createBuffer(&vertexBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

	VkCommandBuffer commandBuffer = createOneTimeCommandBuffer(QueueType::transfer);
	_allocator->copyBufferToBufferCmd(commandBuffer, &stagingBuffer, &vertexBuffer, bufferSize);
	submitOneTimeCommandBuffer(commandBuffer, QueueType::transfer);

	_allocator->destroy(&stagingBuffer);
}

void Renderer::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
	BufferAlloc stagingBuffer;
	_allocator->createBuffer(&stagingBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

	_allocator->copyIntoAllocation(&stagingBuffer, (void*)indices.data(), 0, bufferSize);

	_allocator->createBuffer(&indexBuffer, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
	VkCommandBuffer commandBuffer = createOneTimeCommandBuffer(QueueType::transfer);
	_allocator->copyBufferToBufferCmd(commandBuffer, &stagingBuffer, &indexBuffer, bufferSize);
	submitOneTimeCommandBuffer(commandBuffer, QueueType::transfer);
	_allocator->destroy(&stagingBuffer);
}

void Renderer::createDescriptorAllocator() {
	VkDescriptorSetLayoutBinding binding{};
	binding.descriptorCount = 1;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	_descriptorAllocator = new DescriptorAllocator(_device, {binding});
	_descriptorAllocator->CreatePool(MAX_FRAMES_IN_FLIGHT);

	std::vector<VkDescriptorSet> descriptorSets = _descriptorAllocator->AllocateDescriptorSets(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < descriptorSets.size(); i++)
	{
		_frames[i].descriptorSet = descriptorSets[i];
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _frames[i].uniformBuffer.buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);
		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = _frames[i].descriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);

	}
}