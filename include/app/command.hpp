#pragma once

#include "render/model.hpp"
#include "render/texture.hpp"

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
			Pipeline,
			Model,

			Buffer
		};
		type type;
		std::string name;
		std::any handle;
		bool valid = true;
		bool fake = false;
		std::vector<resource*> childs = {};

		void destroy(vk::Device device)
		{
			if(fake)
				return;

			switch(type)
			{
				case Pipeline:
					device.destroyPipeline(std::any_cast<vk::Pipeline>(handle));
					break;
				case Model:
					std::any_cast<std::shared_ptr<render::model>>(handle).reset();
					break;
				case Buffer:
					//TODO: destroy
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
