#pragma once

#include "Core.h"
#include "Entity.h"

namespace ECS {
	template <typename T>
	struct Owned { using type = T; using owned_tag = std::true_type; using partial_tag = std::false_type; };
	template <typename T>
	struct Partial { using type = T; using owned_tag = std::false_type; using partial_tag = std::true_type; };

	template <typename T>
	concept IsValidOwnershipTag = requires (T) {
		T::type;
		T::owned_tag;
		T::partial_tag;
	};

	template <typename T>
	concept IsOwnedTag = IsValidOwnershipTag<T> && typename T::owned_tag();
	template <typename T>
	concept IsPartialTag = IsValidOwnershipTag<T> && typename T::partial_tag();

	struct GroupData {
	public:
		ECS_SIZE_TYPE start_index = 0;
		ECS_SIZE_TYPE end_index = 0;
		Signature owned_components;
		Signature partial_components;
		Signature affected_components;

		GroupData() = default;

		template <typename... WrappedTypes> requires IsValidOwnershipTag<WrappedTypes...>
		void Init() {
			bool encountered_owned = false;

			// Setup our signatures
			([&] {
				ECS_COMP_ID_TYPE id = ComponentAllocator<typename WrappedTypes::type>::GetID();
				affected_components.set(id, true);
				
				// Owned types
				if constexpr (IsOwnedTag<WrappedTypes>) {
					owned_components.set(id, true);
					encountered_owned = true;
				}
				// Partially owned types
				else {
					partial_components.set(id, true);
				}
			} (), ...);

			if (!encountered_owned) {
				LogFatal("Formed bad group! Must have at least one owned component within group");
			}
		}

		inline bool ContainsID(ECS_COMP_ID_TYPE id) {
			return affected_components.test(id);
		}

		inline bool ContainsSignature(const Signature& other) {
			// From: https://stackoverflow.com/questions/19258598/check-if-a-bitset-contains-all-values-of-another-bitset
			// TODO: Apparently a custom bitset might be faster?
			// Checking for if WE are a subset of THEM
			return (other & affected_components) == affected_components;
		}

		template <typename T>
		bool Contains() {
			return affected_components.test(ComponentAllocator<T>::GetID());
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
