#include "engine.h"
#include "filesystem/resource_loader.h"
#include "types/type_registry.h"

#include "filesystem/engine_io.h"

void Engine::Run(vector<string> args) {
	engine_type_registry::type_registry::register_all_types();
	vector<string> files = EngineIO::FileSystem::GetFilesInDir("./", true);
	Init();
	MainLoop();
	Cleanup();
}

void Engine::Init() {
	EngineIO::FileSystem::Init();
	ResourceLoader::Init();

	initWindow();
	_renderer.Init(_window);
}

void Engine::initWindow() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
	Log.Info("Core", "GLFW: Window Init Done");
}

void Engine::framebufferResizeCallback(GLFWwindow* window, int32_t width, int32_t height) {
	const auto app = static_cast<Engine*>(glfwGetWindowUserPointer(window));
	app->_framebufferChanged = true;
}

void Engine::MainLoop() {
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
		if (_framebufferChanged) {
			_renderer.RefreshFramebuffer();
			_framebufferChanged = false;
		}

		_renderer.ProcessFrame();
	}

}

void Engine::Cleanup() {
	Log.Info("Core", "Cleaning up vulkan");
	_renderer.Cleanup();
	Log.Info("Core", "Cleaning up resources");
	ResourceLoader::Cleanup();
	Log.Info("Core", "Exiting");
	glfwDestroyWindow(_window);
	glfwTerminate();
}