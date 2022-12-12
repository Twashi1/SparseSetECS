#pragma once

#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <tuple>
#include <vector>

#include "Logger.h"

#define ECS_ID_TYPE   std::uint32_t
#define ECS_SIZE_TYPE std::uint32_t
#define ECS_SPARSE_PAGE 4096U
#define ECS_PACKED_PAGE 1024U
#define ECS_ENTITY_MAX  0xfffffU
#define ECS_VERSION_MAX 0xfffU

#define ECS_MAX_COMPONENTS 64U

#define ECS_ENTITY_BIT_SIZE		20U
#define ECS_VERSION_SHIFT_ALIGN 20U // Should be same value as above
#define ECS_VERSION_BIT_SIZE	12U
#define ECS_ENTITY_BITMASK		0b00000000000011111111111111111111U
#define ECS_VERSION_BITMASK     0b11111111111100000000000000000000U
