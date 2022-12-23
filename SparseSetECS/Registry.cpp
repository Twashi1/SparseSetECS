#include "Registry.h"
#include "View.h"
#include "Group.h"

namespace ECS {
	bool Registry::m_DoesEntityBelongToFullOwningGroup(const Entity& entity) {
		// If we have a group currently used
		if (m_CurrentGroup != nullptr) {
			// Check if this entity already has the other types of this group
			// Iterate the types in this group
			for (ECS_SIZE_TYPE owned_component : m_CurrentGroup->owned_component_ids) {
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
					return false;
				}
			}
		}
		else {
			return false;
		}

		return true;
	}

	void Registry::m_MoveEntityIntoFullOwningGroup(const Entity& entity)
	{
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

	void Registry::m_EntityCheckAgainstFullOwningGroup(const Entity& entity)
	{
		if (m_DoesEntityBelongToFullOwningGroup(entity))
			m_MoveEntityIntoFullOwningGroup(entity);
	}

	Registry::Registry(ECS_SIZE_TYPE default_capacity)
		: m_DefaultCapacity(default_capacity)
	{
		// Fill up component pools with nullptrs (leaving them uninitialised)
		std::fill(m_Pools.begin(), m_Pools.end(), nullptr);
	}

	Registry::~Registry()
	{
		if (m_CurrentGroup != nullptr) delete m_CurrentGroup;

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
		for (ComponentPool*& pool : m_Pools) {
			if (pool != nullptr) {
				if (pool->Contains(entity)) {
					pool->FreeEntity(entity);
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
			if (m_CurrentLargestEntity > ECS_ENTITY_MAX) {
				LogError("Ran out of entities, attempt to free entities so they can be recycled");

				return ECS_ENTITY_MAX;
			}

			// Push into entities in use
			m_EntitiesInUse.push_back(m_CurrentLargestEntity);
			// Return entity
			return m_CurrentLargestEntity++;
		}
	}
}
