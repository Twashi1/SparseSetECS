#include "Registry.h"
#include "View.h"
#include "Group.h"

namespace ECS {
	bool Registry::m_DoesEntityBelongToFullOwningGroup(const Entity& entity) {
		// Our current group isn't empty
		if (m_CurrentGroup != nullptr) {
			// Get signature of this entity
			const Signature& signature = m_Signatures[GetIdentifier(entity)];
			// Check if signature matches
			if (m_CurrentGroup->ContainsSignature(signature)) {
				return true;
			}

			// Signature must not have matched
			return false;
		}
		
		// Group didn't exist
		return false;
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
