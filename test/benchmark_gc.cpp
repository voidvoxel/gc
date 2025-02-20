#include <any>
#include <cstdlib>
#include <iostream>

#include "../src/voidvoxel/garbage_collection/gc.h"
#include "../src/voidvoxel/garbage_collection/gc.hpp"

#include "../src/voidvoxel/garbage_collection/gc.cpp"


class Foo : public voidvoxel::garbage_collection::GarbageCollectable {
public:
    Foo(int x) : value(x) {
        // std::cout << "Foo constructor called with value " << value << '\n';
    }
    void __del__() {
        // std::cout << "Foo destructor called\n";
    }
    void show() {
        // std::cout << "Foo Value: " << value << '\n';
    }
private:
    int value;
};


template <typename T>
void vanilla_test()
{
    // Allocate raw memory for a `T` instance.
    void *memory = std::malloc(sizeof(T));

    if (!memory) {
        std::cerr << "Memory allocation failed!\n";

        exit(1);
    }

    // Construct the object using placement new.
    T* instance = new (memory) T(42);

    // Use the object.
    instance->show();

    // Manually call the destructor.
    instance->~T();

    // Free the allocated memory.
    std::free(memory);

    std::cout << std::endl;
}


template <typename T, typename... Args>
void vgc_test(voidvoxel::garbage_collection::GarbageCollector *gc, Args ...args)
{
    // Construct the object using placement new.
    T *instance = gc->make_managed<T>(args...);

    // Use the object.
    instance->show();
}


int main(int argc, char const *argv[])
{
    voidvoxel::garbage_collection::GarbageCollector gc(&argc);

    for (int i = 0; i < 1000000; i++)
    {
#if defined(CONTROL_TEST)
        vanilla_test<Foo>();
#else
        vgc_test<Foo>(&gc, 420);
#endif
    }

    gc.collect();

    return 0;
}
