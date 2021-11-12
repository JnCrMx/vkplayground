#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <ResourceLimits.h>
#include <ShaderLang.h>
#include <GlslangToSpv.h>

namespace render
{
	vk::UniqueShaderModule createUserShader(vk::Device device, std::string file, vk::ShaderStageFlagBits stage);
	vk::UniqueShaderModule createShader(vk::Device device, std::string file);
}
