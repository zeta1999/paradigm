﻿
// core.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#ifdef CORE_EXECUTABLE
#include "header_info.h"
#include "data/window.h" // application data
#include "os/surface.h"  // the OS surface to draw one
#include "vk/context.h"  // the vulkan context
//#include "systems\resource.h" // resource system
#include "vk/swapchain.h" // the gfx swapchain which we'll use as our backbuffer
#include "gfx/pass.h"

#include "spdlog/spdlog.h"
//#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

// drawgroup
#include "gfx/drawgroup.h"

// hello triangle
#include "data/buffer.h"
#include "data/geometry.h"
#include "data/material.h"
#include "vk/buffer.h"
#include "vk/geometry.h"
#include "vk/material.h"
#include "gfx/bundle.h"
#include "vk/pipeline_cache.h"
#include "meta/shader.h"

// hello texture
#include "meta/texture.h"
#include "vk/texture.h"
#include "data/sampler.h"
#include "vk/sampler.h"

#include "systems/input.h"

#include "utility/geometry.h"


#include "math/math.hpp"

#include "../../psl/inc/ecs/state.h"
#include "ecs/components/transform.h"
#include "ecs/components/camera.h"
#include "ecs/components/input_tag.h"
#include "ecs/components/renderable.h"
#include "ecs/components/lifetime.h"
#include "ecs/components/dead_tag.h"
#include "ecs/components/velocity.h"

#include "ecs/systems/render.h"
#include "ecs/systems/fly.h"
#include "ecs/systems/geometry_instance.h"
#include "ecs/systems/lifetime.h"
#include "ecs/systems/death.h"
#include "ecs/systems/attractor.h"
#include "ecs/systems/movement.h"
#include "ecs/systems/gpu_camera.h"

#include "data/framebuffer.h"
#include "vk/framebuffer.h"
#include "gfx/render_graph.h"

#include "ecs/systems/lighting.h"


#include "gles/context.h"
#include "gles/shader.h"
#include "gles/buffer.h"
#include "gles/geometry.h"
#include "gles/texture.h"
#include "gles/sampler.h"
#include "gles/material.h"
#include "gles/swapchain.h"


#include "gfx/limits.h"
#include "gfx/context.h"
#include "gfx/buffer.h"
#include "gfx/geometry.h"
#include "gfx/texture.h"
#include "gfx/swapchain.h"
#include "gfx/sampler.h"

#include "resource/variant_handle.h"

using namespace core;
using namespace core::resource;
using namespace core::gfx;
using namespace core::ivk;
using namespace core::os;

using namespace psl::ecs;
using namespace core::ecs::components;

handle<core::ivk::material> setup_example_material(resource::cache& cache, handle<core::ivk::context> context_handle,
												   handle<pipeline_cache> pipeline_cache,
												   handle<core::ivk::buffer> matBuffer, const psl::UID& texture)
{
	auto vertShaderMeta = cache.library().get<core::meta::shader>("3982b466-58fe-4918-8735-fc6cc45378b0"_uid).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>("4429d63a-9867-468f-a03f-cf56fee3c82e"_uid).value();

	auto textureHandle = create<ivk::texture>(cache, texture);
	textureHandle.load(context_handle);

	// create the sampler
	auto samplerData = create<data::sampler>(cache);
	samplerData.load();
	auto samplerHandle = create<ivk::sampler>(cache);
	samplerHandle.load(context_handle, samplerData);

	// load the example material
	auto matData = create<data::material>(cache);
	matData.load();
	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto stages = matData->stages();
	for(auto& stage : stages)
	{
		if(stage.shader_stage() != core::gfx::shader_stage::fragment) continue;

		auto bindings = stage.bindings();
		bindings[0].texture(textureHandle.RUID());
		bindings[0].sampler(samplerHandle.RUID());
		stage.bindings(bindings);
		// binding.texture()
	}
	matData->stages(stages);

	auto material = create<core::ivk::material>(cache);
	material.load(context_handle, matData, pipeline_cache, matBuffer);


	// psl::serialization::serializer s;
	// s.serialize<psl::serialization::encode_to_format>(*(data::material*)matData.cvalue(),
	//												  utility::application::path::get_path() + "material_example.mat");

	return material;
}


handle<core::ivk::material> setup_depth_material(resource::cache& cache, handle<core::ivk::context> context_handle,
												 handle<pipeline_cache> pipeline_cache,
												 handle<core::ivk::buffer> matBuffer)
{
	auto vertShaderMeta = cache.library().get<core::meta::shader>("404e5c7e-665b-e7c8-35c5-0f92854dd48e"_uid).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>("c7405fe0-232a-7464-5388-86c3f76fffaa"_uid).value();


	auto matData = create<data::material>(cache);

	matData.load();
	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto material = create<ivk::material>(cache);
	material.load(context_handle, matData, pipeline_cache, matBuffer);
	return material;
}

#ifndef PLATFORM_ANDROID
void setup_loggers()
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::time_t now_c						  = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm							  = *std::localtime(&now_c);
	psl::string time;
	time.resize(20);
	strftime(time.data(), 20, "%Y-%m-%d %H-%M-%S", &now_tm);
	time[time.size() - 1] = '/';
	psl::string sub_path  = "logs/" + time;
	if(!utility::platform::file::exists(utility::application::path::get_path() + sub_path + "main.log"))
		utility::platform::file::write(utility::application::path::get_path() + sub_path + "main.log", "");
	std::vector<spdlog::sink_ptr> sinks;
	auto mainlogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "main.log", true);
	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	outlogger->set_level(spdlog::level::level_enum::warn);

	auto ivklogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "ivk.log", true);

	auto igleslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "igles.log", true);

	auto gfxlogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "gfx.log", true);

	auto systemslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "systems.log", true);

	auto oslogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "os.log", true);

	auto datalogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "data.log", true);


	auto corelogger = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
		utility::application::path::get_path() + sub_path + "core.log", true);

	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(corelogger);

	auto logger = std::make_shared<spdlog::logger>("main", begin(sinks), end(sinks));
	spdlog::register_logger(logger);
	core::log = logger;


	sinks.clear();
	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(systemslogger);

	auto system_logger = std::make_shared<spdlog::logger>("systems", begin(sinks), end(sinks));
	spdlog::register_logger(system_logger);
	core::systems::log = system_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(oslogger);

	auto os_logger = std::make_shared<spdlog::logger>("os", begin(sinks), end(sinks));
	spdlog::register_logger(os_logger);
	core::os::log = os_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(datalogger);

	auto data_logger = std::make_shared<spdlog::logger>("data", begin(sinks), end(sinks));
	spdlog::register_logger(data_logger);
	core::data::log = data_logger;

	sinks.clear();
	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(gfxlogger);

	auto gfx_logger = std::make_shared<spdlog::logger>("gfx", begin(sinks), end(sinks));
	spdlog::register_logger(gfx_logger);
	core::gfx::log = gfx_logger;

#ifdef PE_VULKAN
	sinks.clear();
	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(ivklogger);

	auto ivk_logger = std::make_shared<spdlog::logger>("ivk", begin(sinks), end(sinks));
	spdlog::register_logger(ivk_logger);
	core::ivk::log = ivk_logger;
#endif
#ifdef PE_GLES
	sinks.clear();
	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(igleslogger);

	auto igles_logger = std::make_shared<spdlog::logger>("igles", begin(sinks), end(sinks));
	spdlog::register_logger(igles_logger);
	core::igles::log = igles_logger;
#endif
	spdlog::set_pattern("[%8T:%6f] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}
#else
#include "spdlog/sinks/android_sink.h"
void setup_loggers()
{
	core::log		   = spdlog::android_logger_mt("main");
	core::systems::log = spdlog::android_logger_mt("systems");
	core::os::log	  = spdlog::android_logger_mt("os");
	core::data::log	= spdlog::android_logger_mt("data");
	core::gfx::log	 = spdlog::android_logger_mt("gfx");
	core::ivk::log	 = spdlog::android_logger_mt("ivk");
	spdlog::set_pattern("[%8T:%6f] [%=8l] %^%v%$ %@", spdlog::pattern_time_type::utc);
}

#endif

#if defined(MULTI_CONTEXT_RENDERING)
#include <atomic>
#include <shared_mutex>
namespace core::systems
{
	class renderer_view;

	class renderer
	{
	  public:
		explicit renderer(cache* cache) : m_Cache(cache), deviceIndex(0) {}
		~renderer();
		renderer(renderer&& other) = delete;
		renderer& operator=(renderer&& other) = delete;
		renderer(const renderer& other)		  = delete;
		renderer& operator=(const renderer& other) = delete;

		void start() { m_Thread = std::thread(&core::systems::renderer::main, this); }
		void lock() { mutex.lock(); }
		void unlock() { mutex.unlock(); }

		core::systems::renderer_view& create_view(handle<surface> surface);
		cache& get_cache() { return *m_Cache; }

		const std::vector<renderer_view*>& views() { return m_Views; }

		void close(renderer_view* view);

	  private:
		void main();
		std::shared_mutex mutex;
		uint32_t deviceIndex = 0u;
		cache* m_Cache;
		handle<context> m_Context;
		std::thread m_Thread;
		std::atomic<bool> m_ForceClose{false};
		std::atomic<bool> m_Closeable{false};
		std::vector<renderer_view*> m_Views;
	};
	class renderer_view
	{
		friend class renderer;

	  public:
		renderer_view(renderer* renderer, handle<surface> surface, handle<context> context)
			: m_Renderer(renderer), m_Surface(surface), m_Context(context), m_Swapchain(create_swapchain()),
			  m_Pass(m_Context, m_Swapchain)
		{}
		~renderer_view() { m_Surface.unload(true); };
		handle<surface>& current_surface() { return m_Surface; }

	  private:
		handle<swapchain> create_swapchain()
		{
			auto swapchain_handle = create<swapchain>(m_Renderer->get_cache());
			swapchain_handle.load(m_Surface, m_Context);
			return swapchain_handle;
		}
		void present()
		{
			if(m_Surface->open()) m_Pass.present();
		}
		renderer* m_Renderer;
		handle<surface> m_Surface;
		handle<context> m_Context;
		handle<swapchain> m_Swapchain;
		pass m_Pass;
	};

	renderer::~renderer()
	{
		m_ForceClose = true;
		while(!m_Closeable)
		{
		}
		if(m_Thread.joinable())
		{
			m_Thread.join();
		}
		for(auto& view : m_Views) delete(view);
		m_Context.unload();
		if(m_Cache) delete(m_Cache);
	}
	void renderer::main()
	{
		// Utility::OS::RegisterThisThread("RenderThread " + ss.str());
		while(!m_ForceClose)
		{
			mutex.lock();
			{
				for(auto& view : m_Views)
				{
					auto& surf = view->current_surface();
					if(surf->open()) view->present();
				}
			}
			mutex.unlock();
		}

		m_Closeable = true;
	}

	renderer_view& renderer::create_view(handle<surface> surface)
	{
		mutex.lock();
		m_Context = create<context>(*m_Cache);
		if(!m_Context.load(APPLICATION_FULL_NAME, deviceIndex))
		{
			throw std::runtime_error("no vulkan context could be created");
		}

		auto& view = *m_Views.emplace_back(new renderer_view(this, surface, m_Context));
		mutex.unlock();
		return view;
	}

	void renderer::close(renderer_view* view)
	{
		auto it = std::find(std::begin(m_Views), std::end(m_Views), view);
		if(it == std::end(m_Views)) return;
		mutex.lock();
		delete(*it);
		m_Views.erase(it);
		mutex.unlock();
	}
} // namespace core::systems

core::systems::renderer* init(size_t views = 1)
{
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{psl::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

	auto window_data = create_shared<data::window>(cache, "cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data.load();

	auto rend = new core::systems::renderer{&cache};
	for(auto i = 0u; i < views; ++i)
	{
		auto window_handle = create<surface>(cache);
		if(!window_handle.load(window_data))
		{
			// LOG_FATAL << "Could not create a OS surface to draw on.";
			throw std::runtime_error("no OS surface could be created");
		}
		rend->create_view(window_handle);
	}
	rend->start();
	return rend;
}

int main()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	setup_loggers();
	// Utility::OS::RegisterThisThread("Main");

	std::vector<core::systems::renderer*> renderers;
	{
		renderers.push_back(init(4u));
	}
	uint64_t frameCount = 0u;
	while(renderers.size() > 0)
	{
		for(auto i = 0u; i < renderers.size(); i)
		{
			renderers[i]->lock();
			for(auto& view : renderers[i]->views())
			{
				auto& surf = view->current_surface();
				if(surf->open())
					surf->tick();
				else
				{
					renderers[i]->close(view);
					break;
				}
			}
			if(renderers[i]->views().size() == 0)
			{
				delete(renderers[i]);
				renderers.erase(std::begin(renderers) + i);
			}
			else
			{
				++i;
			}
			renderers[i]->unlock();
		}
		++frameCount;
	}
	return 0;
}
#elif defined(DEDICATED_GRAPHICS_THREAD)

static void render_thread(handle<context> context, handle<swapchain> swapchain, handle<surface> surface, pass* pass)
{
	try
	{
		while(surface->open() && swapchain->is_ready())
		{
			pass->prepare();
			pass->present();
		}
	}
	catch(...)
	{
		core::gfx::log->critical("critical issue happened in rendering thread");
	}
}
int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);

	setup_loggers();

	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{psl::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

	auto window_data = create<data::window>(cache, "cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data.load();

	auto window_handle = create<surface>(cache);
	if(!window_handle.load(window_data))
	{
		core::log->critical("could not create a OS surface to draw on.");
		return -1;
	}


	auto context_handle = create<context>(cache);
	if(!context_handle.load(APPLICATION_FULL_NAME))
	{
		core::log->critical("could not create graphics API surface to use for drawing.");
		return -1;
	}

	auto swapchain_handle = create<swapchain>(cache);
	swapchain_handle.load(window_handle, context_handle);
	window_handle->register_swapchain(swapchain_handle);
	pass pass{context_handle, swapchain_handle};

	std::thread tr{&render_thread, context_handle, swapchain_handle, window_handle, &pass};


	uint64_t frameCount = 0u;
	while(window_handle->tick())
	{
		++frameCount;
	}

	tr.join();

	return 0;
}
#else


#if defined(PLATFORM_ANDROID)
bool focused{true};
int android_entry()
{
	setup_loggers();
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	cache cache{psl::meta::library{psl::to_string8_t(libraryPath)}, resource_region.allocator()};

	auto window_data = create<data::window>(cache, UID::convert("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"));
	window_data.load();

	auto surface_handle = create<surface>(cache);
	if(!surface_handle.load(window_data))
	{
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = create<context>(cache);
	if(!context_handle.load(APPLICATION_FULL_NAME, 0))
	{
		core::log->critical("Could not create graphics API surface to use for drawing.");
		return -1;
	}

	auto swapchain_handle = create<swapchain>(cache);
	swapchain_handle.load(surface_handle, context_handle);
	surface_handle->register_swapchain(swapchain_handle);
	context_handle->device().waitIdle();
	pass pass{context_handle, swapchain_handle};
	pass.build();
	uint64_t frameCount										 = 0u;
	std::chrono::high_resolution_clock::time_point last_tick = std::chrono::high_resolution_clock::now();

	int ident;
	int events;
	struct android_poll_source* source;
	bool destroy = false;
	focused		 = true;

	while(true)
	{
		while((ident = ALooper_pollAll(focused ? 0 : -1, NULL, &events, (void**)&source)) >= 0)
		{
			if(source != NULL)
			{
				source->process(platform::specifics::android_application, source);
			}
			if(platform::specifics::android_application->destroyRequested != 0)
			{
				destroy = true;
				break;
			}
		}

		if(destroy)
		{
			ANativeActivity_finish(platform::specifics::android_application->activity);
			break;
		}

		if(swapchain_handle->is_ready())
		{
			pass.prepare();
			pass.present();
		}
		auto current_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed =
			std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);
		last_tick = current_time;
		++frameCount;

		if(frameCount % 60 == 0)
		{
			swapchain_handle->clear_color(vk::ClearColorValue{
				std::array<float, 4>{(float)(std::rand() % 255) / 255.0f, (float)(std::rand() % 255) / 255.0f,
									 (float)(std::rand() % 255) / 255.0f, 1.0f}});
			pass.build();
		}
	}

	return 0;
}


#endif

struct lifetime_test
{
	bool operator()(const core::ecs::components::lifetime& value) const noexcept { return value.value > 0.5f; }
};

auto scaleSystem =
	[](psl::ecs::info& info,
	   psl::ecs::pack<psl::ecs::partial, core::ecs::components::transform, const core::ecs::components::lifetime,
					  psl::ecs::on_condition<lifetime_test, core::ecs::components::lifetime>>
		   pack) {
		for(auto [transform, lifetime] : pack)
		{
			auto remaining = std::min(0.5f, lifetime.value) * 2.0f;
			transform.scale *= remaining;
		}
	};


#if defined(PLATFORM_ANDROID)
static bool initialized = false;

// todo deal with this extern
android_app* platform::specifics::android_application;

void handleAppCommand(android_app* app, int32_t cmd)
{
	switch(cmd)
	{
	case APP_CMD_INIT_WINDOW: initialized = true; break;
	}
}

int32_t handleAppInput(struct android_app* app, AInputEvent* event) {}

void android_main(android_app* application)
{
	application->onAppCmd = handleAppCommand;
	// Screen density
	AConfiguration* config = AConfiguration_new();
	AConfiguration_fromAssetManager(config, application->activity->assetManager);
	// vks::android::screenDensity = AConfiguration_getDensity(config);
	AConfiguration_delete(config);

	while(!initialized)
	{
		int ident;
		int events;
		struct android_poll_source* source;
		bool destroy = false;

		while((ident = ALooper_pollAll(0, NULL, &events, (void**)&source)) >= 0)
		{
			if(source != NULL)
			{
				source->process(application, source);
			}
		}
	}

	platform::specifics::android_application = application;
	android_entry();
}
#elif defined(PLATFORM_WINDOWS) || defined(PLATFORM_LINUX)

void vulkan(memory::region& resource_region, core::resource::cache& cache,
			core::resource::handle<core::os::surface> surface_handle,
			core::resource::handle<core::gfx::context> gfx_context_handle,
			core::resource::handle<core::gfx::swapchain> gfx_swapchain_handle,
	core::resource::handle<core::gfx::buffer> gfx_matBuffer,
			std::vector<core::resource::handle<core::gfx::geometry>> geometryHandles,
			psl::array<core::resource::handle<core::gfx::texture>> textures,
			core::resource::handle<core::ivk::sampler> samplerHandle)
{
	auto context_handle = gfx_context_handle->resource().get<core::ivk::context>();
	auto swapchain_handle = gfx_swapchain_handle->resource().get<core::ivk::swapchain>();
	auto matBuffer		= gfx_matBuffer->resource().get<core::ivk::buffer>();
	auto frameCamBuffer =
		cache.find<core::gfx::buffer>(cache.library().find("GLOBAL_WORLD_VIEW_PROJECTION_MATRIX").value())
			->resource()
			.get<core::ivk::buffer>();

	
	

	// create a staging buffer, this is allows for more advantagous resource access for the GPU
	auto stagingBufferData = create<data::buffer>(cache);
	stagingBufferData.load(vk::BufferUsageFlagBits::eTransferSrc,
						   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
						   memory::region{1024 * 1024 * 32, 4, new memory::default_allocator(false)});
	auto stagingBuffer = create<ivk::buffer>(cache);
	stagingBuffer.load(context_handle, stagingBufferData);

	
	// create a pipeline cache
	auto pipeline_cache = create<core::ivk::pipeline_cache>(cache);
	pipeline_cache.load(context_handle);

	auto material		= setup_example_material(cache, context_handle, pipeline_cache, matBuffer, textures[0].RUID());
	auto material2		= setup_example_material(cache, context_handle, pipeline_cache, matBuffer, textures[1].RUID());
	auto depth_material = setup_depth_material(cache, context_handle, pipeline_cache, matBuffer);

	auto instanceBufferData = create<data::buffer>(cache);
	instanceBufferData.load(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
							vk::MemoryPropertyFlagBits::eDeviceLocal,
							memory::region{1024 * 1024 * 128, 4, new memory::default_allocator(false)});
	auto instanceBuffer = create<ivk::buffer>(cache);
	instanceBuffer.load(context_handle, instanceBufferData, stagingBuffer);

	auto mat_bundle = create<gfx::bundle>(cache);
	mat_bundle.load(instanceBuffer);
	mat_bundle->set(material, 2000);

	auto mat_bundle2 = create<gfx::bundle>(cache);
	mat_bundle2.load(instanceBuffer);
	mat_bundle2->set(material2, 2000);
	mat_bundle2->set(depth_material, 1000);
	
	core::gfx::render_graph renderGraph{};
	auto swapchain_pass = renderGraph.create_pass(context_handle, swapchain_handle);
	// create the ecs
	using psl::ecs::state;

	state ECSState{};

	using namespace core::ecs::components;

	const size_t area			  = 128;
	const size_t area_granularity = 128;
	const size_t size_steps		  = 24;


	utility::platform::file::write(utility::application::path::get_path() + "frame_data.txt",
								   core::profiler.to_string());


	core::ecs::components::transform camTrans{psl::vec3{40, 15, 150}};
	camTrans.rotation = psl::math::look_at_q(camTrans.position, psl::vec3::zero, psl::vec3::up);

	core::ecs::systems::render render_system{ECSState, swapchain_pass};
	render_system.add_render_range(2000, 3000);
	core::ecs::systems::fly fly_system{ECSState, surface_handle->input()};
	core::ecs::systems::gpu_camera gpu_camera_system{ECSState, surface_handle, frameCamBuffer};

	ECSState.declare(psl::ecs::threading::par, scaleSystem);
	ECSState.declare(psl::ecs::threading::par, core::ecs::systems::movement);
	ECSState.declare(psl::ecs::threading::par, core::ecs::systems::death);
	ECSState.declare(psl::ecs::threading::par, core::ecs::systems::lifetime);

	ECSState.declare(psl::ecs::threading::par, core::ecs::systems::attractor);
	ECSState.declare(core::ecs::systems::geometry_instance);

	core::ecs::systems::lighting_system lighting{
		psl::view_ptr(&ECSState), psl::view_ptr(&cache), resource_region, psl::view_ptr(&renderGraph),
		swapchain_pass,			  context_handle,		 surface_handle};
	
	auto eCam		  = ECSState.create(1, std::move(camTrans), psl::ecs::empty<core::ecs::components::camera>{},
								psl::ecs::empty<core::ecs::components::input_tag>{});
	size_t iterations = 25600;
	std::chrono::high_resolution_clock::time_point last_tick = std::chrono::high_resolution_clock::now();

	ECSState.create(
		(iterations > 0) ? 5 : (std::rand() % 100 == 0) ? 0 : 0,
		[&mat_bundle, &geometryHandles, &mat_bundle2](core::ecs::components::renderable& renderable) {
			renderable = {(std::rand() % 2 == 0) ? mat_bundle : mat_bundle2,
						  geometryHandles[std::rand() % geometryHandles.size()]->resource().get<core::ivk::geometry>()};
		},
		psl::ecs::empty<core::ecs::components::transform>{},
		[](core::ecs::components::lifetime& target) { target = {0.5f + ((std::rand() % 50) / 50.0f) * 2.0f}; },
		[&size_steps](core::ecs::components::velocity& target) {
			target = {psl::math::normalize(psl::vec3((float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
													 (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
													 (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f)),
					  ((std::rand() % 5000) / 500.0f) * 8.0f, 1.0f};
		});

	while(surface_handle->tick())
	{
		core::log->info("There are {} renderables alive right now", ECSState.count<renderable>());
		core::profiler.next_frame();
		auto current_time = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> elapsed =
			std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_tick);

		core::log->info("dTime {}ms", elapsed.count());
		last_tick = current_time;
		core::profiler.scope_begin("system tick");
		ECSState.tick(elapsed);
		core::profiler.scope_end();

		core::profiler.scope_begin("presenting");
		renderGraph.present();
		core::profiler.scope_end();

		core::profiler.scope_begin("creating entities");

		ECSState.create(
			(iterations > 0) ? 500 + std::rand() % 150 : (std::rand() % 100 == 0) ? 0 : 0,
			[&mat_bundle, &geometryHandles, &mat_bundle2](core::ecs::components::renderable& renderable) {
				renderable = {(std::rand() % 2 == 0) ? mat_bundle : mat_bundle2,
							  geometryHandles[std::rand() % geometryHandles.size()]->resource().get<core::ivk::geometry>()};
			},
			psl::ecs::empty<core::ecs::components::transform>{},
			[](core::ecs::components::lifetime& target) { target = {0.5f + ((std::rand() % 50) / 50.0f) * 2.0f}; },
			[&size_steps](core::ecs::components::velocity& target) {
				target = {psl::math::normalize(psl::vec3((float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
														 (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
														 (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f)),
						  ((std::rand() % 5000) / 500.0f) * 8.0f, 1.0f};
			});


		if(iterations > 0)
		{
			if(ECSState.filter<core::ecs::components::attractor>().size() < 6)
			{
				ECSState.create(
					2,
					[](core::ecs::components::lifetime& target) {
						target = {5.0f + ((std::rand() % 50) / 50.0f) * 5.0f};
					},
					[&size_steps](core::ecs::components::attractor& target) {
						target = {(float)(std::rand() % size_steps) / size_steps * 3 + 0.5f,
								  (float)(std::rand() % size_steps) / size_steps * 80};
					},
					[&area_granularity, &area, &size_steps](core::ecs::components::transform& target) {
						target = {
							psl::vec3(
								(float)((float)(std::rand() % (area * area_granularity)) / (float)area_granularity) -
									(area / 2.0f),
								(float)((float)(std::rand() % (area * area_granularity)) / (float)area_granularity) -
									(area / 2.0f),
								(float)((float)(std::rand() % (area * area_granularity)) / (float)area_granularity) -
									(area / 2.0f)),

							psl::vec3((float)(std::rand() % size_steps) / size_steps,
									  (float)(std::rand() % size_steps) / size_steps,
									  (float)(std::rand() % size_steps) / size_steps)};
					});
			}
			--iterations;
		}
		core::profiler.scope_end();

		/*if(iterations == 25590)
		{
			ECSState.create(10,
							[](ecs::components::light& var) {
								var = ecs::components::light{psl::vec3{1.0f, 1.0f, 1.0f}, 1.0f,
															 ecs::components::light::type::DIRECTIONAL,
															 std::rand() % 2 == 0};
							},
							core::ecs::components::transform{});
		}*/
	}

	context_handle->device().waitIdle();
}

#include "glad/glad_wgl.h"

void gles(core::resource::cache& cache, core::resource::handle<core::os::surface> surface_handle,
		  core::resource::handle<core::gfx::context> context_handle,
		  core::resource::handle<core::gfx::swapchain> swapchain_handle,
		  core::resource::handle<core::gfx::buffer> matBuffer,
		  std::vector<core::resource::handle<core::gfx::geometry>> geometryHandles,
		  psl::array<core::resource::handle<core::gfx::texture>> textures,
	core::resource::handle<core::igles::sampler> samplerHandle)
{

	auto vertShaderMeta = cache.library().get<core::meta::shader>("f889c133-1ec0-44ea-9209-251cd236f887"_uid).value();
	auto fragShaderMeta = cache.library().get<core::meta::shader>("4429d63a-9867-468f-a03f-cf56fee3c82e"_uid).value();
	

	// load the example material
	auto matData = create<data::material>(cache);
	matData.load();
	matData->from_shaders(cache.library(), {vertShaderMeta, fragShaderMeta});

	auto stages = matData->stages();
	for(auto& stage : stages)
	{
		if(stage.shader_stage() != core::gfx::shader_stage::fragment) continue;

		auto bindings = stage.bindings();
		bindings[0].texture(textures[0].RUID());
		bindings[0].sampler(samplerHandle.RUID());
		stage.bindings(bindings);
	}
	matData->stages(stages);

	auto material = create<core::igles::material>(cache);
	material.load(matData, matBuffer->resource().get<core::igles::buffer>());


	while(surface_handle->tick())
	{
		/* Render here */
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.2f, 0.5f, 0.6f, 1.0f);

		glViewport(0, 0, surface_handle->data().width(), surface_handle->data().height());

		material->bind();

		geometryHandles[2]->resource().get<core::igles::geometry>()->bind(material);
		// geometryHandles[std::rand() % geometryHandles.size()]->bind(material);
		glFinish();
		swapchain_handle->resource().get<core::igles::swapchain>()->present();
	}
}

int entry(gfx::graphics_backend backend)
{
	psl::string libraryPath{utility::application::path::library + "resources.metalib"};

	memory::region resource_region{1024u * 1024u * 20u, 4u, new memory::default_allocator()};
	psl::string8_t environment = "";
	switch(backend)
	{
	case graphics_backend::gles: environment = "gles"; break;
	case graphics_backend::vulkan: environment = "vulkan"; break;
	}
	cache cache{psl::meta::library{psl::to_string8_t(libraryPath), {{environment}}}, resource_region.allocator()};

	auto window_data = create<data::window>(cache, "cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"_uid);
	window_data.load();

	auto surface_handle = create<surface>(cache);
	if(!surface_handle.load(window_data))
	{
		core::log->critical("Could not create a OS surface to draw on.");
		return -1;
	}

	auto context_handle = create<core::gfx::context>(cache);
	context_handle.load(backend, APPLICATION_FULL_NAME);

	context_handle->target_surface(surface_handle.value());

	auto swapchain_handle = create<core::gfx::swapchain>(cache);
	swapchain_handle.load(surface_handle, context_handle);

	// get a vertex and fragment shader that can be combined, we only need the meta
	if(!cache.library().contains("f889c133-1ec0-44ea-9209-251cd236f887"_uid) ||
	   !cache.library().contains("4429d63a-9867-468f-a03f-cf56fee3c82e"_uid))
	{
		core::log->critical(
			"Could not find the required shader resources in the meta library. Did you forget to copy the files over?");
		if(surface_handle) surface_handle->terminate();
		return -1;
	}

	auto matBufferData		 = create<data::buffer>(cache);
	auto storage_buffer_align = core::gfx::limits::storage_buffer_offset_alignment(context_handle);
	auto uniform_buffer_align = core::gfx::limits::uniform_buffer_offset_alignment(context_handle);

	matBufferData.load(
		vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
					   memory::region{1024 * 1024 * 32, static_cast<uint64_t>(storage_buffer_align),
									  new memory::default_allocator(false)});
	auto matBuffer = create<gfx::buffer>(cache);
	matBuffer.load(context_handle, matBufferData);

	// create the buffers to store the model in
	// - memory region which we'll use to track the allocations, this is supposed to be virtual as we don't care to have
	// a copy on the CPU
	// - then we create the vulkan buffer resource to interface with the GPU
	auto vertexBufferData = create<data::buffer>(cache);
	vertexBufferData.load(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
						  vk::MemoryPropertyFlagBits::eDeviceLocal,
						  memory::region{1024 * 1024 * 32, 4, new memory::default_allocator(false)});
	auto vertexBuffer = create<gfx::buffer>(cache);
	vertexBuffer.load(context_handle, vertexBufferData);

	auto indexBufferData = create<data::buffer>(cache);
	indexBufferData.load(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
						 vk::MemoryPropertyFlagBits::eDeviceLocal,
						 memory::region{1024 * 1024 * 32, 4, new memory::default_allocator(false)});
	auto indexBuffer = create<gfx::buffer>(cache);
	indexBuffer.load(context_handle, indexBufferData);

	std::vector<resource::handle<data::geometry>> geometryDataHandles;
	std::vector<resource::handle<gfx::geometry>> geometryHandles;
	geometryDataHandles.push_back(utility::geometry::create_icosphere(cache, psl::vec3::one, 0));
	geometryDataHandles.push_back(utility::geometry::create_cone(cache, 1.0f, 1.0f, 1.0f, 12));
	geometryDataHandles.push_back(utility::geometry::create_quad(cache, 0.5f, -0.5f, -0.5f, 0.5f));
	geometryDataHandles.push_back(utility::geometry::create_spherified_cube(cache, psl::vec3::one, 2));
	geometryDataHandles.push_back(utility::geometry::create_box(cache, psl::vec3::one));
	geometryDataHandles.push_back(utility::geometry::create_sphere(cache, psl::vec3::one, 12, 8));
	for(auto& handle : geometryDataHandles)
	{
		handle.load();
		auto& positionstream =
			handle->vertices(core::data::geometry::constants::POSITION).value().get().as_vec3().value().get();
		core::stream colorstream{core::stream::type::vec3};
		auto& colors			  = colorstream.as_vec3().value().get();
		const float range		  = 1.0f;
		const bool inverse_colors = false;
		for(auto i = 0; i < positionstream.size(); ++i)
		{
			if(inverse_colors)
			{
				float red   = std::max(-range, std::min(range, positionstream[i][0])) / range;
				float green = std::max(-range, std::min(range, positionstream[i][1])) / range;
				float blue  = std::max(-range, std::min(range, positionstream[i][2])) / range;
				colors.emplace_back(psl::vec3(red, green, blue));
			}
			else
			{
				float red   = (std::max(-range, std::min(range, positionstream[i][0])) + range) / (range * 2);
				float green = (std::max(-range, std::min(range, positionstream[i][1])) + range) / (range * 2);
				float blue  = (std::max(-range, std::min(range, positionstream[i][2])) + range) / (range * 2);
				colors.emplace_back(psl::vec3(red, green, blue));
			}
		}

		handle->vertices(core::data::geometry::constants::COLOR, colorstream);

		geometryHandles.emplace_back(create<gfx::geometry>(cache));
		geometryHandles[geometryHandles.size() - 1].load(context_handle, handle, vertexBuffer, indexBuffer);
	}

	// still todo

	psl::array<core::resource::handle<core::gfx::texture>> textures;
	textures.emplace_back(create<core::gfx::texture>(cache, "3c4af7eb-289e-440d-99d9-20b5738f0200"_uid));
	textures.emplace_back(create<core::gfx::texture>(cache, "7f24e25c-8b94-4da4-8a31-493815889698"_uid));

	for(auto& texture : textures) texture.load(context_handle);

	
	// create the sampler
	auto samplerData = create<data::sampler>(cache);
	samplerData.load();
	auto samplerHandle = create<gfx::sampler>(cache);
	samplerHandle.load(context_handle, samplerData);

	// create the buffer that we'll use for storing the WVP for the shaders;
	auto frameCamBufferData = create<data::buffer>(cache);
	frameCamBufferData.load(vk::BufferUsageFlagBits::eUniformBuffer,
							vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
							resource_region
								.create_region(sizeof(core::ecs::systems::gpu_camera::framedata) * 128,
											   uniform_buffer_align,
											   new memory::default_allocator(true))
								.value());
	// memory::region{sizeof(framedata)*128, context_handle->properties().limits.minUniformBufferOffsetAlignment, new
	// memory::default_allocator(true)});
	auto frameCamBuffer = create<gfx::buffer>(cache);
	frameCamBuffer.load(context_handle, frameCamBufferData);
	cache.library().set(frameCamBuffer.ID(), "GLOBAL_WORLD_VIEW_PROJECTION_MATRIX");


	switch(backend)
	{
	case graphics_backend::gles:
		gles(cache, surface_handle, context_handle, swapchain_handle, matBuffer, geometryHandles, textures,
			 samplerHandle->resource().get<core::igles::sampler>());
		break;
	case graphics_backend::vulkan:
		vulkan(resource_region, cache, surface_handle, context_handle, swapchain_handle, matBuffer, geometryHandles,
			   textures, samplerHandle->resource().get<core::ivk::sampler>());
		break;
	}
	return 0;
}

int main()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

#ifdef _MSC_VER
	{ // here to trick the compiler into generating these types to get UUID natvis support
		dummy::hex_dummy_high hex_dummy_high{};
		dummy::hex_dummy_low hex_dummy_lowy{};
	}
#endif

	setup_loggers();

	return entry(graphics_backend::gles);
}
#endif

#endif
#endif