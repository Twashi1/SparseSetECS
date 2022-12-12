#pragma once

#include "Registry.h"

namespace ECS {
	template <typename... Ts>
	class View {
	private:
		struct __PoolSizeComparator {
			bool operator()(ComponentPool* a, ComponentPool* b) {
				return a->m_PackedArray.size < b->m_PackedArray.size;
			}
		};

		Registry* m_Registry;

		ComponentPool* m_SmallestPool = nullptr;
		ECS_SIZE_TYPE m_SmallestPoolID = std::numeric_limits<ECS_SIZE_TYPE>::max();
		ECS_SIZE_TYPE m_SmallestPoolSize = 0;

		std::tuple<Ts*...> m_GetIndex(ECS_SIZE_TYPE index) {
			if (index >= m_SmallestPoolSize) {
				// "Soft error"
				return std::tuple<Ts*...>{};
			}

			// Get entity from smallest pool
			Entity& entity = m_SmallestPool->m_PackedArray[index];

			// For each pool, index the entity to get the component (GetComponentForEntity)
			return std::make_tuple<Ts*...>(
				m_Registry->m_Pools[ComponentAllocator<Ts>::GetID()]->GetComponentForEntity<Ts>(entity)...
				);
		}

	public:
		View(Registry* registry)
			: m_Registry(registry)
		{
			// Assert pool exists first
			m_SmallestPool = std::min<ComponentPool*, __PoolSizeComparator>({ m_Registry->m_Pools[ComponentAllocator<Ts>::GetID()]... }, __PoolSizeComparator{});
			m_SmallestPoolID = m_SmallestPool->m_ID; // TODO: unused?
			m_SmallestPoolSize = m_SmallestPool->GetSize();
		}

		struct Iterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::tuple<Ts*...>;
			using pointer = value_type*;
			using reference = value_type&;

		private:
			View* m_View;
			value_type m_Current;
			ECS_SIZE_TYPE m_Index = 0;

		public:
			Iterator(View* view, const ECS_SIZE_TYPE& index)
				: m_Index(index), m_View(view)
			{
				m_Current = m_View->m_GetIndex(index);
			}

			reference operator*() { return m_Current; }
			pointer operator->() { return &m_Current;  }

			Iterator& operator++() {
				m_Index++;

				m_Current = m_View->m_GetIndex(m_Index);

				return *this;
			}
			Iterator operator++(int) {
				Iterator tmp = *this;

				m_Current = m_View->m_GetIndex(m_Index);

				++(*this);
				return tmp;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_Index == b.m_Index; }
			friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_Index != b.m_Index; }
		};

		Iterator begin()	{ return Iterator(this, 0); }
		Iterator end()		{ return Iterator(this, m_SmallestPoolSize); }

		friend Iterator;
	};

	template <typename T>
	class SingleView {
	private:
		ComponentPool* m_Pool;

	public:
		struct Iterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = T;
			using pointer = value_type*;
			using reference = value_type&;

		private:
			pointer m_Current;

		public:
			Iterator(pointer ptr) : m_Current(ptr) {}

			reference operator*() const { return *m_Current; }
			pointer operator->() { return m_Current; }

			Iterator& operator++() {
				m_Current++;

				return *this;
			}

			Iterator operator++(int) {
				Iterator tmp = *this;
				++(*this);

				return tmp;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_Current == b.m_Current; }
			friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_Current != b.m_Current; }
		};

		Iterator begin() {
			return Iterator(reinterpret_cast<T*>(&(m_Pool->m_ComponentArray.data[0])));
		}

		Iterator end() {
			return Iterator(reinterpret_cast<T*>(&(m_Pool->m_ComponentArray.data[m_Pool->m_ComponentArray.size * m_Pool->m_Allocator->SizeInBytes()])));
		}

		SingleView(ComponentPool* pool) : m_Pool(pool) {}
	};
}
