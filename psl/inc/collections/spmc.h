#pragma once
#include "collections/ring_array.h"
#include "view_ptr.h"
#include <atomic>
#include <optional>
#include "math/math.hpp"

namespace psl::spmc
{
	template <typename T>
	class producer;


	/// \brief provides multithread safe access into a psl::spmc::producer
	///
	/// \details This is the object to pass to the consumer threads, they have a minimal multithread safe API
	/// into the SPMC deque. The consumer is limited to viewing the state of the producer, and popping items from the
	/// front (if any are left).
	template <typename T>
	class consumer final
	{
		friend class producer<T>;
		consumer(psl::view_ptr<producer<T>> producer) : m_Producer(producer){};

	  public:
		consumer()  = delete;
		~consumer() = default;

		consumer(const consumer& other)		= default;
		consumer(consumer&& other) noexcept = default;
		consumer& operator=(const consumer& other) = default;
		consumer& operator=(consumer&& other) noexcept = default;

		/// \brief Tries to pop an element from the front of the producer thread's deque.
		///
		/// \details If any items are left on the producer's deque, then this method will pop an item off of the front.
		/// Otherwise it will return a nullopt;
		auto pop() noexcept;

		/// \Returns the current count of all elements in the producer.
		/// \warning The result here doesn't mean there will be/won't be an item on the deque by the time you invoke
		/// pop.
		auto size() const noexcept;

		/// \Returns the current count of all elements in the producer.
		/// \warning The result here doesn't mean there will be/won't be an item on the deque by the time you invoke
		/// pop.
		auto ssize() const noexcept;

	  private:
		psl::view_ptr<producer<T>> m_Producer;
	};

	/// \brief SPMC based on Chase-Lev deque
	///
	/// \details This class is based on Chase-Lev's deque implementation of a Single Producer Multi Consumer (SPMC).
	/// The producer should only be used on a single thread, and the 'psl::spmc::consumer' variant should be used in
	/// the consuming threads. This offers a safe API.
	template <typename T>
	class producer final
	{
		friend class consumer<T>;

		/// \brief Wrapper over ring_array<T>
		///
		/// \details This wrapper class over a ring_array keeps track of its internal offset.
		/// That will be used by the producer as it schedules more and more items.
		/// The ring_array can only be resized by invoking the buffer::copy method.
		/// Internally you can keep incrementing your access indices, as long as you never exceed the range
		/// being used (range <= buffer::capacity(), where range == begin to end indices).
		/// When you exceed the capacity, you need to buffer::copy a new one.
		struct buffer
		{
		  public:
			buffer(size_t capacity) : m_Data(psl::math::next_pow_of(2, std::max<size_t>(32u, capacity))){};
			~buffer() {}
			void set(size_t index, T&& value) noexcept
			{
				m_Data[(index - m_Offset) & (m_Data.size() - 1)] = std::forward<T>(value);
			}

			auto at(size_t index) const noexcept { return m_Data[(index - m_Offset) & (m_Data.size() - 1)]; }

			/// \brief Returns a logical continuation buffer based on this buffer
			///
			/// \details Copies the current buffer into a new one of 'at least' the given capacity. It will grow to the
			/// next logical power of 2 that can also contain the begin-end items.
			buffer* copy(size_t begin, size_t end, size_t capacity = 0)
			{
				capacity = psl::math::next_pow_of(2, std::max(capacity, (end - begin) + 1));

				buffer* ptr   = new buffer(capacity);
				ptr->m_Offset = begin;
				for(size_t i = begin; i != end; ++i)
				{
					ptr->set(i, at(i));
				}
				return ptr;
			}

			/// \returns Max continuous range of items in the buffer.
			size_t capacity() const noexcept { return m_Data.size(); };

		  private:
			ring_array<T> m_Data;
			size_t m_Offset{0};
		};

	  public:
		producer(int64_t capacity = 1024)
		{
			capacity = psl::math::next_pow_of(2, std::max(capacity, 1024i64));
			m_Begin.store(0, std::memory_order_relaxed);
			m_End.store(0, std::memory_order_relaxed);
			auto cont = new buffer(capacity);
			m_Data.store(cont, std::memory_order_relaxed);
		};
		~producer()
		{
			if(m_Last != nullptr) delete(m_Last);
			delete(m_Data.load());
		}

		producer(const producer& other)		= delete;
		producer(producer&& other) noexcept = default;
		producer& operator=(const producer& other) = delete;
		producer& operator=(producer&& other) noexcept = default;

		/// \Returns a consumer that is linked to the current producer, to be used in other threads.
		::psl::spmc::consumer<T> consumer() noexcept
		{
			return ::psl::spmc::consumer<T>(psl::view_ptr<producer<T>>{this});
		};

		bool empty() const noexcept
		{
			auto begin = m_Begin.load(std::memory_order_relaxed);
			auto end   = m_End.load(std::memory_order_relaxed);
			return end <= begin;
		}

		/// \Returns the current count of all elements in the producer.
		size_t size() const noexcept { return static_cast<size_t>(ssize()); }

		/// \Returns the current count of all elements in the producer.
		int64_t ssize() const noexcept
		{
			auto begin = m_Begin.load(std::memory_order_relaxed);
			auto end   = m_End.load(std::memory_order_relaxed);
			return std::max(end - begin, 0);
		}

		/// \brief Resizes the internal buffer to the given size
		///
		/// \details Tries to resize to the given size, it will automatically align itself to the next power of 2 if
		/// the value isn't a power of 2 already. The minimum size will be 'at least' equal to, or bigger than, the
		/// current size (not capacity) of the internal buffer.
		void resize(size_t size)
		{
			auto cont = m_Data.load(std::memory_order_relaxed);
			size	  = psl::math::next_pow_of(2, size);
			if(size == (int64_t)cont->capacity()) return;

			auto begin   = m_Begin.load(std::memory_order_relaxed);
			auto end	 = m_End.load(std::memory_order_relaxed);
			auto newCont = cont->copy(begin, end);
			std::swap(newCont, cont);
			m_Data.store(cont, std::memory_order_relaxed);

			if(m_Last != nullptr) delete(m_Last);
			m_Last = newCont;
		}

		/// \brief Push the given element onto the end of the deque.
		///
		/// \details Will push the given element onto the deque if enough space is present.
		/// If not enough space is present it will construct a new internal buffer that contains at least enough space.
		/// If a previous (unused) buffer is present it will now clean that buffer up.
		/// \warning Callable only on the owning thread, do not call from multiple threads.
		/// \todo Implement the backing storage as an atomic<shared_ptr<buffer>> for more logical cleanup flow.
		void push(T&& value)
		{
			int64_t end   = m_End.load(std::memory_order_relaxed);
			int64_t begin = m_Begin.load(std::memory_order_acquire);
			auto cont	 = m_Data.load(std::memory_order_relaxed);

			if((int64_t)cont->capacity() - 1 < (end - begin))
			{
				auto newCont = cont->copy(begin, end);
				std::swap(newCont, cont);
				m_Data.store(cont, std::memory_order_relaxed);

				if(m_Last != nullptr) delete(m_Last);
				m_Last = newCont;
			}

			cont->set(end, std::forward<T>(value));
			std::atomic_thread_fence(std::memory_order_release);
			m_End.store(end + 1, std::memory_order_relaxed);
		}

		/// \brief Pops an element (if any are left) off the deque from the end.
		///
		/// \details Tries to pop an element off the end of the deque.
		/// \warning Only callable from the owning thread, otherwise the results will be undefined.
		std::optional<T> pop() noexcept
		{
			int64_t end = m_End.load(std::memory_order_relaxed) - 1;
			auto cont   = m_Data.load(std::memory_order_relaxed);
			m_End.store(end, std::memory_order_relaxed);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			int64_t begin = m_Begin.load(std::memory_order_relaxed);

			std::optional<T> res{};

			if(begin <= end)
			{
				res = cont->at(end);
				if(begin == end)
				{
					if(!m_Begin.compare_exchange_strong(begin, begin + 1, std::memory_order_seq_cst,
														std::memory_order_relaxed))
					{
						res = std::nullopt;
					}
					m_End.store(end + 1, std::memory_order_relaxed);
				}
			}
			else
			{
				m_End.store(end + 1, std::memory_order_relaxed);
			}
			return res;
		}

		void clear() noexcept 
		{ 
			auto end = m_End.load(std::memory_order_relaxed);
			m_Begin.store(end, std::memory_order_seq_cst);
		}

	  private:
		/// \brief Pops an element (if any are left) off the deque from the front.
		///
		/// \details To be used by consumer threads, this gives a thread safe way of stealing items from the deque.
		/// It can be called by any thread, but is only exposed to the consumer class.
		std::optional<T> steal() noexcept
		{
			int64_t begin = m_Begin.load(std::memory_order_acquire);
			std::atomic_thread_fence(std::memory_order_seq_cst);
			int64_t end = m_End.load(std::memory_order_acquire);

			std::optional<T> res;

			if(begin < end)
			{
				res = m_Data.load(std::memory_order_consume)->at(begin);

				if(!m_Begin.compare_exchange_strong(begin, begin + 1, std::memory_order_seq_cst,
													std::memory_order_relaxed))
				{
					return std::nullopt;
				}
			}

			return res;
		}

		std::atomic_int64_t m_Begin;
		std::atomic_int64_t m_End;
		std::atomic<buffer*> m_Data;
		buffer* m_Last{nullptr};
	};

	template <typename T>
	auto consumer<T>::pop() noexcept
	{
		return m_Producer->steal();
	}

	template <typename T>
	auto consumer<T>::size() const noexcept
	{
		return m_Producer->size();
	}
	template <typename T>
	auto consumer<T>::ssize() const noexcept
	{
		return m_Producer->ssize();
	}
} // namespace psl::spmc