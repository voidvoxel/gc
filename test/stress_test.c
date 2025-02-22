#include "../src/vgc.h"

#include "../src/vgc.c"

struct Vector3 {
    float x;
    float y;
    float z;
};
typedef struct Vector3 Vector3;

struct String {
    size_t length;
    char *data;
};
typedef struct String String;

struct Entity {
    String *name;
    Vector3 position;
};
typedef struct Entity Entity;

void do_something()
{
    vgcx_var(Entity, x);
    // or:  var(Entity, x); // C only (C++ not supported)

    x->name = vgcx_new(String);
    // or:  x->name = new(String); // C only (C++ not supported)

    vgc_Array *some_data = vgcx_create_array(size_t, 1024 * 1024 * 100);

    ((int *) some_data)[0] = 10;
    ((int *) some_data)[1] = 42;

    printf("%i\n", ((int *) some_data)[0]);
    printf("%i\n", ((int *) some_data)[1]);

    exit(0);

    vgc_Array *input = vgcx_create_array(float, 2);
    vgc_Array *hidden = vgcx_create_array(float, 3);
    vgc_Array *output = vgcx_create_array(float, 1);
}

void do_lots_of_things()
{
    int total_iterations = 1000000;

    for (int i = 0; i < total_iterations; i++)
    {
        do_something();
    }
}

int main(int argc, char **argv) {
    vgcx_start();

    do_lots_of_things();

    vgcx_stop();
}
