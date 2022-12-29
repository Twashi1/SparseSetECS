#include "ECS.h"

#include <chrono>

using namespace ECS;

#define CURRENT std::chrono::steady_clock::now()
#define ELAPSED(before) std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - before).count() / 1000.0f
#define LOGTIME(val) std::cout << "Elapsed: " << val / 1000.0f << "ms" << std::endl

struct Position {
    int x, y;

    Position(int _0, int _1) : x(_0), y(_1) {}
};

struct Name {
    std::string my_string;

    Name(Name&& other) noexcept : my_string(other.my_string) {}
};

struct Physics {
    float mass;
    float restitution;
    bool  is_rigid;

    Physics(float _0, float _1, bool _2)
        : mass(_0), restitution(_1), is_rigid(_2) {}
};

int main()
{
    auto t1 = CURRENT;
    
    Registry reg;
    reg.RegisterComponent<Position>();
    reg.RegisterComponent<Name>();
    reg.RegisterComponent<Physics>();

    auto physics_group = reg.CreateGroup<typename Partial<Position>, typename Owned<Physics>>();

    std::vector<Entity> phys_comps;
    for (int i = 0; i < 100; i++) {
        int z = i;
        Entity e = reg.Create();
        phys_comps.push_back(e);
        reg.EmplaceComponent<Position>(e, z, z);
        reg.EmplaceComponent<Physics>(e, 5.0f * z, (float)z, true);
    }

    for (auto& [entity, position, physics] : physics_group) {
        LogTrace("Entity: {}, is at {}, {}, with mass {}", entity, position->x, position->y, physics->mass);
    }

    auto e1 = ELAPSED(t1);
    LOGTIME(e1);
}