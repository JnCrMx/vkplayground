#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

namespace render
{
	vk::UniqueShaderModule createUserShader(vk::Device device, std::string file);
	vk::UniqueShaderModule createShader(vk::Device device, std::string file);
}
