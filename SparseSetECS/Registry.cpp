#include "Registry.h"
#include "View.h"
#include "Group.h"

namespace ECS {
	void Registry::m_MoveEntityIntoOwningGroup(const Entity& entity, const Signature& owned_pools)
	{
		GroupData* relevant_group = nullptr;

		// So iterate each affected pool
		for (ComponentPool* pool : m_Pools) {
			if (pool != nullptr) {
				if (pool->m_OwningGroup != nullptr) {
					// If this pool is in our group
					if (pool->m_OwningGroup->OwnsSignature(owned_pools)) {
						relevant_group = pool->m_OwningGroup.get();

						// Get entity we have to replace
						Entity& replacement_entity = pool->m_PackedArray.data[pool->m_OwningGroup->end_index];
						// Move this entity to the end of the group
						pool->Swap(entity, replacement_entity);
					}
				}
			}
		}
		// Increment size of group because we added an entity to it
		++(relevant_group->end_index);
	}

	void Registry::m_MoveEntityIntoOwningGroupWithUniqueValidation(const Entity& entity, const Signature& signature)
	{
		std::vector<std::shared_ptr<GroupData>> relevant_groups;

		bool moved_entity = false;

		// So iterate each affected pool
		for (ComponentPool* pool : m_Pools) {
			if (pool != nullptr) {
				if (pool->m_OwningGroup != nullptr) {
					// If this pool is in our group
					if (pool->m_OwningGroup->ContainsSignature(signature)) {
						// Ensure this pool doesn't already contain this entity
						ECS_SIZE_TYPE current_index = pool->m_SparseArray[GetIdentifier(entity)];
						// Check current index is within bounds of that pools group
						// This is not contained within the owning of this pool already
						// So move it inside
						if (current_index >= pool->m_OwningGroup->end_index || current_index < pool->m_OwningGroup->start_index) {
							if (std::find(relevant_groups.begin(), relevant_groups.end(), pool->m_OwningGroup) == relevant_groups.end()) {
								relevant_groups.push_back(pool->m_OwningGroup);
							}

							// Get entity we have to replace
							Entity& replacement_entity = pool->m_PackedArray[pool->m_OwningGroup->end_index];
							// Move this entity to the end of the group
							pool->Swap(entity, replacement_entity);

							moved_entity = true;
						}
					}
				}
			}
		}

		if (moved_entity) {
			for (std::shared_ptr<GroupData>& group : relevant_groups) {
				// Increment size of group because we added an entity to it
				++(group->end_index);
			}
		}
	}

	Registry::Registry(ECS_SIZE_TYPE default_capacity)
		: m_DefaultCapacity(default_capacity)
	{
		// Fill up component pools with nullptrs (leaving them uninitialised)
		std::fill(m_Pools.begin(), m_Pools.end(), nullptr);
	}

	Registry::~Registry()
	{
		for (ComponentPool* pool : m_Pools) {
			delete pool;
		}
	}

	void Registry::Resize(ECS_SIZE_TYPE new_capacity) {
		// Iterate each component pool
		for (ComponentPool* pool : m_Pools) {
			// Ensure component pool is valid, and if so, resize
			if (pool != nullptr) {
				pool->Resize(new_capacity);
			}
		}
	}
	
	void Registry::FreeEntity(const Entity& entity) {
		// Free components assosciated with that entity
		for (ComponentPool* pool : m_Pools) {
			// If pool is allocated
			if (pool != nullptr) {
				// If pool contains us
				if (pool->Contains(entity)) {
					// Free ourselves from the pool
					pool->FreeEntity(entity);
					// Update our signature
					m_Signatures[GetIdentifier(entity)].set(pool->m_ID, false);
				}
			}
		}

		// Now setup entity to be recycled
		// Increment available entities
		++m_AvailableEntities;
		// Get a reference to the now destroyed entity in our entities in use vector
		Entity& destroyed_entity = m_EntitiesInUse[GetIdentifier(entity)];
		// Increase version of this destroyed entity
		AddValueToVersion(destroyed_entity, 1);
		// Now swap that with whatever is our next entity right now
		std::swap(destroyed_entity, m_NextEntity);
		// Now next entity points to where our destroyed entity was, which points to what next was pointing towards
	}
	
	[[nodiscard]] Entity Registry::Create() {
		// If we have an entity available for recycling
		if (m_AvailableEntities > 0) {
			// Get what the next entity in our list is pointing to
			Entity& next_next_entity = m_EntitiesInUse[GetIdentifier(m_NextEntity)];
			// Swap the next entity (the one about to be recycled) and what it points towards
			std::swap(m_NextEntity, next_next_entity);
			// Decrement available entities
			--m_AvailableEntities;
			// Return our next entity (which is whatever is stored at next_next_entity after the swap)
			return next_next_entity;
		}
		// Just return a new entity
		else {
			if (m_NextLargestEntity > ECS_ENTITY_MAX) {
				LogError("Ran out of entities, attempt to free entities so they can be recycled");

				return ECS_ENTITY_MAX;
			}

			// Push into entities in use
			m_EntitiesInUse.push_back(m_NextLargestEntity);
			// Return entity
			return m_NextLargestEntity++;
		}
	}
}
