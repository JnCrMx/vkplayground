#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include <string>
#include <any>
#include <optional>

namespace app
{
	struct resource
	{
		enum type
		{
			Pipeline
		};
		type type;
		std::string name;
		std::any handle;
		bool valid;

		void destroy(vk::Device device)
		{
			switch(type)
			{
				case Pipeline:
					device.destroyPipeline(std::any_cast<vk::Pipeline>(handle));
					break;
			}
		}
	};
	static resource INVALID_PIPELINE{resource::Pipeline, "invalid", vk::Pipeline{}, false};

	struct command_context
	{
		std::vector<resource*>& resources;
	};

	struct command_state
	{
		bool pipeline_bound;
	};

	class command
	{
		public:
			enum type
			{
				BindPipeline,
				Draw,
				DrawIndexed
			};
			command(type type);

			std::string to_string();
			void execute(vk::CommandBuffer commandBuffer, command_context& ctx);
			std::optional<std::string> simulate(command_state& state, command_context& ctx);

			void show_options(command_context& ctx);
			bool enabled = true;
		protected:
			type type;
			bool fixed;
			std::vector<std::any> args;
	};
}
