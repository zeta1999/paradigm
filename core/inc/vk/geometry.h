﻿#pragma once

namespace core::data
{
	class geometry;
	class material;
}

namespace core::gfx
{
	class context;
	class buffer;
	class material;

	/// \brief describes the driver visible concept of geometry.
	class geometry
	{
		struct binding
		{
			psl::string name;
			memory::segment segment;
			memory::range sub_range;
		};
	public:
		/// \brief constructs, and uploads the geometry data to the buffers.
		/// \param[in] data the geometry source data for this instance.
		/// \param[in] geometryBuffer the buffer that the mesh data will be uploaded to.
		/// \param[in] indicesBuffer the buffer that the indices data will be uploaded to.
		geometry(const UID& uid,
				 core::resource::cache& cache,
				 core::resource::handle<context> context,
				 core::resource::handle<core::data::geometry> data,
				 core::resource::handle<core::gfx::buffer> geometryBuffer,
				 core::resource::handle<core::gfx::buffer> indicesBuffer);
		~geometry();
		geometry(const geometry&) = delete;
		geometry(geometry&&) = delete;
		geometry& operator=(const geometry&) = delete;
		geometry& operator=(geometry&&) = delete;

		/// \returns wether this geometry can be combined with the given material (i.e. it has
		/// all the required channels that the material needs).
		/// \param[in] material the material to check against.
		bool compatible(const core::gfx::material& material) const noexcept;

		/// \brief binds the geometry to the given material for rendering.
		/// \param[in] buffer the command buffer to upload the commands to.
		/// \param[in] material the material to bind with.
		/// \warning only invoke this method in the context of recording draw instructions.
		void bind(vk::CommandBuffer& buffer, const core::gfx::material& material) const noexcept;
		
		/// \returns the geometry data used by this instance.
		core::resource::handle<core::data::geometry> data() const noexcept { return m_Data; };
	private:
		core::resource::handle<core::gfx::context> m_Context;
		core::resource::handle<core::data::geometry> m_Data;
		core::resource::handle<core::gfx::buffer> m_GeometryBuffer;
		core::resource::handle<core::gfx::buffer> m_IndicesBuffer;
		memory::segment m_IndicesSegment;
		memory::range m_IndicesSubRange;
		std::vector<binding> m_Bindings;
		const UID& m_UID;
	};
}
