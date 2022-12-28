#pragma once

#include "Core.h"
#include "Entity.h"

namespace ECS {
	struct FullOwningGroupData {
		ECS_SIZE_TYPE start_index = 0;
		ECS_SIZE_TYPE end_index = 0;
		Signature signature;

		FullOwningGroupData() = default;

		template <typename... Ts>
		void Init() {
			([&] {
				ECS_COMP_ID_TYPE id = ComponentAllocator<Ts>::GetID();
				signature.set(id, true);
			} (), ...);
		}

		inline bool ContainsID(ECS_COMP_ID_TYPE id) {
			return signature.test(id);
		}

		inline bool ContainsSignature(const Signature& other) {
			// From: https://stackoverflow.com/questions/19258598/check-if-a-bitset-contains-all-values-of-another-bitset
			// TODO: Apparently a custom bitset might be faster?
			// Checking for if WE are a subset of THEM
			return (other & signature) == signature;
		}

		template <typename T>
		bool Contains() {
			return signature.test(ComponentAllocator<T>::GetID());
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

	// TODO: partial groups
	// TODO: syntax will be difficult, how will you differentiate between
	// TODO: fully owned elements within the group, non-owning elements
	// TODO: if this can be generalised like entt, then single view/view/"FullOwning"/"PartiallyOwning" become basically obsolete
	struct PartiallyOwnedGroupData {

	};
}
