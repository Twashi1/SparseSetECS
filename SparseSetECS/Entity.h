#pragma once

#include "Core.h"

namespace ECS {
	typedef ECS_ID_TYPE Entity;
	typedef ECS_ID_TYPE Identifier_t;
	typedef ECS_ID_TYPE Version_t;

	inline Identifier_t GetVersion(const Entity& entity) {
		return entity & ECS_VERSION_BITMASK >> ECS_VERSION_SHIFT_ALIGN;
	}

	inline void AddValueToVersion(Entity& entity, const ECS_SIZE_TYPE& value) {
		entity = entity + (value << ECS_VERSION_SHIFT_ALIGN);
	}

	inline Version_t GetIdentifier(const Entity& entity) {
		return entity & ECS_ENTITY_BITMASK;
	}

	static constexpr Entity entity_max_value	= std::numeric_limits<Entity>::max();
	static constexpr Entity null_entity			= entity_max_value & ECS_ENTITY_BITMASK;
	static constexpr Entity tomb_entity			= entity_max_value & ECS_VERSION_BITMASK;
	static constexpr Entity dead_entity			= entity_max_value; // Completely dead entity
}
