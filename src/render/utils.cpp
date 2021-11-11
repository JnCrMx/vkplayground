#include "render/utils.hpp"
#include "render/debug.hpp"

#include <fstream>

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

	vk::UniqueShaderModule createUserShader(vk::Device device, std::string file)
	{
		if(file.ends_with(".spv"))
			return createBinaryShader(device, file);
		throw std::runtime_error("unsupported shader format");
	}

	vk::UniqueShaderModule createShader(vk::Device device, std::string file)
	{
		return createBinaryShader(device, "shaders/"+file+".spv");
	};
}
