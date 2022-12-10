#define TWASHI_LOGGER_IMPLEMENTATION
#include "Registry.h"

struct MyStruct {
    int x = 3;
};

using namespace ECS;

int main()
{
    ECS::Registry reg;

    reg.RegisterComponent<MyStruct>();

    ECS::Entity my_ent = reg.Create();

    reg.AddComponent<MyStruct>(my_ent, MyStruct(5));

    MyStruct* s = reg.GetComponent<MyStruct>(my_ent);

    std::cout << s->x << std::endl;
}