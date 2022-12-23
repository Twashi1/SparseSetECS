#include "Family.h"

namespace ECS {
	ECS_SIZE_TYPE Family::m_Identifier() noexcept {
		static ECS_SIZE_TYPE value = 0;
		return value++;
	}
}
