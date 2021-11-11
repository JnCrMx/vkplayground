#include "app/command.hpp"

#include <sstream>
#include "imgui.h"

namespace app
{
	command::command(enum command::type type) : type(type)
	{
		switch(type)
		{
			case BindPipeline:
				args.push_back(INVALID_PIPELINE);
				break;
			case Draw:
				args.push_back(0u); // vertexCount
				args.push_back(1u); // instanceCount
				args.push_back(0u); // firstVertex
				args.push_back(0u); // firstInstance
				break;
			default:
				break;
		}
	}

	std::string command::to_string()
	{
		std::ostringstream oss;

		oss << "vkCmd";
		switch(type)
		{
			case BindPipeline:
				oss << "BindPipeline";
				break;
			case Draw:
				oss << "Draw";
				break;
			case DrawIndexed:
				oss << "DrawIndexed";
				break;
			default:
				oss << "unknownCommand";
		}
		oss << "(";
		switch(type)
		{
			case BindPipeline:
				oss << std::any_cast<resource>(args[0]).name;
				break;
			case Draw:
				oss << std::any_cast<uint32_t>(args[0]) << ", ";
				oss << std::any_cast<uint32_t>(args[1]) << ", ";
				oss << std::any_cast<uint32_t>(args[2]) << ", ";
				oss << std::any_cast<uint32_t>(args[3]);
				break;
			case DrawIndexed:
				break;
		}
		oss << ")";

		return oss.str();
	}

	void command::execute(vk::CommandBuffer commandBuffer)
	{
		switch(type)
		{
			case BindPipeline: {
				commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, 
					std::any_cast<vk::Pipeline>(std::any_cast<resource>(args[0]).handle));
			} break;
			case Draw: {
				commandBuffer.draw(
					std::any_cast<uint32_t>(args[0]),
					std::any_cast<uint32_t>(args[1]),
					std::any_cast<uint32_t>(args[2]),
					std::any_cast<uint32_t>(args[3]));
			} break;
			case DrawIndexed: {

			} break;
		}
	}

	std::optional<std::string> command::simulate(command_state& state)
	{
		switch(type)
		{
			case BindPipeline: {
				if(!std::any_cast<resource>(args[0]).valid)
					return "invalid pipeline";
				state.pipeline_bound = true;
			} break;
			case Draw: {
				if(!state.pipeline_bound)
					return "no pipeline bound";
			} break;
			case DrawIndexed: {
				if(!state.pipeline_bound)
					return "no pipeline bound";
			} break;
		}
		return std::optional<std::string>();
	}

	void command::show_options(command_context& ctx)
	{
		switch(type)
		{
			case BindPipeline: {
				if(ImGui::BeginCombo("Pipeline", std::any_cast<resource>(args[0]).name.c_str()))
				{
					for(int i=0; i<ctx.resources.size(); i++)
					{
						resource& r = ctx.resources[i];

						if(r.type == resource::Pipeline)
							if(ImGui::Selectable(r.name.c_str(), false))
								args[0] = r;
					}
					ImGui::EndCombo();
				}
			} break;
			case Draw: {
				uint32_t vertexCount = std::any_cast<uint32_t>(args[0]);
				ImGui::InputScalar("vertexCount", ImGuiDataType_U32, &vertexCount);
				args[0] = vertexCount;

				uint32_t instanceCount = std::any_cast<uint32_t>(args[1]);
				ImGui::InputScalar("instanceCount", ImGuiDataType_U32, &instanceCount);
				args[1] = instanceCount;

				uint32_t firstVertex = std::any_cast<uint32_t>(args[2]);
				ImGui::InputScalar("firstVertex", ImGuiDataType_U32, &firstVertex);
				args[2] = firstVertex;

				uint32_t firstInstance = std::any_cast<uint32_t>(args[3]);
				ImGui::InputScalar("firstInstance", ImGuiDataType_U32, &firstInstance);
				args[3] = firstInstance;
			}
			default : {}
		}
	}
}
