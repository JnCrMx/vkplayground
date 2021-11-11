#include "app/main_app.hpp"
#include "render/window.hpp"
#include "render/utils.hpp"

#include "imgui.h"
#include "ImGuiFileDialog.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"

namespace app
{
	main_phase::main_phase(render::window* window) : render::phase(window)
	{
	}

	main_phase::~main_phase()
	{
		for(auto& r : resources)
		{
			r.destroy(device);
		}

		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplVulkan_Shutdown();
	}

	void main_phase::preload()
	{
		pool = device.createCommandPoolUnique(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, graphicsFamily));

		{
			vk::AttachmentDescription attachment({}, win->swapchainFormat.format, vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);
			vk::AttachmentReference ref(0, vk::ImageLayout::eColorAttachmentOptimal);
			vk::SubpassDependency dep(VK_SUBPASS_EXTERNAL, 0, 
				vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eColorAttachmentOutput, 
				{}, vk::AccessFlagBits::eColorAttachmentWrite);
			vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, ref);
			vk::RenderPassCreateInfo renderpass_info({}, attachment, subpass, dep);
			renderPass = device.createRenderPassUnique(renderpass_info);
		}

		{
			vk::PipelineLayoutCreateInfo layout_info({}, {}, {});
			pipelineLayout = device.createPipelineLayoutUnique(layout_info);
		}

		std::array<vk::DescriptorPoolSize, 11> sizes = {
			vk::DescriptorPoolSize(vk::DescriptorType::eSampler, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eUniformBufferDynamic, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eStorageBufferDynamic, 1000),
			vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, 1000)
		};
		vk::DescriptorPoolCreateInfo pool_info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, sizes);
		imguiPool = device.createDescriptorPoolUnique(pool_info);

		{
			ImGui::CreateContext();
			ImGui_ImplGlfw_InitForVulkan(win->win, true);

			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = win->instance.get();
			init_info.PhysicalDevice = win->physicalDevice;
			init_info.Device = win->device.get();
			init_info.Queue = win->graphicsQueue;
			init_info.DescriptorPool = imguiPool.get();
			init_info.MinImageCount = win->swapchainImages.size();
			init_info.ImageCount = win->swapchainImages.size();
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			ImGui_ImplVulkan_Init(&init_info, renderPass.get());

			vk::UniqueCommandBuffer cmd = std::move(device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, 1)).front());
			cmd->begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
			ImGui_ImplVulkan_CreateFontsTexture(cmd.get());
			cmd->end();

			vk::SubmitInfo submit_info({}, {}, cmd.get(), {});
			graphicsQueue.submit(submit_info);
			graphicsQueue.waitIdle();

			ImGui_ImplVulkan_DestroyFontUploadObjects();
		}
	}

	void main_phase::prepare(std::vector<vk::Image> swapchainImages, std::vector<vk::ImageView> swapchainViews)
	{
		commandBuffers = device.allocateCommandBuffersUnique(
			vk::CommandBufferAllocateInfo(pool.get(), vk::CommandBufferLevel::ePrimary, swapchainImages.size()));
		this->swapchainImages = swapchainImages;

		for(int i=0; i<swapchainViews.size(); i++)
		{
			vk::FramebufferCreateInfo framebuffer_info({}, renderPass.get(), swapchainViews[i],
				win->swapchainExtent.width, win->swapchainExtent.height, 1);
			framebuffers.push_back(device.createFramebufferUnique(framebuffer_info));
		}
	}

	void main_phase::window_commands()
	{
		ImGui::Begin("Commands", nullptr, ImGuiWindowFlags_MenuBar);

		if(ImGui::BeginMenuBar())
		{
			if(ImGui::BeginMenu("Add"))
			{
				if(ImGui::MenuItem("vkCmdBindPipeline")) {
					commands.push_back(command(command::type::BindPipeline));
				}
				if(ImGui::MenuItem("vkDraw")) {
					commands.push_back(command(command::type::Draw));
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		static int selected = 0;
		
		command_state state = {};
		command_context ctx = {resources};

		{
			ImGui::BeginChild("command buttons", ImVec2(0, 50));

			bool upDisabled = selected <= 0;
			bool downDisabled = selected >= ((int)commands.size())-1;

			ImGui::BeginDisabled(upDisabled);
			if(ImGui::Button("up"))
			{
				std::swap(commands[selected-1], commands[selected]);
				selected--;
			}
			ImGui::EndDisabled();

			ImGui::SameLine();

			ImGui::BeginDisabled(downDisabled);
			if(ImGui::Button("down"))
			{
				std::swap(commands[selected], commands[selected+1]);
				selected++;
			}
			ImGui::EndDisabled();
				
			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("command list", ImVec2(0, 250), true);
			for(int i=0; i<commands.size(); i++)
			{
				command& command = commands[i];

				std::string label = command.to_string();

				constexpr const char* fmt = "#%03d: %s";
				int len = snprintf(nullptr, 0, fmt, i, label.c_str()) + 1;
				auto buf = std::make_unique<char[]>( len );
				snprintf(buf.get(), len, fmt, i, label.c_str());

				if(ImGui::Selectable(buf.get(), selected == i))
					selected = i;

				auto error = command.simulate(state);
				if(error.has_value())
				{
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "X");
					if(ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", error->c_str());
				}
			}
			ImGui::EndChild();
		}

		if(selected < commands.size())
		{
			ImGui::BeginChild("command options", ImVec2(0, 0), true);
			ImGui::Text("%s", commands[selected].to_string().c_str());
			ImGui::SameLine();
			if(ImGui::Button("remove"))
			{
				commands.erase(commands.begin()+selected);
			}
			else
			{
				commands[selected].show_options(ctx);
			}

			ImGui::EndChild();
		}

		ImGui::End();
	}

	struct pipeline_create_shader_stage
	{
		std::string filename;
		vk::ShaderStageFlagBits stage;
		std::string entry;
	};

	struct pipeline_create_state
	{
		std::unique_ptr<char[]> name = std::make_unique<char[]>(256);
		std::vector<pipeline_create_shader_stage> stages = {};

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly = 
			vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList, false);
		vk::PipelineRasterizationStateCreateInfo rasterization = 
			vk::PipelineRasterizationStateCreateInfo({}, false, false, vk::PolygonMode::eFill, {}, vk::FrontFace::eCounterClockwise, false, 0.0f, 0.0f, 0.0f, 1.0f);
	};

	bool main_phase::popup_pipeline()
	{
		float h = ImGui::GetWindowHeight();
		ImGui::BeginChild("content", ImVec2(0, h-40), false);
		
		static pipeline_create_state state = {};

		ImGui::InputText("Name", state.name.get(), 256);

		/*{
			ImGui::BeginChild("Flags", ImVec2(0, 50), true);
			ImGui::Text("Flags");
			ImGui::EndChild();
		}*/
		{
			ImGui::BeginChild("Stages", ImVec2(0, 100), true);
			ImGui::Text("Stages");

			int selected = 0;
			if(ImGui::BeginListBox("##empty"))
			{
				for(int i=0; i<state.stages.size(); i++)
				{
					std::string text = vk::to_string(state.stages[i].stage)+" Shader: "+state.stages[i].filename;
					if(ImGui::Selectable(text.c_str(), selected == i))
						selected = i;
				}
				ImGui::EndListBox();
			}
			ImGui::SameLine();
			{
				ImGui::BeginChild("stages_buttons");
				if(ImGui::Button("+"))
					ImGui::OpenPopup("shader_popup");
				if(ImGui::BeginPopup("shader_popup"))
				{
					static auto filename = std::make_unique<char[]>(1024);
					static auto entry = [](){
						auto a = std::make_unique<char[]>(256);
						std::string main = "main";
						std::copy(main.begin(), main.end(), a.get());
						return std::move(a);
					}();
					ImGui::InputText("Filename", filename.get(), 1024);

					ImGui::SameLine();
					if(ImGui::Button("select"))
						ImGuiFileDialog::Instance()->OpenModal("open_shader", "Open File", ".*,.glsl,.vert,.frag,.spv", ".");
					if(ImGuiFileDialog::Instance()->Display("open_shader"))
					{
						if(ImGuiFileDialog::Instance()->IsOk())
						{
							auto path = ImGuiFileDialog::Instance()->GetSelection().begin()->second;
							std::copy(path.begin(), path.end(), filename.get());
						}
						ImGuiFileDialog::Instance()->Close();
					}

					static vk::ShaderStageFlagBits stageBit = vk::ShaderStageFlagBits::eVertex;
					if(ImGui::BeginCombo("Stage", vk::to_string(stageBit).c_str()))
					{
						for(auto stage : {vk::ShaderStageFlagBits::eVertex, vk::ShaderStageFlagBits::eGeometry, vk::ShaderStageFlagBits::eFragment})
						{
							if(ImGui::Selectable(vk::to_string(stage).c_str(), stage == stageBit))
								stageBit = stage;
						}
						ImGui::EndCombo();
					}
					ImGui::InputText("Entry", entry.get(), 1024);
					if(ImGui::Button("Add"))
					{
						ImGui::CloseCurrentPopup();
						state.stages.push_back({std::string(filename.get()), stageBit, std::string(entry.get())});
					}
					ImGui::EndPopup();
				}

				bool disabled = selected >= state.stages.size();
				if(disabled)
					ImGui::BeginDisabled();
				if(ImGui::Button("-"))
				{
					state.stages.erase(state.stages.begin()+selected);
				}
				if(disabled)
					ImGui::EndDisabled();

				ImGui::EndChild();
			}

			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("VertexInput", ImVec2(0, 100), true);
			ImGui::Text("VertexInput");

			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("InputAssembly", ImVec2(0, 100), true);
			ImGui::Text("InputAssembly");

			if(ImGui::BeginCombo("topology", vk::to_string(state.inputAssembly.topology).c_str()))
			{
				for(auto toplogoy : {vk::PrimitiveTopology::ePointList, vk::PrimitiveTopology::eLineList, vk::PrimitiveTopology::eTriangleList})
				{
					if(ImGui::Selectable(vk::to_string(toplogoy).c_str(), toplogoy == state.inputAssembly.topology))
						state.inputAssembly.topology = toplogoy;
				}
				ImGui::EndCombo();
			}

			ImGui::EndChild();
		}
		{
			ImGui::BeginChild("Rasterization", ImVec2(0, 200), true);
			ImGui::Text("Rasterization");

			ImGui::Checkbox("depthClampEnable", (bool*)&state.rasterization.depthClampEnable);
			ImGui::Checkbox("rasterizerDiscardEnable", (bool*)&state.rasterization.rasterizerDiscardEnable);

			if(ImGui::BeginCombo("polygonMode", vk::to_string(state.rasterization.polygonMode).c_str()))
			{
				for(auto mode : {vk::PolygonMode::eFill, vk::PolygonMode::eLine, vk::PolygonMode::ePoint})
				{
					if(ImGui::Selectable(vk::to_string(mode).c_str(), mode == state.rasterization.polygonMode))
						state.rasterization.polygonMode = mode;
				}
				ImGui::EndCombo();
			}

			if(ImGui::BeginCombo("cullMode", vk::to_string((vk::CullModeFlagBits)(uint32_t)state.rasterization.cullMode).c_str()))
			{
				for(auto mode : {vk::CullModeFlagBits::eNone, vk::CullModeFlagBits::eFront, vk::CullModeFlagBits::eBack, vk::CullModeFlagBits::eFrontAndBack})
				{
					if(ImGui::Selectable(vk::to_string(mode).c_str(), mode == state.rasterization.cullMode))
						state.rasterization.cullMode = mode;
				}
				ImGui::EndCombo();
			}

			if(ImGui::BeginCombo("frontFace", vk::to_string(state.rasterization.frontFace).c_str()))
			{
				for(auto face : {vk::FrontFace::eClockwise, vk::FrontFace::eCounterClockwise})
				{
					if(ImGui::Selectable(vk::to_string(face).c_str(), face == state.rasterization.frontFace))
						state.rasterization.frontFace = face;
				}
				ImGui::EndCombo();
			}

			ImGui::InputFloat("lineWidth", &state.rasterization.lineWidth);

			ImGui::EndChild();
		}

		ImGui::EndChild();
		if(ImGui::Button("Create"))
		{
			std::vector<vk::UniqueShaderModule> shaderModules;
			std::vector<vk::PipelineShaderStageCreateInfo> shaderInfos;
			for(const auto& s : state.stages)
			{
				shaderModules.push_back(render::createUserShader(device, s.filename));
				shaderInfos.push_back(vk::PipelineShaderStageCreateInfo({}, s.stage, shaderModules.back().get(), s.entry.c_str()));
			}

			vk::PipelineVertexInputStateCreateInfo vertex_input({}, {}, {});
			vk::PipelineTessellationStateCreateInfo tesselation({}, {});

			vk::Viewport v{};
			vk::Rect2D s{};
			vk::PipelineViewportStateCreateInfo viewport({}, v, s);

			vk::PipelineMultisampleStateCreateInfo multisample({}, vk::SampleCountFlagBits::e1);
			vk::PipelineDepthStencilStateCreateInfo depthStencil({}, false, false);

			vk::PipelineColorBlendAttachmentState attachment(false);
			attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
			vk::PipelineColorBlendStateCreateInfo colorBlend({}, false, vk::LogicOp::eClear, attachment);

			std::array<vk::DynamicState, 2> dynamicStates{vk::DynamicState::eViewport, vk::DynamicState::eScissor};
			vk::PipelineDynamicStateCreateInfo dynamic({}, dynamicStates);

			vk::GraphicsPipelineCreateInfo pipeline_info({}, shaderInfos, &vertex_input, 
				&state.inputAssembly, &tesselation, &viewport, &state.rasterization, &multisample, &depthStencil, &colorBlend, &dynamic, pipelineLayout.get(), renderPass.get());
			auto [result, pipeline] = device.createGraphicsPipeline(nullptr, pipeline_info);
			if(result == vk::Result::eSuccess)
			{
				resources.emplace_back(resource::type::Pipeline, std::string(state.name.get()), pipeline, true);
			}
			else
			{

			}
			
			state = {};
			ImGui::CloseCurrentPopup();
			return false;
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel"))
		{
			state = {};
			ImGui::CloseCurrentPopup();
			return false;
		}
		return true;
	}

	void main_phase::window_resources()
	{
		ImGui::Begin("Resources", nullptr, ImGuiWindowFlags_MenuBar);

		bool pipeline_popup = false;
		if(ImGui::BeginMenuBar())
		{
			if(ImGui::BeginMenu("Add"))
			{
				if(ImGui::MenuItem("Pipeline"))
					pipeline_popup = true;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if(pipeline_popup)
			ImGui::OpenPopup("pipeline_popup");

		ImGui::SetNextWindowSize(ImVec2(500, 750));
		if(ImGui::BeginPopup("pipeline_popup", ImGuiWindowFlags_Modal))
		{
			pipeline_popup = popup_pipeline();
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void main_phase::render_imgui()
	{
		window_commands();
		window_resources();
	}

	void main_phase::render(int frame, vk::Semaphore imageAvailable, vk::Semaphore renderFinished, vk::Fence fence)
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		render_imgui();
		ImGui::Render();

		bool commandsValid = true;
		{
			command_state state = {};
			for(auto& c : commands)
			{
				if(c.simulate(state).has_value())
				{
					commandsValid = false;
					break;
				}
			}
		}

		auto time = std::chrono::high_resolution_clock::now().time_since_epoch();
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(time);
		auto partialSeconds = std::chrono::duration<float>(time-seconds);

		vk::UniqueCommandBuffer& commandBuffer = commandBuffers[frame];

		commandBuffer->begin(vk::CommandBufferBeginInfo());

		vk::ClearValue color(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f});
		commandBuffer->beginRenderPass(vk::RenderPassBeginInfo(renderPass.get(), framebuffers[frame].get(), 
			vk::Rect2D({0, 0}, win->swapchainExtent), color), vk::SubpassContents::eInline);
		
		vk::Viewport viewport(0.0f, 0.0f, win->swapchainExtent.width, win->swapchainExtent.height, 0.0f, 1.0f);
		vk::Rect2D scissor({0,0}, win->swapchainExtent);
		commandBuffer->setViewport(0, viewport);
		commandBuffer->setScissor(0, scissor);
		if(commandsValid)
		{
			for(auto& c : commands)
			{
				c.execute(commandBuffer.get());
			}
		}
		
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.get());

		commandBuffer->endRenderPass();
		commandBuffer->end();

		vk::PipelineStageFlags waitFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		vk::SubmitInfo submit_info(imageAvailable, waitFlags, commandBuffer.get(), renderFinished);
		graphicsQueue.submit(submit_info, fence);
	}
}
