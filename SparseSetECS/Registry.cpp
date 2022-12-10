#include "Registry.h"

namespace ECS {
	Registry::Registry(ECS_SIZE_TYPE default_capacity)
		: m_DefaultCapacity(default_capacity)
	{}

	void Registry::Resize(ECS_SIZE_TYPE new_capacity) {
		for (auto& pool : m_Pools) {
			pool.Resize(new_capacity);
		}
	}
	
	void Registry::FreeEntity(const Entity& entity) {
		for (auto& pool : m_Pools) {
			if (pool.Contains(entity)) {
				pool.FreeEntity(entity);
			}
		}
	}
	
	[[nodiscard]] Entity Registry::Create() {
		ECS_SIZE_TYPE new_identifier = Family<EntityFamily>::Type<Entity>();

		Entity new_entity = new_identifier << ECS_ENTITY_SHIFT_ALIGN;

		return new_entity;
	}
}
