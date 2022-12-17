#include "Registry.h"
#include "View.h"
#include "Group.h"

namespace ECS {
	Registry::Registry(ECS_SIZE_TYPE default_capacity)
		: m_DefaultCapacity(default_capacity)
	{
		// Fill up component pools with nullptrs for now
		std::fill(m_Pools.begin(), m_Pools.end(), nullptr);
	}

	Registry::~Registry()
	{
		if (m_CurrentGroup != nullptr) delete m_CurrentGroup;
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
		for (ComponentPool*& pool : m_Pools) {
			if (pool != nullptr) {
				if (pool->Contains(entity)) {
					pool->FreeEntity(entity);
				}
			}
		}
	}
	
	[[nodiscard]] Entity Registry::Create() {
		ECS_SIZE_TYPE new_identifier = m_NextEntity++;

		Entity new_entity = new_identifier;

		return new_entity;
	}
}
