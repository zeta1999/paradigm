#include "gles/pass.h"
#include "gles/swapchain.h"
#include "gles/material.h"
#include "gles/geometry.h"
#include "gles/buffer.h"
#include "gfx/drawgroup.h"
#include "gfx/drawlayer.h"
#include "gfx/drawcall.h"
#include "gfx/material.h"
#include "gfx/geometry.h"
#include "gfx/buffer.h"
#include "glad/glad_wgl.h"

using namespace core::igles;
using namespace core::resource;

pass::pass(handle<swapchain> swapchain) : m_Swapchain(swapchain) {}


void pass::clear() { m_DrawGroups.clear(); }
void pass::prepare() { m_Swapchain->clear(); }
bool pass::build() { return true; }
void pass::present()
{
	glFinish();
	for(const auto& group : m_DrawGroups)
	{
		for(auto& drawLayer : group.m_Group)
		{
			for(auto& drawCall : drawLayer.second)
			{
				if(drawCall.m_Geometry.size() == 0) continue;
				auto bundle = drawCall.m_Bundle;

				auto matIndices = drawCall.m_Bundle->materialIndices(drawLayer.first.begin(), drawLayer.first.end());

				for(auto index : matIndices)
				{
					bundle->bind_material(index);
					auto gfxmat{bundle->bound()};
					auto mat{gfxmat->resource().get<core::igles::material>()};

					mat->bind();
					for(auto& [gfxGeometryHandle, count] : drawCall.m_Geometry)
					{
						auto geometryHandle = gfxGeometryHandle->resource().get<core::igles::geometry>();
						uint32_t instance_n = bundle->instances(gfxGeometryHandle);
						if(instance_n == 0 /*|| !geometryHandle->compatible(mat)*/) continue;

						geometryHandle->create_vao(
							mat, bundle->m_InstanceData.buffer()->resource().get<core::igles::buffer>(),
							bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle));
						// for(const auto& b : bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle))
						//{
						//	auto buffer = bundle->m_InstanceData.buffer()->resource().get<core::igles::buffer>()->id();
						//	glBindBuffer(GL_ARRAY_BUFFER, buffer);
						//	glEnableVertexAttribArray(b.first);
						//	glEnableVertexAttribArray(b.first + 1);
						//	glEnableVertexAttribArray(b.first + 2);
						//	glEnableVertexAttribArray(b.first + 3);
						//	for(int i = 0; i < 4; ++i)
						//	{
						//		glGetError();
						//		auto offset = b.second + (i * sizeof(float) * 4);
						//		//glVertexAttribPointer(b.first + i, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float),
						//		//					  (void*)(offset));

						//		glVertexAttribPointer(b.first, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4 * 4,
						//							  (void*)(0));
						//		glGetError();
						//		glVertexAttribDivisor(b.first + i, 1);
						//		glGetError();
						//	}
						//	glBindBuffer(GL_ARRAY_BUFFER, 0);
						//}

						geometryHandle->bind(mat, instance_n);

						/*for(const auto& b : bundle->m_InstanceData.bindings(gfxmat, gfxGeometryHandle))
						{
							glDisableVertexAttribArray(b.first);
						}*/
					}
				}
			}
		}
	}
	glFinish();
	m_Swapchain->present();
}

void pass::add(core::gfx::drawgroup& group) noexcept { m_DrawGroups.push_back(group); }