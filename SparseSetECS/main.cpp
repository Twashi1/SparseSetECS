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

    reg.RegisterComponent<int>();
    Entity e = reg.Create();

    reg.EmplaceComponent<int>(e, 9);
    reg.ApplyToComponent<int>(e, [](auto& val_ptr) { *val_ptr = 5; std::cout << "I got called" << std::endl; });

    int* val = reg.GetComponent<int>(e);

    std::cout << "Value is " << *val << std::endl;
}