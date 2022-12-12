#define TWASHI_LOGGER_IMPLEMENTATION
#include "ECS.h"

#include <chrono>

struct MyStruct {
    int x = 3;
};

struct S2 {
    float x = 5.0f;
};

using namespace ECS;

int main()
{
    Registry reg(1);

    reg.RegisterComponent<MyStruct>();
    reg.RegisterComponent<S2>();

    std::array<Entity, 1000> ents;

    for (int i = 0; i < 1000; i++) {
        ents[i] = reg.Create();
        reg.AddComponent(ents[i], MyStruct(i));
        reg.AddComponent(ents[i], S2(0.5f));
    }

    for (auto& tup : reg.CreateView<MyStruct, S2>()) {
        std::cout << std::get<0>(tup)->x << ", " << std::get<1>(tup)->x << std::endl;
    }
}