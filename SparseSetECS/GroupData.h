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
	concept IsOwnedTag = IsValidOwnershipTag<T> && T::owned_tag::value;
	template <typename T>
	concept IsPartialTag = IsValidOwnershipTag<T> && T::partial_tag::value;

	struct GroupData {
	public:
		ECS_SIZE_TYPE start_index = 0;
		ECS_SIZE_TYPE end_index = 0;
		Signature owned_components;
		Signature partial_components;
		Signature affected_components;

		GroupData() = default;

		template <IsValidOwnershipTag... WrappedTypes>
		void Init() {
			// Setup our signatures
			([&] {
				ECS_COMP_ID_TYPE id = ComponentAllocator<typename WrappedTypes::type>::GetID();
				affected_components.set(id, true);
				
				// Owned types
				if constexpr (IsOwnedTag<WrappedTypes>) {
					owned_components.set(id, true);
				}
				// Partially owned types
				else {
					partial_components.set(id, true);
				}
			} (), ...);
		}

		inline bool OwnsID(ECS_COMP_ID_TYPE id) {
			return owned_components.test(id);
		}

		inline bool ContainsID(ECS_COMP_ID_TYPE id) {
			return affected_components.test(id);
		}

		inline bool OwnsSignature(const Signature& other) {
			// From: https://stackoverflow.com/questions/19258598/check-if-a-bitset-contains-all-values-of-another-bitset
			// TODO: Apparently a custom bitset might be faster?
			// Checking for if WE are a subset of THEM
			return (other & owned_components) == owned_components;
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
}
