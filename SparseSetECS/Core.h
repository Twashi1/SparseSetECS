#pragma once

#include <array>
#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

#include "Logger.h"

#define ECS_ID_TYPE			std::uint32_t
#define ECS_SIZE_TYPE		std::uint32_t
#define ECS_COMP_ID_TYPE	std::uint32_t // TODO: really should be uint8_t
#define ECS_SPARSE_PAGE		4096U
#define ECS_PACKED_PAGE		1024U	 // TODO: unused (packed arrays aren't paged)
#define ECS_ENTITY_MAX		0xfffffU // 5 * 4 bits (20)
#define ECS_VERSION_MAX		0xfffU   // 3 * 4 bits (12)

#define ECS_MAX_COMPONENTS	64U   // Arbitrary value for maximum amount of components (can be changed)

#define ECS_ENTITY_BIT_SIZE		20U
#define ECS_VERSION_SHIFT_ALIGN 20U // Should be same value as above
#define ECS_VERSION_BIT_SIZE	12U
#define ECS_ENTITY_BITMASK		0b00000000000011111111111111111111U
#define ECS_VERSION_BITMASK     0b11111111111100000000000000000000U
