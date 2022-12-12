// No idea why its called "family", but this is what the smart people call it so
#pragma once

#include "Core.h"

namespace ECS {
	class Family {
	private:
		static ECS_SIZE_TYPE m_Identifier() noexcept {
			static ECS_SIZE_TYPE value = 0;
			return value++;
		}

	public:
		template <typename>
		static ECS_SIZE_TYPE Type() noexcept {
			static const ECS_SIZE_TYPE value = m_Identifier();
			return value;
		}
	};
}
