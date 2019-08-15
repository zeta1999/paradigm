﻿#pragma once
#include "serialization.h"
#include "gfx/types.h"
#include "fwd/resource/resource.h"


namespace core::data
{
	/// \brief describes the data to build a core::ivk::sampler instance
	class sampler final
	{
		friend class psl::serialization::accessor;
	public:
		sampler(core::resource::cache& cache, const core::resource::metadata& metaData, psl::meta::file* metaFile) noexcept;
		~sampler() = default;
		sampler(const sampler& other) = delete;
		sampler(sampler&& other) = delete;
		sampler& operator=(const sampler& other) = delete;
		sampler& operator=(sampler&& other) = delete;

		/// \brief returns if mipmapping is enabled or not
		/// \returns if mipmapping is enabled or not
		bool mipmaps() const;
		/// \brief enables or disables mipmapping on this sampler.
		/// \param[in] value true for enabling mipmapping.
		void mipmaps(bool value);

		/// \brief returns the mip_bias that might be present on this sampler.
		/// \returns the mip_bias that might be present on this sampler.
		/// \note mipmaps() should be true for this to have an effect.
		float mip_bias() const;
		/// \brief sets the mip bias when sampling mipmaps.
		/// \param[in] value the bias to apply to mipmap sampling.
		/// \note mipmaps() should be true for this to have an effect.
		void  mip_bias(float value);

		/// \brief returns the mode for mipmap texture lookups.
		/// \returns the mode for mipmap texture lookups.
		core::gfx::sampler_mipmap_mode mip_mode() const;

		/// \brief sets the mode for mipmap texture lookups.
		/// \param[in] value the mode to change to.
		void mip_mode(core::gfx::sampler_mipmap_mode value);

		/// \brief returns the minimal mipmap LOD this instance has set.
		/// \returns the minimal mipmap LOD this instance has set.
		float mip_minlod() const;

		/// \brief sets the minimal mipmap LOD offset for this instance.
		/// \param[in] value the value to set.
		void mip_minlod(float value);

		/// \brief returns the max mipmap LOD this instance has set.
		/// \returns the max mipmap LOD this instance has set.
		/// \todo this value is currently ignored in core::core::ivk::sampler.
		float mip_maxlod() const;
		/// \brief sets the minimal mipmap max LOD for this instance.
		/// \param[in] value the value to set.
		void mip_maxlod(float value);

		/// \brief returns how this instances deals with texture tiling in the U-axis.
		/// \returns how this instances deals with texture tiling in the U-axis.
		core::gfx::sampler_address_mode addressU() const;
		/// \brief sets how this instance should deal with texture tiling in the U-axis.
		/// \param[in] value the mode to set this instance to.
		void addressU(core::gfx::sampler_address_mode value);

		/// \brief returns how this instances deals with texture tiling in the V-axis.
		/// \returns how this instances deals with texture tiling in the V-axis.
		core::gfx::sampler_address_mode addressV() const;
		/// \brief sets how this instance should deal with texture tiling in the V-axis.
		/// \param[in] value the mode to set this instance to.
		void addressV(core::gfx::sampler_address_mode value);

		/// \brief returns how this instances deals with texture tiling in the W-axis.
		/// \returns how this instances deals with texture tiling in the W-axis.
		core::gfx::sampler_address_mode addressW() const;
		/// \brief sets how this instance should deal with texture tiling in the W-axis.
		/// \param[in] value the mode to set this instance to.
		void addressW(core::gfx::sampler_address_mode value);

		/// \brief returns the border color that will be used during texture lookups.
		/// \returns the border color that will be used during texture lookups.
		core::gfx::border_color border_color() const;

		/// \brief sets the border color that should be used during texture lookups.
		/// \param[in] value the new border color value.
		void border_color(core::gfx::border_color value);

		/// \brief returns if anisotropic filtering is enabled.
		/// \returns if anisotropic filtering is enabled.
		bool anisotropic_filtering() const;

		/// \brief call this to enable or disable anisotropic filtering.
		/// \param[in] value set to true to enable anisotropic filtering.
		/// \note if the current core::ivk::context doesn't support anisotropic filtering, 
		/// then this value will be ingored upstream (core::core::ivk::sampler).
		void anisotropic_filtering(bool value);

		/// \brief returns the max anistropic value for this instance.
		/// \returns the max anistropic value for this instance.
		float max_anisotropy() const;
		/// \brief sets the max anistropic value for this instance.
		/// \param[in] value the value to set the max anisotropic value.
		void max_anisotropy(float value);

		/// \brief returns if compare operations have been enabled or not.
		/// \returns if compare operations have been enabled or not.
		bool compare_mode() const;
		/// \brief enables or disables compare ops.
		/// \param[in] value set to true to enable compare ops.
		void compare_mode(bool value);

		/// \brief returns what compare op would be used if compare_mode() is true.
		/// \returns what compare op would be used if compare_mode() is true.
		core::gfx::compare_op compare_op() const;
		/// \brief sets the compare op to a new value.
		/// \param[in] value the new compare op to use.
		/// \note compare_op() will only be used if compare_mode() is true. You can still set this value regardless however.
		void compare_op(core::gfx::compare_op value);

		/// \brief returns the filtering mode to use when dealing with minification.
		/// \returns the filtering mode to use when dealing with minification.
		core::gfx::filter filter_min() const;
		
		/// \brief sets the filtering mode to use when dealing with minification.
		/// \param[in] value the new filter mode to use.
		void filter_min(core::gfx::filter value);

		/// \brief returns the filtering mode to use when dealing with magnification.
		/// \returns the filtering mode to use when dealing with magnification.
		core::gfx::filter filter_max() const;
		/// \brief sets the filtering mode to use when dealing with magnification.
		/// \param[in] value the new filter mode to use.
		void filter_max(core::gfx::filter value);

		/// \brief returns if the coordinates for this sampler will be normalized or not.
		/// \returns if the coordinates for this sampler will be normalized or not.
		bool normalized_coordinates() const;
		/// \brief enables or disables coordinate normalization when this sampler is used.
		/// \param[in] value enables or disables the behaviour.
		void normalized_coordinates(bool value);
	private:
		template <typename S>
		void serialize(S& serializer)
		{
			serializer << m_MipMapped;
			if(m_MipMapped.value)
				serializer << m_MipMapMode << m_MipLodBias << m_MinLod << m_MaxLod;
			serializer << m_AddressModeU << m_AddressModeV << m_AddressModeW << m_BorderColor << m_AnisotropyEnable;
			if(m_AnisotropyEnable.value)
				serializer << m_MaxAnisotropy;
			serializer << m_CompareEnable;
			if(m_CompareEnable.value)
				serializer << m_CompareOp;
			serializer << m_MinFilter << m_MaxFilter << m_NormalizedCoordinates;
		}

		static constexpr const char serialization_name[8]{"SAMPLER"};

		psl::serialization::property<bool, const_str("MIPMAPS", 7)>						m_MipMapped = true;
		psl::serialization::property<core::gfx::sampler_mipmap_mode, const_str("MIP_MODE", 8)> m_MipMapMode =
			core::gfx::sampler_mipmap_mode::nearest;
		psl::serialization::property<float, const_str("MIP_BIAS", 8)>					m_MipLodBias = 0.0f;
		psl::serialization::property<float, const_str("MIP_MIN", 7)>						m_MinLod = 0.0f;
		psl::serialization::property<float, const_str("MIP_MAX", 7)>						m_MaxLod = 0.0f;

		psl::serialization::property<core::gfx::sampler_address_mode, const_str("ADDRESS_U", 9)> m_AddressModeU =
			core::gfx::sampler_address_mode::repeat;
		psl::serialization::property<core::gfx::sampler_address_mode, const_str("ADDRESS_V", 9)> m_AddressModeV =
			core::gfx::sampler_address_mode::repeat;
		psl::serialization::property<core::gfx::sampler_address_mode, const_str("ADDRESS_W", 9)> m_AddressModeW =
			core::gfx::sampler_address_mode::repeat;
		psl::serialization::property<core::gfx::border_color, const_str("BORDER_COLOR", 12)> m_BorderColor =
			core::gfx::border_color::float_transparent_black;

		psl::serialization::property<bool, const_str("ANISOTROPY",10)>					m_AnisotropyEnable = true;
		psl::serialization::property<float, const_str("MAX_ANISO", 9)>					m_MaxAnisotropy = 2.0f;

		psl::serialization::property<bool, const_str("COMPARE", 7)>						m_CompareEnable = false;
		psl::serialization::property<core::gfx::compare_op, const_str("COMPARE_OPERATION", 17)>	m_CompareOp = core::gfx::compare_op::never;

		psl::serialization::property<core::gfx::filter, const_str("FILTER_MIN", 10)> m_MinFilter = core::gfx::filter::linear;
		psl::serialization::property<core::gfx::filter, const_str("FILTER_MAX", 10)> m_MaxFilter = core::gfx::filter::linear;

		psl::serialization::property<bool, const_str("NORMALIZED_COORDINATES",22)>		m_NormalizedCoordinates = true;
	};
}
