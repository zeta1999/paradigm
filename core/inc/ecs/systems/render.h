#pragma once
#include "ecs/ecs.hpp"
#include "gfx/pass.h"
#include "systems/resource.h"

namespace core::gfx
{
	class context;
	class swapchain;
	class buffer;
} // namespace core::gfx
namespace core::os
{
	class surface;
}
namespace core::ecs::components
{
	struct transform;
	struct renderable;
	struct camera;
} // namespace core::ecs::components
namespace core::ecs::systems
{
	class render
	{
		const psl::mat4x4 clip{1.0f,  0.0f, 0.0f, 0.0f, +0.0f, -1.0f, 0.0f, 0.0f,
							   +0.0f, 0.0f, 0.5f, 0.0f, +0.0f, 0.0f,  0.5f, 1.0f};

		core::ecs::vector<core::ecs::components::transform, core::ecs::READ_ONLY> m_Transforms;
		core::ecs::vector<core::ecs::components::renderable, core::ecs::READ_ONLY> m_Renderers;
		core::ecs::vector<core::ecs::entity> m_RenderableEntities;

		core::ecs::vector<core::ecs::components::camera, core::ecs::READ_ONLY> m_Cameras;
		core::ecs::vector<core::ecs::components::transform, core::ecs::READ_ONLY> m_CameraTransforms;
		core::ecs::vector<core::ecs::entity> m_CameraEntities;

	  public:
		struct framedata
		{
			psl::mat4x4 clipMatrix;
			psl::mat4x4 projectionMatrix;
			psl::mat4x4 modelMatrix;
			psl::mat4x4 viewMatrix;
			psl::mat4x4 WVP;
			psl::mat4x4 VP;
			psl::vec4 ScreenParams;
			psl::vec4 GameTimer;
			psl::vec4 Parameters;
			psl::vec4 FogColor;
			psl::vec4 viewPos;
			psl::vec4 viewDir;
		};

		render(core::resource::handle<core::gfx::context> context,
			   core::resource::handle<core::gfx::swapchain> swapchain,
			   core::resource::handle<core::os::surface> surface, 
			   core::resource::handle<core::gfx::buffer> buffer);
		void announce(core::ecs::state& state);
		void tick(core::ecs::state& state, std::chrono::duration<float> dTime);

	  private:
		void update_buffer(size_t index, const core::ecs::components::transform& transform,
						   const core::ecs::components::camera& camera);

		core::gfx::pass m_Pass;
		core::resource::handle<core::gfx::swapchain> m_Swapchain;
		core::resource::handle<core::os::surface> m_Surface;

		std::vector<memory::segment> fdatasegment;
		core::resource::handle<core::gfx::buffer> m_Buffer;
	};
} // namespace core::ecs::systems