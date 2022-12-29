#pragma once

#include "View.h"
#include "Group.h"

// TODO: needs extensive testing that GetIdentifier is being used appropriately
// TODO: multiple assumptions that the identifier is the first 20 bits
// TODO: differentiate between ECS_SIZE_TYPE and ECS_ID_TYPE
// TODO: version isn't being really used right now
// TODO: general cleanup

// TODO: some group operations should be taking the smallest component pool but are just taking the first one
// TODO: group deconstruction
// TODO: freeing groups
