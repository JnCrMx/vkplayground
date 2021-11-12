#pragma once

#include "render/phase.hpp"
#include "app/command.hpp"

#include "FileWatch.hpp"

namespace app
{
	class main_phase : public render::phase
	{
		public:
			main_phase(render::window* window);
			~main_phase() override;
			void preload() override;
			void prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews) override;
			void render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence) override;
		protected:
			void render_imgui();
			void window_commands();
			void window_resources();

			bool popup_pipeline();

			std::vector<command> commands;
			std::vector<resource*> resources;

			std::vector<filewatch::FileWatch<std::string>> watchers;

		private:
			vk::UniqueRenderPass renderPass;
			vk::UniquePipelineLayout pipelineLayout;

			std::vector<vk::Image> swapchainImages;
			std::vector<vk::UniqueFramebuffer> framebuffers;

			vk::UniqueCommandPool pool;
			std::vector<vk::UniqueCommandBuffer> commandBuffers;

			vk::UniqueDescriptorPool imguiPool;
	};
}
