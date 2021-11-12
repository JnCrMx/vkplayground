#include "render/utils.hpp"
#include "ShaderLang.h"
#include "render/debug.hpp"

#include <fstream>
#include <stdexcept>
#include <vulkan/vulkan_enums.hpp>

namespace render
{
	vk::UniqueShaderModule createBinaryShader(vk::Device device, std::string file)
	{
		std::ifstream in(file, std::ios_base::binary | std::ios_base::ate);

		size_t size = in.tellg();
		std::vector<uint32_t> buf(size/sizeof(uint32_t));
		in.seekg(0);
		in.read(reinterpret_cast<char*>(buf.data()), size);

		vk::ShaderModuleCreateInfo shader_info({}, size, buf.data());
		vk::UniqueShaderModule shader = device.createShaderModuleUnique(shader_info);
		debugName(device, shader.get(), "Shader Module \""+file+"\"");
		return std::move(shader);
	}

	std::tuple<bool, std::string> compileShader(EShLanguage stage, std::string glslCode, std::vector<unsigned int>& shaderCode)
	{
		const char * shaderStrings[1];
		shaderStrings[0] = glslCode.data();

		glslang::TShader shader(stage);
		shader.setStrings(shaderStrings, 1);
		shader.setEnvInput(glslang::EShSourceGlsl, stage, glslang::EShClientVulkan, 100);
		shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetClientVersion::EShTargetVulkan_1_0);
		shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);
		shader.setEntryPoint("main");
		shader.setSourceEntryPoint("main");

		EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
		if(!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages))
		{
			return {false, std::string(shader.getInfoLog())};
		}

		glslang::TProgram program;
		program.addShader(&shader);
		if(!program.link(messages))
		{
			return {false, std::string(program.getInfoLog())};
		}
		glslang::GlslangToSpv(*program.getIntermediate(stage), shaderCode);

		return {true, ""};
	}

	std::string slurp(std::ifstream& in) {
		std::ostringstream sstr;
		sstr << in.rdbuf();
		return sstr.str();
	}

	vk::UniqueShaderModule createUserShader(vk::Device device, std::string file, vk::ShaderStageFlagBits stage)
	{
		if(file.ends_with(".spv"))
			return createBinaryShader(device, file);
		else
		{
			std::ifstream in(file);
			std::string code = slurp(in);

			std::vector<unsigned int> bin;
			EShLanguage lang;
			switch(stage)
			{
				case vk::ShaderStageFlagBits::eVertex:
					lang = EShLangVertex;
					break;
				case vk::ShaderStageFlagBits::eGeometry:
					lang = EShLangGeometry;
					break;
				case vk::ShaderStageFlagBits::eFragment:
					lang = EShLangFragment;
					break;
				default:
					throw std::runtime_error("stage not supported");
			}
			auto [success, error] = compileShader(lang, code, bin);
			if(!success)
				throw std::runtime_error("shader compilation failed: "+error);
			return device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, bin));
		}
		throw std::runtime_error("unsupported shader format");
	}

	vk::UniqueShaderModule createShader(vk::Device device, std::string file)
	{
		return createBinaryShader(device, "shaders/"+file+".spv");
	};
}
