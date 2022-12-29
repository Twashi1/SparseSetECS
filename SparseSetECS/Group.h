#pragma once

#include "Registry.h"

namespace ECS {
	template <typename... WrappedTypes> requires IsValidOwnershipTag<WrappedTypes...>
	class Group {
	private:
		GroupData* m_GroupData; // Registry stores and may update this data for us
		Registry* m_Registry;
		ComponentPool* m_IteratingPool; // The pool we iterate, will be any owned component pool

		template <typename T>
		T* m_Grab(ECS_SIZE_TYPE index, Entity entity) {
			ECS_COMP_ID_TYPE id = ComponentAllocator<T>::GetID();
			ComponentPool* pool = m_Registry->m_Pools[id];

			// If its an owned component
			if constexpr (IsOwnedTag<T>) {
				return pool->m_Index<T>(index);
			}
			// If partially owned component
			else {
				return pool->GetComponentForEntity<T>(entity);
			}
		}

		using tuple_type = std::tuple<Entity, typename WrappedTypes::type*...>;

		tuple_type m_GetIndex(ECS_SIZE_TYPE index) {
			Entity entity = m_IteratingPool->m_PackedArray[index];

			return std::make_tuple<Entity, typename WrappedTypes::type*...>(
				entity, m_Grab<typename WrappedTypes::type>(index, entity)...
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

		Group(Registry* registry, GroupData* data)
			: m_GroupData(data), m_Registry(registry)
		{}

		Iterator begin()	{ return Iterator(this, m_GroupData->start_index); }
		Iterator end()		{ return Iterator(this, m_GroupData->end_index);   }

		inline ECS_SIZE_TYPE size() { return m_GroupData->end_index - m_GroupData->start_index; }
		inline bool empty()			{ return size() == 0; }

		friend Registry;
		friend Iterator;
	};

	template <typename... Ts>
	class FullOwningGroup {
	private:
		FullOwningGroupData* m_GroupData; // Registry stores and may update this data for us
		Registry* m_Registry;

		std::tuple<Ts*...> m_GetIndex(ECS_SIZE_TYPE index) {
			// "Perfect SOA", but there's still so much indirection
			// Iterate each component pool and grab component at index
			return std::make_tuple<Ts*...>(m_Registry->m_Pools[
				ComponentAllocator<Ts>::GetID()
			]->m_Index<Ts>(index)...);
		}

	public:
		struct Iterator {
		public:
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = std::tuple<Ts*...>;
			using pointer = value_type*;
			using reference = value_type&;

		private:
			FullOwningGroup<Ts...>* m_Group;
			ECS_SIZE_TYPE m_Index = 0;
			value_type m_Current;

		public:
			Iterator(FullOwningGroup* group, const ECS_SIZE_TYPE& index)
				: m_Index(index), m_Group(group)
			{
				m_Current = m_Group->m_GetIndex(m_Index);
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

		Iterator begin() { return Iterator(this, m_GroupData->start_index); }
		Iterator end() { return Iterator(this, m_GroupData->end_index); }

		FullOwningGroup(Registry* registry, FullOwningGroupData* data)
			: m_Registry(registry), m_GroupData(data) {}

		friend Registry;
		friend Iterator;
	};
}
