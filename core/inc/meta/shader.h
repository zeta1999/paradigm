﻿#pragma once
#include "gfx/types.h"
#include "psl/array.h"
#include "psl/array_view.h"
#include "psl/library.h"
#include "psl/serialization.h"
#include "psl/ustring.h"

namespace core::meta
{
	/// \brief contains extensions for meta data when loading shader files.
	///
	/// shaders need various metadata that can describe the binding points and types of resources
	/// a SPIR-V might be expecting, as well as the pipeline stage it is assigned to.
	/// This extension to psl::meta::file contains that type of data.
	/// \todo add a way to deal with push_constants
	class shader final : public psl::meta::file
	{
		friend class psl::serialization::accessor;

	  public:
		class member
		{
			friend class psl::serialization::accessor;

		  public:
			psl::string_view name() const noexcept { return m_Name.value; }
			void name(psl::string value) noexcept { m_Name.value = value; }

			uint32_t offset() const noexcept { return m_Offset.value; }
			void offset(uint32_t value) noexcept { m_Offset.value = value; }

			uint32_t count() const noexcept { return m_Count.value; }
			void count(uint32_t value) noexcept { m_Count.value = value; }

			uint32_t stride() const noexcept { return m_Stride.value; }
			void stride(uint32_t value) noexcept { m_Stride.value = value; }

			size_t size() const noexcept { return (size_t)stride() * count(); }
			// void size(size_t value) noexcept { m_Size.value = value; }

			psl::array_view<member> members() const noexcept { return m_Members.value; }
			void members(psl::array<member> value) noexcept { m_Members.value = std::move(value); }

			/// \brief returns true if this is an unconstrained/unsized array
			inline bool is_unconstrained() const noexcept { return is_array() && m_Count.value == 0; }

			inline bool is_array() const noexcept { return m_Members.value.size() > 0; }

		  private:
			template <typename S>
			void serialize(S& s)
			{
				s << m_Name << m_Offset << m_Count << m_Stride << m_Members;
			}

			static constexpr const char serialization_name[7]{"MEMBER"};
			psl::serialization::property<psl::string, const_str("NAME", 4)> m_Name;	 // name of the element
			psl::serialization::property<uint32_t, const_str("OFFSET", 6)> m_Offset; // location of the element
			psl::serialization::property<uint32_t, const_str("COUNT", 5)> m_Count{
				1}; // how many elements, f.e. for a vec4 this is 1, but for mat4x4 this is 4
			psl::serialization::property<uint32_t, const_str("STRIDE", 6)> m_Stride; // size per element
			psl::serialization::property<psl::array<member>, const_str("MEMBERS", 7)>
				m_Members; // sub-elements (if array)
		};
		class attribute
		{
			friend class psl::serialization::accessor;

		  public:
			psl::string_view name() const noexcept { return m_Name.value; }
			void name(psl::string value) noexcept { m_Name.value = value; }

			uint32_t location() const noexcept { return m_Location.value; }
			void location(uint32_t value) noexcept { m_Location.value = value; }

			uint32_t count() const noexcept { return m_Count.value; }
			void count(uint32_t value) noexcept { m_Count.value = value; }

			uint32_t stride() const noexcept { return m_Stride.value; }
			void stride(uint32_t value) noexcept { m_Stride.value = value; }

			core::gfx::format format() const noexcept { return m_Format.value; }
			void format(core::gfx::format value) noexcept { m_Format.value = value; }

			size_t size() const noexcept { return m_Count.value * m_Stride.value; }

		  private:
			template <typename S>
			void serialize(S& s)
			{
				s << m_Name << m_Format << m_Location << m_Count << m_Stride;
			}
			static constexpr const char serialization_name[10]{"ATTRIBUTE"};
			psl::serialization::property<psl::string, const_str("NAME", 4)> m_Name;			  // name of the element
			psl::serialization::property<uint32_t, const_str("LOCATION", 8)> m_Location;	  // location of the element
			psl::serialization::property<uint32_t, const_str("COUNT", 5)> m_Count{1};		  // how many elements
			psl::serialization::property<uint32_t, const_str("STRIDE", 6)> m_Stride;		  // size per element
			psl::serialization::property<core::gfx::format, const_str("FORMAT", 6)> m_Format; // format of the element
		};

		class descriptor
		{
			friend class psl::serialization::accessor;

		  public:
			enum class dependency
			{
				in	  = 1 << 0,
				out	  = 1 << 1,
				inout = in + out
			};

			~descriptor() = default;


			psl::string_view name() const noexcept { return m_Name.value; }
			void name(psl::string value) noexcept { m_Name.value = value; }

			uint32_t binding() const noexcept { return m_Binding.value; }
			void binding(uint32_t value) noexcept { m_Binding.value = value; }

			uint32_t set() const noexcept { return m_Set.value; }
			void set(uint32_t value) noexcept { m_Set.value = value; }

			dependency qualifier() const noexcept { return m_Dependency.value; }
			void qualifier(dependency value) noexcept { m_Dependency.value = value; }

			core::gfx::binding_type type() const noexcept { return m_Type.value; }
			void type(core::gfx::binding_type value) { m_Type.value = value; }

			psl::array_view<member> members() const noexcept { return m_Members.value; }
			void members(psl::array<member> value) noexcept { m_Members.value = value; }


			size_t size() const noexcept
			{
				return std::accumulate(std::begin(m_Members.value), std::end(m_Members.value), size_t{0},
									   [](auto sum, const auto& member) { return sum + member.size(); });
			}

		  private:
			template <typename S>
			void serialize(S& s)
			{
				s << m_Name << m_Type << m_Set << m_Binding << m_Dependency << m_Members;
			}
			static constexpr const char serialization_name[11]{"DESCRIPTOR"};
			psl::serialization::property<psl::string, const_str("NAME", 4)> m_Name;
			psl::serialization::property<uint32_t, const_str("BINDING", 7)> m_Binding;
			psl::serialization::property<uint32_t, const_str("SET", 3)> m_Set;
			psl::serialization::property<core::gfx::binding_type, const_str("TYPE", 4)> m_Type;
			psl::serialization::property<dependency, const_str("DEPENDENCY", 10)> m_Dependency;
			psl::serialization::property<psl::array<member>, const_str("MEMBERS", 7)> m_Members;
		};

		shader() = default;
		shader(const psl::UID& key) : psl::meta::file(key){};

		~shader() = default;

		bool per_instance(size_t attribute_index) const noexcept;
		bool per_instance(const attribute& attribute) const noexcept;


		/// \returns the shader stage of this SPIR-V module (i.e. vertex, fragment, compute, etc..)
		core::gfx::shader_stage stage() const noexcept { return m_Stage.value; }
		/// \brief sets the stage of this SPIR-V module to the given value.
		/// \warning it is assumed this stage flag is the actual stage flag, otherwise binding the
		/// shader will fail during creation.
		/// \param[in] value the stage to expect.
		void stage(core::gfx::shader_stage value) noexcept { m_Stage.value = value; }

		psl::array_view<attribute> inputs() noexcept
		{
			return psl::array_view<attribute>(m_Attributes.data(), m_InputAttributesSize);
		}
		void inputs(psl::array<attribute> value) noexcept
		{
			m_Attributes.erase(std::begin(m_Attributes), std::next(std::begin(m_Attributes), m_InputAttributesSize));
			m_InputAttributesSize = value.size();
			m_Attributes.insert(std::begin(m_Attributes), std::move_iterator(std::begin(value)),
								std::move_iterator(std::end(value)));
		}
		psl::array_view<attribute> outputs() noexcept
		{
			return psl::array_view<attribute>(m_Attributes.data() + m_InputAttributesSize,
											  std::size(m_Attributes) - m_InputAttributesSize);
		}
		void outputs(psl::array<attribute> value) noexcept
		{
			m_Attributes.erase(std::next(std::begin(m_Attributes), m_InputAttributesSize), std::end(m_Attributes));
			m_Attributes.insert(std::end(m_Attributes), std::move_iterator(std::begin(value)),
								std::move_iterator(std::end(value)));
		}

		psl::array_view<descriptor> descriptors() const noexcept { return {m_Descriptors.value}; }
		void descriptors(psl::array<descriptor> value) noexcept { m_Descriptors.value = std::move(value); }

	  private:
		/// \brief method that will be invoked by the serialization system.
		/// \tparam S the type of the serializer/deserializer
		/// \param[in] s instance of a serializer that you can read from, or write to.
		template <typename S>
		void serialize(S& s)
		{
			psl::meta::file::serialize(s);
			psl::serialization::property<psl::array<attribute>, const_str("INPUTS", 6)> inputs;
			psl::serialization::property<psl::array<attribute>, const_str("OUTPUTS", 7)> outputs;

			if(m_Attributes.size() != 0)
			{
				inputs.value.insert(std::end(inputs.value), std::begin(m_Attributes),
									std::next(std::begin(m_Attributes), m_InputAttributesSize));
				outputs.value.insert(std::end(outputs.value),
									 std::next(std::begin(m_Attributes), m_InputAttributesSize),
									 std::end(m_Attributes));
			}

			s << m_Stage << inputs << outputs << m_Descriptors;

			std::sort(std::begin(m_Descriptors.value), std::end(m_Descriptors.value), [](const auto& lhs, const auto& rhs) { return lhs.binding() < rhs.binding();  });

			if(m_Attributes.size() == 0)
			{
				m_Attributes.reserve(inputs.value.size() + outputs.value.size());
				m_Attributes.insert(std::end(m_Attributes), std::begin(inputs.value), std::end(inputs.value));
				m_Attributes.insert(std::end(m_Attributes), std::begin(outputs.value), std::end(outputs.value));
				m_InputAttributesSize = std::size(inputs.value);
			}
		}

		psl::serialization::property<core::gfx::shader_stage, const_str("STAGE", 5)> m_Stage;
		psl::serialization::property<psl::array<descriptor>, const_str("DESCRIPTORS", 11)> m_Descriptors;
		psl::array<attribute> m_Attributes;
		size_t m_InputAttributesSize{0}; // index of end() for input attributes

		/// \brief the polymorphic serialization name for the psl::format::node that will be used to calculate the CRC64
		/// ID of this type on.
		static constexpr const char polymorphic_name[12]{"SHADER_META"};
		/// \brief returns the polymorphic ID at runtime, to resolve what type this is.
		virtual const uint64_t polymorphic_id() override { return polymorphic_identity; }
		/// \brief the associated unique ID (per type, not instance) for the polymorphic system.
		static const uint64_t polymorphic_identity;
	};
} // namespace core::meta
