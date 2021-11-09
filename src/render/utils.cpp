#include "render/utils.hpp"
#include "render/debug.hpp"

#include <fstream>

namespace render
{
	vk::UniqueShaderModule createShader(vk::Device device, std::string file)
	{
		std::ifstream in("shaders/"+file+".spv", std::ios_base::binary | std::ios_base::ate);

		size_t size = in.tellg();
		std::vector<uint32_t> buf(size/sizeof(uint32_t));
		in.seekg(0);
		in.read(reinterpret_cast<char*>(buf.data()), size);

		vk::ShaderModuleCreateInfo shader_info({}, size, buf.data());
		vk::UniqueShaderModule shader = device.createShaderModuleUnique(shader_info);
		debugName(device, shader.get(), "Shader Module \""+file+"\"");
		return std::move(shader);
	};
}
