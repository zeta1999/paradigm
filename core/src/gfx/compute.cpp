#include "gfx/compute.h"
#include "gfx/context.h"
#include "gfx/pipeline_cache.h"
#include "data/material.h"

#ifdef PE_GLES
#include "gles/compute.h"
#endif
#ifdef PE_VULKAN
//#include "vk/compute.h"
#endif
using namespace core::gfx;
using namespace core::resource;

compute::compute(core::resource::handle<value_type>& handle) : m_Handle(handle){};
compute::compute(core::resource::cache& cache, const core::resource::metadata& metaData, core::meta::shader* metaFile,
				 core::resource::handle<context> context_handle, core::resource::handle<core::data::material> data,
				 core::resource::handle<pipeline_cache> pipeline_cache)
{
	switch(context_handle->backend())
	{
#ifdef PE_GLES
	case graphics_backend::gles:
		m_Handle << cache.create_using<core::igles::compute>(
			metaData.uid, data, pipeline_cache->resource().get<core::igles::program_cache>());
		break;
#endif
#ifdef PE_VULKAN
		/*case graphics_backend::vulkan:
			m_Handle << cache.create_using<core::ivk::compute>(metaData.uid,
																context_handle->resource().get<core::ivk::context>(),
		   data, pipeline_cache->resource().get<core::ivk::pipeline_cache>());*/
		break;
#endif
	}
}


void compute::dispatch(const psl::static_array<uint32_t, 3>& size)
{
#ifdef PE_GLES
	if(m_Handle.contains<igles::compute>())
	{
		m_Handle.value<igles::compute>().dispatch(size[0], size[1], size[2]);
	}
#endif
}