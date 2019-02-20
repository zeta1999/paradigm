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
#include "gfx/material.h"
#include "gfx/pipeline_cache.h"
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

using namespace core;
using namespace core::resource;
using namespace core::gfx;
using namespace core::os;

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

	sinks.clear();
	sinks.push_back(mainlogger);
	// sinks.push_back(outlogger);
	sinks.push_back(ivklogger);

	auto ivk_logger = std::make_shared<spdlog::logger>("ivk", begin(sinks), end(sinks));
	spdlog::register_logger(ivk_logger);
	core::ivk::log = ivk_logger;

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
		uint32_t lock(uint32_t v)
		{
			mutex.lock();
			return v;
		}

	  public:
		explicit renderer(cache* cache)
			: m_Cache(cache), deviceIndex(lock(0)), m_Thread(&core::systems::renderer::main, this)
		{}
		~renderer();
		renderer(renderer&& other) = delete;
		renderer& operator=(renderer&& other) = delete;
		renderer(const renderer& other)		  = delete;
		renderer& operator=(const renderer& other) = delete;

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
		psl::string8_tstream ss;
		ss << m_Thread.get_id();
		// Utility::OS::RegisterThisThread("RenderThread " + ss.str());
		m_Context = create<context>(*m_Cache);
		if(!m_Context.load(APPLICATION_FULL_NAME, deviceIndex))
		{
			LOG_FATAL << "Could not create graphics API surface to use for drawing.";
			return throw std::runtime_error("no vulkan context could be created");
		}
		mutex.unlock();
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
	psl::string libraryPath{utility::application::path::library + _T("resources.metalib"_sv)};

	cache* cache = new core::resource::cache(psl::meta::library{psl::from_string8_t(libraryPath)}, 1024u * 1024u * 20u,
											 4u, new memory::default_allocator());

	auto window_data = create_shared<data::window>(*cache, UID::convert("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"));
	window_data.load();

	auto rend = new core::systems::renderer{cache};
	for(auto i = 0u; i < views; ++i)
	{
		auto window_handle = create<surface>(*cache);
		if(!window_handle.load(window_data))
		{
			LOG_FATAL << "Could not create a OS surface to draw on.";
			throw std::runtime_error("no OS surface could be created");
		}
		rend->create_view(window_handle);
	}
	return rend;
}

int main()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	Utility::Logger::Init("Main");
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

	cache cache{psl::meta::library{psl::from_string8_t(libraryPath)}, 1024u * 1024u * 20u, 4u,
				new memory::default_allocator()};

	auto window_data = create<data::window>(cache, UID::convert("cd61ad53-5ac8-41e9-a8a2-1d20b43376d9"));
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

struct timer final
{
	timer() : start{std::chrono::high_resolution_clock::now()} {}

	void count()
	{
		auto now = std::chrono::high_resolution_clock::now();
		std::cout << (std::chrono::duration<double>(now - start) - paused).count() * 1000 << "ms\n";
		start = std::chrono::high_resolution_clock::now();
	}

	void pause() { pause_start = std::chrono::high_resolution_clock::now(); }
	void resume()
	{
		auto now = std::chrono::high_resolution_clock::now();
		paused += std::chrono::duration<double>(now - pause_start);
	}

	void reset() { start = std::chrono::high_resolution_clock::now(); }

  private:
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::chrono::time_point<std::chrono::high_resolution_clock> pause_start;
	std::chrono::duration<double> paused{0.0};
};

class benchmark
{
  public:
	template <typename F>
	benchmark(F&& fnc, const size_t iterations)
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> start{};
		results.reserve(iterations);
		for(size_t i = 0; i < iterations; ++i)
		{
			start = std::chrono::high_resolution_clock::now();
			std::invoke(fnc);
			results.emplace_back(
				std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() * 1000.0);
		}
		std::sort(std::begin(results), std::end(results));
	}

	template <typename F, typename F2>
	benchmark(F2&& buildup, F&& fnc, const size_t iterations)
	{
		std::chrono::time_point<std::chrono::high_resolution_clock> start{};
		results.reserve(iterations);
		for(size_t i = 0; i < iterations; ++i)
		{
			auto res{std::invoke(buildup)};
			start = std::chrono::high_resolution_clock::now();
			std::apply(fnc, res);
			results.emplace_back(
				std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() * 1000.0);
		}
		std::sort(std::begin(results), std::end(results));
	}


	double time(size_t percentile)
	{
		percentile   = std::max<size_t>(std::min<size_t>(100, percentile), 0);
		size_t index = (100.0 / (double)(results.size() + 1)) * (double)percentile;
		return results[index];
	}

	template <typename... size_t>
	void log(spdlog::logger& logger, size_t... percentiles)
	{
		std::string format{"    {} iterations [ "};
		(format.append(std::to_string(percentiles) + "% {}ms | "), ...);
		format.resize(format.size() - 2);
		format.append(" ]");
		logger.info(format.c_str(), results.size(), time(percentiles)...);
	};

  private:
	std::vector<double> results;
};


struct position
{
	std::uint64_t x;
	std::uint64_t y;
};

struct rotation
{
	std::uint64_t x;
	std::uint64_t y;
};
void Construct(spdlog::logger& log)
{
	log.info("Constructing 1000000 entities");

	benchmark b{[]() { return std::tuple<psl::ecs::state>(); },
				[](psl::ecs::state& state) {
					for(std::uint64_t i = 0; i < 1000000L; i++)
					{
						state.create();
					}
				},
				100};

	b.log(log, 0, 25, 50, 80, 95, 98, 100);
}

void ConstructMany(spdlog::logger& log)
{
	log.info("Constructing 1000000 entities at once");
	benchmark b{[]() { return std::tuple<psl::ecs::state>(); }, [](psl::ecs::state& state) { state.create(1000000L); },
				100};

	b.log(log, 0, 25, 50, 80, 95, 98, 100);
}

void ConstructManyOneComponent(spdlog::logger& logger)
{
	logger.info("Constructing 1000000 entities /w one component at once");
	benchmark b{[]() { return std::tuple<psl::ecs::state>(); },
				[](psl::ecs::state& state) { state.create<position>(1000000u); }, 100};
	b.log(logger, 0, 25, 50, 80, 95, 98, 100);
}

void Destroy(spdlog::logger& logger)
{
	logger.info("Destroying 1000000 entities");

	benchmark b{[]() {
					psl::ecs::state s{};
					s.create<position>(1000000u);
					return std::tuple<psl::ecs::state>(std::move(s));
				},
				[](psl::ecs::state& state) {
					for(std::uint32_t i = 0; i < 1000000L; i++)
					{
						state.destroy(i);
					}
				},
				100};
	b.log(logger, 0, 25, 50, 80, 95, 98, 100);
}

void DestroyMany(spdlog::logger& logger)
{
	psl::ecs::state state{};
	logger.info("Constructing 1000000 entities /w one component at once");

	benchmark b{[]() {
					psl::ecs::state s{};
					s.create<position>(1000000u);
					return std::tuple<psl::ecs::state>(std::move(s));
				},
				[](psl::ecs::state& state) {
					state.destroy(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 1000000u}});
				},
				100};
	b.log(logger, 0, 25, 50, 80, 95, 98, 100);
}

void DestroyOneLess(spdlog::logger& logger)
{
	logger.info("Destroying 999999u entities out of 1000000u at once");

	benchmark b{[]() {
					psl::ecs::state s{};
					s.create<position>(1000000u);
					return std::tuple<psl::ecs::state>(std::move(s));
				},
				[](psl::ecs::state& state) {
					state.destroy(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 999999u}});
				},
				100};
	b.log(logger, 0, 25, 50, 80, 95, 98, 100);
}

void RemovingOne(spdlog::logger& logger)
{
	logger.info("Removing 1000000 components from entities");

	benchmark b{[]() {
					psl::ecs::state s{};
					auto entities = s.create<position>(1000001u);
					entities.resize(entities.size() - 1);
					return std::tuple<psl::array<psl::ecs::entity>, psl::ecs::state>(std::move(entities), std::move(s));
				},
				[](psl::array<psl::ecs::entity>& entities, psl::ecs::state& state) {
					state.remove_components<position>(entities);
				},
				100};
	b.log(logger, 0, 25, 50, 80, 95, 98, 100);
}

void RemovingOneFromMany(spdlog::logger& logger)
{
	logger.info("Removing 1000000 components from entities with 2 components");

	benchmark b{[]() {
					psl::ecs::state s{};
					auto entities = s.create<position, rotation>(1000001u);
					entities.resize(entities.size() - 1);
					return std::tuple<psl::array<psl::ecs::entity>, psl::ecs::state>(std::move(entities), std::move(s));
				},
				[](psl::array<psl::ecs::entity>& entities, psl::ecs::state& state) {
					state.remove_components<position>(entities);
				},
				100};
	b.log(logger, 0, 25, 50, 80, 95, 98, 100);
}

void IterateCreateDeleteSingleComponent(spdlog::logger& logger)
{
	psl::ecs::state state{};
	logger.info("Looping 10000 times creating and deleting a random number of entities");
	timer timer;

	for(int i = 0; i < 10000; i++)
	{
		state.create<position>(10000);
		timer.pause();
		psl::array<psl::ecs::entity> range{state.entities<position>()};
		timer.resume();

		// range.resize(
		//	std::distance(std::begin(range), std::remove_if(std::begin(range), std::end(range),
		//													[](const auto& r) { return std::rand() % 2 == 0; })));
		////std::sort(std::begin(range), std::end(range));
		// auto start_index = range[0];
		// auto end_index   = start_index;
		// auto it			 = std::next(std::begin(range));
		// psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>> ranges;
		// for(auto end = std::end(range); it != end; ++it)
		//{
		//	++end_index;
		//	if((*it) != end_index)
		//	{
		//		ranges.emplace_back(std::make_pair(start_index, end_index));
		//		start_index = (*it);
		//		end_index   = start_index;
		//	}
		//}
		// ranges.emplace_back(std::make_pair(start_index, end_index + 1));
		// state.destroy(ranges);
		std::for_each(std::begin(range), std::end(range), [&state](auto e) {
			if(rand() % 2 == 0) state.destroy(e);
		});
	}

	timer.count();
}

void IterateOneComponentView(spdlog::logger& log)
{
	psl::ecs::state state{};
	size_t count = 0;
	log.info("Iterating over 1000000 entities that have one component");

	state.create<position>(1000000);
	auto view = state.view<position>();


	benchmark set_values{[&view]() { std::for_each(std::begin(view), std::end(view), [](position& e) { e.x = 5; }); },
						 250};
	benchmark const_values{[&view, &count]() {
							   std::for_each(std::begin(view), std::end(view),
											 [&count](const position& e) { count += e.x; });
						   },
						   250};

	set_values.log(log, 0, 25, 50, 80, 95, 98, 100);
	const_values.log(log, 0, 25, 50, 80, 95, 98, 100);
}

// void IterateOneComponent()
//{
//	psl::ecs::state state{};
//	size_t count = 0;
//	std::cout << "Iterating over 1000000 entities that have one component" << std::endl;
//
//	state.create<position>(1000000);
//	auto view = state.filter<position>();
//	std::cout << std::to_string(view.size()) << std::endl;
//	timer timer;
//
//	for(auto e : view)
//	{
//		e->x = 5;
//	}
//	timer.count();
//
//	for(auto e : view)
//	{
//		count += e->x;
//	}
//	timer.count();
//	std::cout << std::to_string(count) << std::endl;
//}
int entry()
{
#ifdef PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
	setup_loggers();

	// for(auto n = 0; n < 10000; ++n)
	//{
	//	psl::sparse_array<size_t, size_t, 4> set;
	//	for(auto i = 0; i < 100; ++i) set[i] = i;

	//	for(auto i = 0; i < 100; ++i) assert(set[i] == i);

	//	/*for(auto i : set.indices())
	//	{
	//		if(set[i] != set.dense()[i])
	//			debug_break();
	//	}*/

	//	for(auto c = 0; c < 100; ++c)
	//	{
	//		auto start = std::rand() % 200;
	//		auto size = std::rand() % 6;
	//		auto before{set};
	//		set.erase(start, start + size);

	//		/*for(auto i = 0; i < set.size(); ++i)
	//		{
	//			if(set[set.indices()[i]] != set.dense()[i])
	//				debug_break();
	//		}*/
	//	}
	//}


	// IterateCreateDeleteSingleComponent();

	auto outlogger = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	spdlog::logger logger{"", outlogger};

	// for(auto i = 0; i < 10; ++i)
	{
		IterateOneComponentView(logger);
		Construct(logger);
		ConstructMany(logger);
		ConstructManyOneComponent(logger);
		Destroy(logger);
		DestroyMany(logger);
		DestroyOneLess(logger);
		IterateOneComponentView(logger);
		RemovingOne(logger);
		RemovingOneFromMany(logger);
	}
	return 0;
	// create the ecs
	psl::ecs::state ECSState{};

	using namespace core::ecs::components;

	const size_t area			  = 128;
	const size_t area_granularity = 128;
	const size_t size_steps		  = 24;

	timer timer;
	auto entities = ECSState.create(1000000000L);
	timer.count();
	ECSState.add_components<position>(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 1000000u}});
	ECSState.remove_components<position>(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 1000000u}});
	timer.reset();
	ECSState.add_components<position>(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 1000000u}});
	timer.count();
	ECSState.add_components<renderable, transform, lifetime, velocity>(
		psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{100000u, 200000u}});
	timer.count();
	ECSState.add_components(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 100000u}},
							[](renderable& target) {}, psl::ecs::empty<transform>{},
							[](lifetime& target) { target.value = 0.5f + ((std::rand() % 50) / 50.0f) * 2.0f; },
							[&size_steps](velocity& target) {
								target.direction = psl::math::normalize(
									psl::vec3((float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
											  (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f,
											  (float)(std::rand() % size_steps) / size_steps * 2.0f - 1.0f));
								target.force   = ((std::rand() % 5000) / 500.0f) * 8.0f;
								target.inertia = 1.0f;
							});
	timer.count();
	ECSState.remove_components<position>(psl::array<std::pair<psl::ecs::entity, psl::ecs::entity>>{{0u, 5000u}});
	timer.count();
	auto eees = ECSState.filter<position>();
	timer.count();
	// ECSState.add_components<core::ecs::components::transform>(entities);
	// timer.count();
	return 0;
}

static bool initialized = false;
#if defined(PLATFORM_ANDROID)

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
int main() { return entry(); }
#endif

#endif
#endif