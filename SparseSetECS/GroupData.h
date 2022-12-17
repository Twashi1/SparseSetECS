#pragma once

#include "Core.h"

namespace ECS {
	struct FullOwningGroupData {
		ECS_SIZE_TYPE start_index = 0;
		ECS_SIZE_TYPE end_index = 0;
		std::vector<ECS_SIZE_TYPE> owned_component_ids;

		FullOwningGroupData() {}

		template <typename T>
		bool Contains() {
			return std::find(
				owned_component_ids.begin(), owned_component_ids.end(), ComponentAllocator<T>::GetID()
			) != owned_component_ids.end();
		}

		template <typename... Ts>
		bool AnyOf() {
			return (Contains<Ts>() || ...);
		}

		template <typename... Ts>
		bool AllOf() {
			return (Contains<Ts>() && ...);
		}
	};
}
