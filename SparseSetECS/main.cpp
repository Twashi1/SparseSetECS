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
#define LOGTIME(val) std::cout << "Elapsed: " << val / 1000.0f << "ms" << std::endl

void my_test() {
    Registry reg(1000);
    reg.RegisterComponent<int>();

    std::array<Entity, 1000> ents;

    for (int i = 0; i < 1000; i++) {
        ents[i] = reg.Create();
        reg.EmplaceComponent<int>(ents[i], i);
    }

    for (auto& val : reg.CreateSingleView<int>()) {}
}

int main()
{
    auto t1 = CURRENT;
    for (int i = 0; i < 100; i++) {
        my_test();
    }
    auto e1 = ELAPSED(t1);
    LOGTIME(e1);
}