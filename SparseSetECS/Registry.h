#pragma once

#include "ComponentPool.h"
#include "GroupData.h"

namespace ECS {
	template <typename... Ts>
	class View;
	template <typename T>
	class SingleView;
	template <IsValidOwnershipTag... WrappedTypes>
	class Group;
	struct GroupData;

	class Registry {
	private:
		// Use entity identifier as index into this to get its signature
		PagedArray<Signature, ECS_SPARSE_PAGE, ECS_ENTITY_MAX> m_Signatures;
		std::array<ComponentPool*, ECS_MAX_COMPONENTS> m_Pools;
		ECS_SIZE_TYPE m_DefaultCapacity = 0; // Default capacity for new component pools

		Entity m_NextEntity = ECS_ENTITY_MAX; // Next entity to be recycled
		Entity m_NextLargestEntity = 0; // The largest value entity we have right now
		ECS_SIZE_TYPE m_AvailableEntities = 0; // Amount of available entities for recycling
		// In this array, a given entity's identifier also represents its position within
		std::vector<Entity> m_EntitiesInUse; // All entities currently in use (alive/dead)

		// TODO: should just be using some lambda fold expression
		struct __PoolSizeComparator {
			bool operator()(ComponentPool* a, ComponentPool* b) {
				return a->m_PackedArray.size < b->m_PackedArray.size;
			}
		};

		void m_MoveEntityIntoOwningGroup(const Entity& entity, const Signature& signature);
		// This doesn't have validation to ensure an entity isn't moved into the same group twice
		void m_MoveEntityIntoOwningGroupWithUniqueValidation(const Entity& entity, const Signature& signature);

	public:
		Registry(ECS_SIZE_TYPE default_capacity = 1000);
		~Registry();

		// Resize entire registry (all active component pools)
		void Resize(ECS_SIZE_TYPE new_capacity);
		
		// Resize specific component pool 
		template <typename T> void ResizePool(ECS_SIZE_TYPE new_capacity) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();
			ComponentPool* pool = m_Pools[comp_id];

			// Ensure pool is registered
			if (pool == nullptr) {
				LogError("Component pool not registered; register pool before attempting resize");

				return;
			}

			// Call resize
			pool->Resize(new_capacity);
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
			ComponentPool* pool = m_Pools[comp_id];

			if (pool == nullptr) { RegisterComponent<T>(); }

			// Emplace this component at the end of the group
			pool->Emplace<T>(entity, std::forward<Args...>(args)...);

			// Update signature for this entity
			Signature& signature = m_Signatures[GetIdentifier(entity)];
			signature.set(comp_id, true);

			// TODO: could speed up by inserting entity into correct location,
			//		 and moving whatever is at that location to the end of the group
			
			// We have to update all groups actually
			m_MoveEntityIntoOwningGroupWithUniqueValidation(entity, signature);
		}

		// Add a new component to an entity
		template <typename T> void AddComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) { RegisterComponent<T>(); }

			// Get pool
			ComponentPool*& pool = m_Pools[comp_id];

			// Push component into pool
			pool->Push<T>(entity, std::forward<T>(comp));

			// Update signature for this entity
			Signature& signature = m_Signatures[GetIdentifier(entity)];
			signature.set(comp_id, true);

			// TODO: could speed up by inserting entity into correct location,
			//		 and moving whatever is at that location to the end of the group
			// If pool has an owning group
			if (pool->m_OwningGroup != nullptr) {
				// If this entity is a part of this group
				if (pool->m_OwningGroup->OwnsSignature(signature)) {
					m_MoveEntityIntoOwningGroupWithUniqueValidation(entity, signature);
				}
			}
		}

		// Update the value of an already existing component
		template <typename T> void ReplaceComponent(const Entity& entity, T&& comp) {
			ECS_SIZE_TYPE comp_id = ComponentAllocator<T>::GetID();

			// Pool not registered
			if (m_Pools[comp_id] == nullptr) { RegisterComponent<T>(); }

			// Get pool
			ComponentPool*& pool = m_Pools[comp_id];

			// Get signature of entity
			Signature& signature = m_Signatures[GetIdentifier(entity)];

			// Component exists
			if (signature.test(comp_id)) {
				pool->Replace<T>(entity, std::forward<T>(comp));
			}
			else {
				LogError("Attempted to replace component {} for entity {}, but entity didn't have component", typeid(T).name(), entity);
			}
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
			Signature& signature = m_Signatures[GetIdentifier(entity)];
			signature.set(comp_id, false);

			GroupData* relevant_group = nullptr;

			// Check if any pool owns this entity
			for (ComponentPool* pool : m_Pools) {
				if (pool != nullptr) {
					// This is our component pool, so we need to just delete it
					if (pool->m_ID == comp_id) {
						pool->FreeEntity(entity);
					}
					else if (pool->m_OwningGroup != nullptr) {
						if (pool->m_OwningGroup->OwnsSignature(signature)) {
							relevant_group = pool->m_OwningGroup;

							// This pool owns us, moves ourselves out of the group
							// We are going to swap the last entity in that group with ourselves
							Entity& last_entity = pool->m_PackedArray[pool->m_OwningGroup->end_index];
							// Perform the swap
							pool->Swap(entity, last_entity);
						}
					}
				}
			}

			if (relevant_group != nullptr) {
				--(relevant_group->end_index);
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
		template <IsValidOwnershipTag... Ts>
		friend class Group;

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

		template <IsValidOwnershipTag... WrappedTypes>
		Group<WrappedTypes...> CreateGroup() {
			std::shared_ptr<GroupData> new_group = std::make_shared<GroupData>();
			new_group->Init<WrappedTypes...>();

			// Get smallest owning component pool
			ComponentPool* smallest_pool = nullptr;
			ECS_SIZE_TYPE smallest_size = std::numeric_limits<ECS_SIZE_TYPE>::max();
			// If we have at least one owned group
			bool owned_group = false;

			// For fully-owned or partially-owned groups
			([&] {
				if constexpr (IsOwnedTag<WrappedTypes>) {
					owned_group = true;

					ECS_COMP_ID_TYPE id = ComponentAllocator<typename WrappedTypes::type>::GetID();
					ComponentPool* pool = m_Pools[id];
					ECS_SIZE_TYPE size = 0;

					if (pool != nullptr) {
						size = pool->GetSize();

						if (size < smallest_size) {
							smallest_pool = pool;
							smallest_size = size;
						}
					}

					if (pool->HasExistingGroup()) {
						LogFatal("Couldn't construct group, since one affected pool already owned by group");
					}
					else {
						pool->m_OwningGroup = new_group;
					}
				}
			} (), ...);

			// For completely non-owning groups
			if (!owned_group) {
				([&] {
					ECS_COMP_ID_TYPE id = ComponentAllocator<typename WrappedTypes::type>::GetID();
					ComponentPool* pool = m_Pools[id];
					ECS_SIZE_TYPE size = 0;

					if (pool != nullptr) {
						size = pool->GetSize();

						if (size < smallest_size) {
							smallest_pool = pool;
							smallest_size = size;
						}
					}
				} (), ...);

				new_group->end_index = smallest_size;
			}

			// Iterate the smallest pool, and move all relevant entities into the group
			for (ECS_SIZE_TYPE pool_index = 0; pool_index < smallest_size; pool_index++) {
				// Get entity at this index
				Entity& entity = smallest_pool->m_PackedArray[pool_index];

				// Get signature of entity
				Signature& signature = m_Signatures[GetIdentifier(entity)];

				// If this entity matches all our owned types
				if (new_group->OwnsSignature(signature)) {
					// Move this entity into the group
					m_MoveEntityIntoOwningGroupWithUniqueValidation(entity, signature);
				}
			}

			return Group<WrappedTypes...>(this, new_group, smallest_pool);
		}

		template <IsValidOwnershipTag... WrappedTypes>
		void DeleteGroup(Group<WrappedTypes...>& group) {
			for (ComponentPool* pool : m_Pools) {
				// If this pool is allocated
				if (pool != nullptr) {
					// If this pool is currently owned by this group
					if (pool->m_OwningGroup == group.m_GroupData) {
						// So set it to being unowned again
						pool->m_OwningGroup = nullptr;
					}
				}
			}

			group.m_GroupData = nullptr;
		}
	};
}
