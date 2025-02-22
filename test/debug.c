#include "../src/vgc.h"

#include "../src/vgc.c"



int main(int argc, char **argv) {
    vgcx_start();

    do_lots_of_things();

    vgcx_stop();
}
