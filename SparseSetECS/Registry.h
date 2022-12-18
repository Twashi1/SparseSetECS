#pragma once

#include "ComponentPool.h"
#include "GroupData.h"

namespace ECS {
	template <typename... Ts>
	class View;
	template <typename T>
	class SingleView;
	template <typename... Ts>
	class FullOwningGroup;
	struct FullOwningGroupData;

	class Registry {
	private:
		std::array<ComponentPool*, ECS_MAX_COMPONENTS> m_Pools;
		ECS_SIZE_TYPE m_DefaultCapacity = 0;
		ECS_SIZE_TYPE m_NextEntity = 0;

		// Data about the current full owning group
		FullOwningGroupData* m_CurrentGroup = nullptr;

		struct __PoolSizeComparator {
			bool operator()(ComponentPool* a, ComponentPool* b) {
				return a->m_PackedArray.size < b->m_PackedArray.size;
			}
		};

	public:
		Registry(ECS_SIZE_TYPE default_capacity = 1000);
		~Registry();

		// Resize entire registry (all active component pools)
		void Resize(ECS_SIZE_TYPE new_capacity);
		
		// Resize specific component pool 
		template <typename T> void ResizePool(ECS_SIZE_TYPE new_capacity) {
			ECS_SIZE_TYPE pool_id = ComponentAllocator<T>::GetID();

			// Ensure pool is registered
			if (m_Pools[pool_id] == nullptr) {
				LogError("Component pool not registered; register pool before attempting resize");

				return;
			}

			// Call resize
			m_Pools[pool_id]->Resize(new_capacity);
		}

		// Free up an entity id and all associated components
		void FreeEntity(const Entity& entity);

		// Get a new entity to use
		[[nodiscard]] Entity Create();

		// Register a component for future use
		template <typename T> void RegisterComponent() {
			ECS_SIZE_TYPE id = ComponentAllocator<T>::GetID();

			if (m_Pools[id] != nullptr) {
				LogWarn("Component {} already registered, ignoring call", typeid(T).name());
			}

			// Otherwise add component pool to pools		
			m_Pools[id] = new ComponentPool(dynamic_cast<ComponentAllocatorBase*>(new ComponentAllocator<T>{}));
			m_Pools[id]->Resize(m_DefaultCapacity);
		}

		// Add a new component to an entity
		template <typename T> void AddComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			bool group_contains_new_comp = false;
			bool entity_belongs = true;

			// If we have a group currently used
			if (m_CurrentGroup != nullptr) {
				// Check if this entity already has the other types of this group
				// Iterate the types in this group
				for (ECS_SIZE_TYPE owned_component : m_CurrentGroup->owned_component_ids) {
					if (owned_component == comp_id) {
						group_contains_new_comp = true;
					}

					bool found_entity = false;

					// Iterate each component pool
					for (ComponentPool* pool : m_Pools) {
						if (pool != nullptr) {
							// Check if pool is one of our component ids
							if (pool->m_ID == owned_component) {
								// This pool contains us
								if (pool->Contains(entity)) {
									found_entity = true;
								}
							}
						}
					}

					// This entity didn't match the full signature for the group, so end checking
					if (!found_entity) {
						entity_belongs = false;
						break;
					}
				}
			}

			// Get pool
			ComponentPool*& pool = m_Pools[comp_id];

			// Push component into pool
			pool->Push<T>(entity, std::forward<T>(comp));

			// We should move this entity into our group
			bool should_move_entity = entity_belongs && group_contains_new_comp;
			if (should_move_entity) {
				// So iterate each affected pool
				for (ComponentPool* pool : m_Pools) {
					if (pool != nullptr) {
						// If this pool is in our group
						if (m_CurrentGroup->ContainsID(pool->m_ID)) {
							// Get entity we have to replace
							Entity& replacement_entity = pool->m_PackedArray.data[m_CurrentGroup->end_index];
							// Move this entity to the end of the group
							pool->Swap(entity, replacement_entity);
						}
					}
				}
				// Increment size of group because we added an entity to it
				++(m_CurrentGroup->end_index);
			}
		}

		// Update the value of an already existing component
		template <typename T> void ReplaceComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			// Get pool
			ComponentPool*& pool = m_Pools[comp_id];

			pool->Replace<T>(entity, std::forward<T>(comp));
		}

		// Remove component from an entity
		template <typename T> void RemoveComponent(const Entity& entity) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, can't remove component when pool doesn't exist");

				return;
			}

			ComponentPool*& pool = m_Pools[comp_id];

			pool->FreeEntity(entity);
		}

		// Get a pointer to a component for an entity
		template <typename T> T* GetComponent(const Entity& entity) {
			// TODO: assert pool not nullptr
			ComponentPool*& pool = m_Pools[ComponentAllocator<T>::GetID()];

			return pool->GetComponentForEntity<T>(entity);
		}

		template <typename T>
		bool HasComponent(const Entity& entity) {
			return m_Pools[ComponentAllocator<T>::GetID()]->Contains(entity);
		}

		template <typename... Ts>
		bool AnyOf(const Entity& entity) {
			return (HasComponent<Ts>(entity) || ...);
		}

		template <typename... Ts>
		bool AllOf(const Entity& entity) {
			return (HasComponent<Ts>(entity) && ...);
		}

		template <typename... Ts>
		std::tuple<Ts*...> GetComponents(const Entity& entity) {
			return std::make_tuple<Ts*...>(GetComponent<Ts>(entity)...);
		}

		template <typename... Ts>
		friend class View;
		template <typename T>
		friend class SingleView;
		template <typename... Ts>
		friend class FullOwningGroup;

		template <typename... Ts>
		View<Ts...> CreateView() {
			if (sizeof...(Ts) == 1) {
				LogInfo("Prefer CreateSingleView when iterating 1 component");
			}

			// Check all pools are allocated
			if (((m_Pools[ComponentAllocator<Ts>::GetID()] == nullptr) || ...)) {
				LogFatal("Can't create view: one of the components is not registered!");
			}

			return View<Ts...>(this);
		}

		template <typename T>
		SingleView<T> CreateSingleView() {
			if (m_Pools[ComponentAllocator<T>::GetID()] == nullptr) {
				LogFatal("Can't create view, object type {} is not registered!", typeid(T).name());
			}

			return SingleView<T>(m_Pools[ComponentAllocator<T>::GetID()]);
		}

		template <typename... Ts>
		FullOwningGroup<Ts...> CreateGroup() {
			// TODO: assert all Ts registered
			if (m_CurrentGroup != nullptr) {
				LogFatal("Must destroy existing group before creating new one!");

				return FullOwningGroup<Ts...>(this, m_CurrentGroup);
			}

			// Create empty group data
			m_CurrentGroup = new FullOwningGroupData{};

			// Scoping this because we really don't wanna use affected components again since we moved it
			{
				// Calculate the affected components
				std::vector<ECS_SIZE_TYPE> affected_components = { ComponentAllocator<Ts>::GetID()... };
				// Set that for the group
				m_CurrentGroup->owned_component_ids = std::move(affected_components);
			}

			// Iterate each affected entity which contains all components
			// Grab smallest pool for these arguments
			ComponentPool* smallest_pool = std::min<ComponentPool*, __PoolSizeComparator>({ m_Pools[ComponentAllocator<Ts>::GetID()]... }, __PoolSizeComparator{});
			ECS_SIZE_TYPE smallest_size = smallest_pool->GetSize();

			// Iterate entities within this smallest pool
			for (ECS_SIZE_TYPE small_pool_index = 0; small_pool_index < smallest_size; small_pool_index++) {
				// Get entity
				Entity& entity = smallest_pool->m_PackedArray.data[small_pool_index];

				// If this entity matches all our types
				if (AllOf<Ts...>(entity)) {
					// We have to move this entity into our group
					// So for each relevant pool, swap it into the correct location
					([&] {
						// Get the relevant pool
						ComponentPool* pool = m_Pools[ComponentAllocator<Ts>::GetID()];
						// Get entity at the location we need to replace
						// Also incrementing the end index of the group here since we're "adding" to it
						Entity& replacement_entity = pool->m_PackedArray.data[m_CurrentGroup->end_index];
						// Swap the entities
						pool->Swap(entity, replacement_entity);
					} (), ...);
					// Increment the end index because we finished sorting one entity
					++(m_CurrentGroup->end_index);
				}
			}

			// All our entities are "sorted" now, so just create a group of relevant size
			return FullOwningGroup<Ts...>(this, m_CurrentGroup);
		}
	};
}
