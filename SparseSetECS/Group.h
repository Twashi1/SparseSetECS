#pragma once

#include "Registry.h"

namespace ECS {
	template <typename... WrappedTypes> requires IsValidOwnershipTag<WrappedTypes...>
	class Group {
	private:
		std::shared_ptr<GroupData > m_GroupData; // Registry stores and may update this data for us
		Registry* m_Registry;
		ComponentPool* m_IteratingPool = nullptr; // The pool we iterate, will be any owned component pool

		template <typename T>
		typename T::type* m_Grab(ECS_SIZE_TYPE index, Entity entity) {
			ECS_COMP_ID_TYPE id = ComponentAllocator<typename T::type>::GetID();
			ComponentPool* pool = m_Registry->m_Pools[id];

			// If its an owned component
			if constexpr (IsOwnedTag<T>) {
				return pool->m_Index<typename T::type>(index);
			}
			// If partially owned component
			else {
				return pool->GetComponentForEntity<typename T::type>(entity);
			}
		}

		using tuple_type = std::tuple<Entity, typename WrappedTypes::type*...>;

		tuple_type m_GetIndex(ECS_SIZE_TYPE index) {
			Entity& entity = m_IteratingPool->m_PackedArray[index];

			return std::make_tuple<Entity, typename WrappedTypes::type*...>(
				std::forward<Entity>(entity), m_Grab<WrappedTypes>(index, entity)...
			);
		}

	public:
		struct Iterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = tuple_type;
			using pointer = value_type*;
			using reference = value_type&;

		private:
			Group<WrappedTypes...>* m_Group;
			ECS_SIZE_TYPE m_Index = 0;
			value_type m_Current;

		public:
			Iterator(Group* group, const ECS_SIZE_TYPE& index)
				: m_Index(index), m_Group(group)
			{
				m_Current = m_Group->m_GetIndex(index);
			}

			reference operator*() { return m_Current; }
			pointer operator->() { return &m_Current; }

			Iterator& operator++() {
				m_Index++;

				m_Current = m_Group->m_GetIndex(m_Index);

				return *this;
			}
			Iterator operator++(int) {
				Iterator tmp = *this;

				m_Current = m_Group->m_GetIndex(m_Index);

				++(*this);
				return tmp;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_Index == b.m_Index; }
			friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_Index != b.m_Index; }
		};

		Group(Registry* registry, std::shared_ptr<GroupData> data, ComponentPool* iterating_pool)
			: m_GroupData(data), m_Registry(registry), m_IteratingPool(iterating_pool)
		{}

		Iterator begin()	{ return Iterator(this, m_GroupData->start_index); }
		Iterator end()		{ return Iterator(this, m_GroupData->end_index);   }

		inline ECS_SIZE_TYPE size() { return m_GroupData->end_index - m_GroupData->start_index; }
		inline bool empty()			{ return size() == 0; }

		friend Registry;
		friend Iterator;
	};
}
