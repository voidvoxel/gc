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
    var(Entity, x);
    // or:  vgcx_var(Entity, x);

    x->name = new(String);
    // or:  x->name = vgcx_new(String);
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
    vgcx_begin();

    do_lots_of_things();

    vgcx_end();
}
