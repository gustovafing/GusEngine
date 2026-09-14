#include "shader.h"
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>
#include "core/types/type_registry.h"

VkShaderModule resources::Shader::GetShaderModule(VkDevice device)
{
	if (_shaderModule) return _shaderModule;
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = 4 * _spirvBinary.size();
	createInfo.pCode = _spirvBinary.data();
	if (vkCreateShaderModule(device, &createInfo, nullptr, &_shaderModule) != VK_SUCCESS) {
		Log.FatalError("Vulkan", "Failed to create shader module.");
	}

	return _shaderModule;	
}

void resources::Shader::_register_type() {
	using namespace engine_type_registry;
	using namespace ObjectRTTIModel;

	type_registry::register_new_class("Shader", "Resource");
	type_registry::class_expose_method(ObjectMethodDefinition("GetLanguage", Variant::StoredType::Int32), &Shader::GetLanguage);
	type_registry::class_expose_method(ObjectMethodDefinition("GetStage", Variant::StoredType::Int32), &Shader::GetStage);

	const auto langProp = ObjectPropertyDefinition("Language", Variant::StoredType::Int32, ObjectPropertyDefinition::INTERNAL_SAVE, "GetLanguage");
	const auto stageProp = ObjectPropertyDefinition("Stage", Variant::StoredType::Int32, ObjectPropertyDefinition::INTERNAL_SAVE, "GetStage");
	type_registry::class_define_property(langProp);
	type_registry::class_define_property(stageProp);
	type_registry::end_class();
}

resources::Shader::Shader(const std::string& source, ShaderLanguage lang, ShaderStage type)
{
	std::vector<uint32_t> spirv;
	std::string	info_log;
	glslang::InitializeProcess();

	auto messages = static_cast<EShMessages>(EShMsgReadHlsl | EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);

	EShLanguage stage{};

	if (type == ShaderStage::StageVert) stage = EShLangVertex;
	if (type == ShaderStage::StageFrag) stage = EShLangFragment;
	if (type == ShaderStage::StageTessControl) stage = EShLangTessControl;
	if (type == ShaderStage::StageTessEval) stage = EShLangTessEvaluation;
	if (type == ShaderStage::StageGeom) stage = EShLangGeometry;
	if (type == ShaderStage::StageComp) stage = EShLangCompute;

	auto shader_source = source.data();

	glslang::TShader shader(stage);
	shader.setStringsWithLengths(&shader_source, nullptr, 1);
	shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 1);
	shader.setEntryPoint("main");
	shader.setSourceEntryPoint("main");
	shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
	shader.setEnvTarget(glslang::EshTargetSpv, glslang::EShTargetSpv_1_6);

	if (!shader.parse(GetDefaultResources(), 100, false, messages))
	{
		Log.Error("ResourceLoader: GLSLang", "Failed to parse GLSL shader, Error:" + std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()));
		return;
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(messages))
	{
		Log.Error("ResourceLoader: GLSLang", "Failed to compile GLSL shader, Error:" + std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()));
		return;
	}

	glslang::TIntermediate* intermediate = program.getIntermediate(stage);
	if (!intermediate)
	{
		Log.Error("ResourceLoader: GLSLang", "Failed to get generated SPIRV, Error:" + std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()));
		return;
	}

	spv::SpvBuildLogger logger;
	glslang::GlslangToSpv(*intermediate, spirv, &logger);
	glslang::FinalizeProcess();

	_spirvBinary = spirv;
	_stage = type;
	_lang = lang;
}
