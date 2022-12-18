#include "ECS.h"

#include <chrono>

struct MyStruct {
    int x = 3;
};

struct S2 {
    float x = 5.0f;
};

using namespace ECS;

#define CURRENT std::chrono::steady_clock::now()
#define ELAPSED(before) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - before).count() / 1000.0f

int main()
{
    Registry reg(1000);

    std::array<Entity, 100> ents;

    for (int i = 0; i < 10; i++) {
        ents[i] = reg.Create();

        LogTrace("Registry created: {}", ents[i]);
    }

    reg.FreeEntity(ents[5]);
    LogTrace("Freed entity 5");

    reg.FreeEntity(ents[3]);
    LogTrace("Freed entity 3");

    reg.FreeEntity(ents[2]);
    LogTrace("Freed entity 2");

    for (int i = 0; i < 4; i++) {
        LogTrace("Generated entity {}", GetIdentifier(reg.Create()));
    }
}