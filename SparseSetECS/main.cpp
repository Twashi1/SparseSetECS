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
    }

    auto t0 = std::chrono::steady_clock::now();

    for (auto& tup : reg.CreateSingleView<MyStruct>()) {}

    auto t1 = std::chrono::steady_clock::now();

    for (auto& tup : reg.CreateView<MyStruct>()) {}

    auto t2 = std::chrono::steady_clock::now();

    double elapsed0 = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    double elapsed1 = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();

    std::cout << "Single view took: " << std::setprecision(12) << elapsed0 / 1000000.0f << "ms" << std::endl;
    std::cout << "Multi view took: " << std::setprecision(12) << elapsed1 / 1000000.0f << "ms" << std::endl;
}