#include "render/phase.hpp"

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

		private:
			vk::UniqueRenderPass renderPass;

			std::vector<vk::Image> swapchainImages;
			std::vector<vk::UniqueFramebuffer> framebuffers;

			vk::UniqueCommandPool pool;
			std::vector<vk::UniqueCommandBuffer> commandBuffers;

			vk::UniqueDescriptorPool imguiPool;
	};
}
