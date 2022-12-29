#pragma once

#include "Registry.h"

namespace ECS {
	template <IsValidOwnershipTag... WrappedTypes>
	class Group {
	private:
		std::shared_ptr<GroupData> m_GroupData; // Registry stores and may update this data for us
		Registry* m_Registry;
		ComponentPool* m_IteratingPool = nullptr; // The pool we iterate, will be any owned component pool
		bool m_OwnsField = false;

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

		tuple_type m_GetIndex(ECS_SIZE_TYPE& index) {
			bool valid_entity = false;
			Entity* entity = nullptr;

			do {
				// Kind of a soft error when we try to grab end()
				if (index >= m_GroupData->end_index || index >= m_IteratingPool->GetSize()) {
					return tuple_type{};
				}

				entity = &(m_IteratingPool->m_PackedArray[index]);

				// We have to check ourselves that the entity actually has all the components
				if (!m_OwnsField) {
					// Get entity signature
					Signature& signature = m_Registry->m_Signatures[GetIdentifier(*entity)];
					// If this entity didn't contain all the signatures
					if (!m_GroupData->ContainsSignature(signature)) {
						// Increment index again
						++index;
					}
					// It did contain all the signatures
					else {
						valid_entity = true;
					}
				}
				// We are already assured this entity contains all the components
				else {
					valid_entity = true;
				}
			} while (!valid_entity);

			return std::make_tuple<Entity, typename WrappedTypes::type*...>(
				std::forward<Entity>(*entity), m_Grab<WrappedTypes>(index, *entity)...
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
				m_Current = m_Group->m_GetIndex(m_Index);
			}

			reference operator*() { return m_Current; }
			pointer operator->() { return &m_Current; }

			Iterator& operator++() {
				++m_Index;

				m_Current = m_Group->m_GetIndex(m_Index);

				return *this;
			}
			Iterator operator++(int) {
				Iterator tmp = *this;

				++(*this);
				return tmp;
			}

			friend bool operator==(const Iterator& a, const Iterator& b) { return a.m_Index == b.m_Index; }
			friend bool operator!=(const Iterator& a, const Iterator& b) { return a.m_Index != b.m_Index; }
		};

		Group(Registry* registry, std::shared_ptr<GroupData> data, ComponentPool* iterating_pool)
			: m_GroupData(data), m_Registry(registry), m_IteratingPool(iterating_pool)
		{}

		~Group() {
			if (m_GroupData != nullptr) {
				m_Registry->DeleteGroup(*this);
			}
		}

		Iterator begin() { return Iterator(this, m_GroupData->start_index); }
		Iterator end()
		{
			if (m_OwnsField) {
				return Iterator(this, m_GroupData->end_index);
			}
			else {
				ECS_SIZE_TYPE max_index = 0;

				// Reverse iterating pool
				for (ECS_SIZE_TYPE index = m_IteratingPool->GetSize() - 1; index >= 0; index--) {
					// Get entity at index
					Entity& entity = m_IteratingPool->m_PackedArray[index];
					Signature& signature = m_Registry->m_Signatures[GetIdentifier(entity)];

					if (m_GroupData->ContainsSignature(signature)) { max_index = index; break; }
				}

				return Iterator(this, max_index + 1);
			}
		}

		inline ECS_SIZE_TYPE size() { return m_GroupData->end_index - m_GroupData->start_index; }
		inline bool empty()			{ return size() == 0; }

		friend Registry;
		friend Iterator;
	};
}
