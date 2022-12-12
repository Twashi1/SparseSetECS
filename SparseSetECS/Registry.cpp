#include "Registry.h"
#include "View.h"

namespace ECS {
	Registry::Registry(ECS_SIZE_TYPE default_capacity)
		: m_DefaultCapacity(default_capacity)
	{
		for (ComponentPool*& pool : m_Pools) {
			pool = nullptr;
		}
	}

	void Registry::Resize(ECS_SIZE_TYPE new_capacity) {
		for (ComponentPool*& pool : m_Pools) {
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
