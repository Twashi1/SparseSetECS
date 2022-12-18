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

    reg.RegisterComponent<std::string>();

    std::array<Entity, 100> ents;

    for (int i = 0; i < 10; i++) {
        ents[i] = reg.Create();

        reg.AddComponent<std::string>(ents[i], "Hello world");
    }

    reg.FreeEntity(ents[3]);
    ents[3] = reg.Create();

    reg.AddComponent<std::string>(ents[3], "Hello new world");

    for (auto& string : reg.CreateSingleView<std::string>()) {
        std::cout << string << std::endl;
    }

    for (auto& [entity, string] : reg.CreateView<std::string>()) {
        std::cout << "Entity " << GetIdentifier(entity) << " has string " << *string << std::endl;
    }
}