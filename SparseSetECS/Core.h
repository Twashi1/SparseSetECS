#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>
#include <vector>

#include <immintrin.h>

#include "Logger.h"

#define ECS_ID_TYPE   std::uint32_t
#define ECS_SIZE_TYPE std::uint32_t
#define ECS_SPARSE_PAGE 4096U
#define ECS_PACKED_PAGE 1024U
#define ECS_ENTITY_MAX  0xfffffU
#define ECS_VERSION_MAX 0xfffU

#define ECS_VERSION_BITMASK    0b00000000000000000000111111111111U
#define ECS_ENTITY_BITMASK     0b11111111111111111111000000000000U
#define ECS_ENTITY_SHIFT_ALIGN 12U
