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
		PagedArray<Signature, ECS_SPARSE_PAGE, ECS_ENTITY_MAX> m_Signatures;
		std::array<ComponentPool*, ECS_MAX_COMPONENTS> m_Pools;
		ECS_SIZE_TYPE m_DefaultCapacity = 0;

		Entity m_NextEntity = ECS_ENTITY_MAX; // Next entity to be recycled
		// TODO: rename, not current but next
		Entity m_CurrentLargestEntity = 0; // The largest value entity we have right now
		ECS_SIZE_TYPE m_AvailableEntities = 0; // Amount of available entities for recycling
		// In this array, a given entity's identifier also represents its position within
		std::vector<Entity> m_EntitiesInUse; // All entities currently in use (alive/dead)

		// Data about the current full owning group
		FullOwningGroupData* m_CurrentGroup = nullptr;

		struct __PoolSizeComparator {
			bool operator()(ComponentPool* a, ComponentPool* b) {
				return a->m_PackedArray.size < b->m_PackedArray.size;
			}
		};

		// TODO: rename to "full-owned group"? or at least a consitent name between the two
		bool m_DoesEntityBelongToFullOwningGroup(const Entity& entity);
		void m_MoveEntityIntoFullOwningGroup(const Entity& entity);
		void m_EntityCheckAgainstFullOwningGroup(const Entity& entity);

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

				return;
			}

			// Otherwise add component pool to pools		
			m_Pools[id] = new ComponentPool(dynamic_cast<ComponentAllocatorBase*>(new ComponentAllocator<T>{}));
			m_Pools[id]->Resize(m_DefaultCapacity);
		}

		template <typename T, typename Func> void ApplyToComponent(const Entity& entity, Func&& func) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			if (m_Pools[comp_id] == nullptr) {
				LogError("Component pool not registered; must exist if trying to apply function");

				return;
			}

			ComponentPool*& pool = m_Pools[comp_id];
			
			if (!pool->Contains(entity)) {
				LogError("Pool didn't contain entity, couldn't patch!");

				return;
			}

			T* component = pool->GetComponentForEntity<T>(entity);
			// Call function on component
			func(component);
		}

		template <typename T, typename... Args> void EmplaceComponent(const Entity& entity, Args&&... args) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			// Emplace this component at the end of the group
			m_Pools[comp_id]->Emplace<T>(entity, std::forward<Args...>(args)...);

			// Update signature for this entity
			m_Signatures[GetIdentifier(entity)].set(comp_id, true);

			// TODO: could speed up by inserting entity into correct location,
			//		 and moving whatever is at that location to the end of the group
			m_EntityCheckAgainstFullOwningGroup(entity);
		}

		// Add a new component to an entity
		template <typename T> void AddComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) {
				LogWarn("Pool was not registered before use, registering for you");
				RegisterComponent<T>();
			}

			// Get pool
			ComponentPool*& pool = m_Pools[comp_id];

			// Push component into pool
			pool->Push<T>(entity, std::forward<T>(comp));

			// Update signature for this entity
			m_Signatures[GetIdentifier(entity)].set(comp_id, true);

			// TODO: could speed up by inserting entity into correct location,
			//		 and moving whatever is at that location to the end of the group
			// We should move this entity into our group
			m_EntityCheckAgainstFullOwningGroup(entity);
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

			// TODO: verify component already exists
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

			// Update signature for this entity
			m_Signatures[GetIdentifier(entity)].set(comp_id, false);

			// The current group does contain this component, so maybe this entity belonged to the group?
			if (m_CurrentGroup->ContainsID(comp_id)) {
				// Entity belongs to the group right now, so we have to remove it
				// since we no longer match the signature of the group
				if (m_DoesEntityBelongToFullOwningGroup(entity)) {
					for (ComponentPool* pool : m_Pools) {
						if (pool != nullptr) {
							// This is our component, so we need to just delete it
							if (pool->m_ID == comp_id) {
								pool->FreeEntity(entity);
							}
							// This is one of the affected groups
							if (m_CurrentGroup->ContainsID(pool->m_ID)) {
								// We are going to swap the last entity in that group with ourselves
								Entity& last_entity = pool->m_PackedArray.data[m_CurrentGroup->end_index];
								// Perform the swap
								pool->Swap(entity, last_entity);
								
							}
						}
					}
					// Decrement the size of the group now since we've gotten rid of this entity
					--(m_CurrentGroup->end_index);
				}
			}
		}

		// Get a pointer to a component for an entity
		template <typename T> T* GetComponent(const Entity& entity) {
			// TODO: assert pool not nullptr
			// TODO: assert entity owns this component
			ComponentPool*& pool = m_Pools[ComponentAllocator<T>::GetID()];

			return pool->GetComponentForEntity<T>(entity);
		}

		template <typename T>
		bool HasComponent(const Entity& entity) {
			// TODO: ensure this doesn't allocate a new array
			return m_Signatures[GetIdentifier(entity)].test(ComponentAllocator<T>::GetID());
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
