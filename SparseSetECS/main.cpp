#include "ECS.h"

#include <chrono>

using namespace ECS;

#define CURRENT std::chrono::steady_clock::now()
#define ELAPSED(before) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - before).count() / 1000.0f
#define LOGTIME(val) std::cout << "Elapsed: " << val / 1000.0f << "ms" << std::endl

void my_test() {
    Registry reg(1000);
    reg.RegisterComponent<int>();
    reg.RegisterComponent<float>();

    std::array<Entity, 1000> ents;

    for (int i = 0; i < 10; i++) {
        ents[i] = reg.Create();
        reg.EmplaceComponent<int>(ents[i], i);
        reg.EmplaceComponent<float>(ents[i], i + 1.0f);
    }

    auto group = reg.CreateGroup<Owned<float>, Partial<int>>();

    ents[10] = reg.Create();
    reg.EmplaceComponent<int>(ents[10], 10);

    for (auto& [ent, fv, iv] : group) {
        std::cout << ent << ": " << *fv << ", " << *iv << std::endl;
    }
}

int main()
{
    auto t1 = CURRENT;
    for (int i = 0; i < 1; i++) {
        my_test();
    }
    auto e1 = ELAPSED(t1);
    LOGTIME(e1);
}