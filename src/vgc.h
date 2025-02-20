/*
 * gc - A simple mark and sweep garbage collector for C.
 */

#if !defined(VGC__VGC_H)
#define VGC__VGC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct AllocationMap;

typedef struct vgc_GC {
    struct AllocationMap *allocs; // allocation map
    bool paused;                  // (temporarily) switch gc on/off
    void *stack_bp;                    // bottom of stack
    size_t min_size;
} vgc_GC;

extern vgc_GC *VGC_GLOBAL_GC;

/// @brief Run the garbage collector, freeing up any unreachable memory resources that are no longer being used.
/// @return The amount of memory freed (in bytes).
size_t vgc_collect(vgc_GC *gc);

/// @brief Disable garbage collection.
void vgc_disable(vgc_GC *gc);

/// @brief Enable garbage collection.
void vgc_enable(vgc_GC *gc);

/// @brief Start the garbage collector.
/// @param gc The garbage collector to start.
/// @param stack_bp The base pointer of the stack.
void vgc_start(vgc_GC *gc, void *stack_bp);

/// @brief Start the garbage collector.
/// @param gc The garbage collector to start.
/// @param stack_bp The base pointer of the stack.
/// @param initial_size The initial size of the heap.
/// @param min_size The minimum size of the heap.
/// @param downsize_load_factor The down-size load factor.
/// @param upsize_load_factor The up-size load factor.
/// @param sweep_factor The sweep factor.
void vgc_start_ext(vgc_GC *gc, void *stack_bp, size_t initial_size, size_t min_size, double downsize_load_factor, double upsize_load_factor, double sweep_factor);

/// @brief Stop the garbage collector.
/// @param gc The garbage collector to stop.
/// @return The number of bytes freed.
size_t vgc_stop(vgc_GC *gc);

/// @brief Allocate managed memory.
/// @param gc The garbage collector to use.
/// @param size The size of the managed memory *(in bytes)* to allocate.
/// @return A pointer to the allocated managed memory.
void * vgc_malloc(vgc_GC *gc, size_t size);

/// @brief Allocate static managed memory.
/// @param gc The garbage collector to use.
/// @param size The number of bytes to allocate.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed memory.
void * vgc_malloc_static(vgc_GC *gc, size_t size, void (*dtor)(void *));

/// @brief Allocate a block of managed memory.
/// @param gc The garbage collector to use.
/// @param size The size of the block of managed memory *(in bytes)* to allocate.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed memory.
void * vgc_malloc_ext(vgc_GC *gc, size_t size, void (*dtor)(void *));

/// @brief Allocate multiple blocks of managed memory.
/// @param gc The garbage collector to use.
/// @param count The number of blocks to allocate.
/// @param size The number of bytes to allocate *(per block)*.
/// @return A pointer to the allocated blocks of managed memory.
void * vgc_calloc(vgc_GC *gc, size_t count, size_t size);

/// @brief Allocate multiple blocks of managed memory.
/// @param gc The garbage collector to use.
/// @param count The number of blocks to allocate.
/// @param size The number of bytes to allocate *(per block)*.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated blocks of managed memory.
void * vgc_calloc_ext(vgc_GC *gc, size_t count, size_t size, void (*dtor)(void *));

/// @brief Reallocate (resize) a block of managed memory.
/// @param gc The garbage collector to use.
/// @param ptr A pointer to the managed memory.
/// @param size The number of bytes to allocate.
/// @return The reallocated block of managed memory.
void * vgc_realloc(vgc_GC *gc, void *ptr, size_t size);

/// @brief Free a block of managed memory.
/// @param gc The garbage collector to use.
/// @param ptr A pointer to the managed memory.
void vgc_free(vgc_GC *gc, void *ptr);

/// @brief Make a block of managed memory become static.
/// @param gc The garbage collector to use.
/// @param ptr A pointer to the managed memory.
/// @return A pointer to the managed memory.
void *vgc_make_static(vgc_GC *gc, void *ptr);

/// @brief Returns a pointer to a null-terminated byte string, which is a duplicate of the string pointed to by `str1`.
/// @param gc
/// @param str1
/// @return
char * vgc_strdup(vgc_GC *gc, const char *str1);

// Core API macros
#define vgc_new(gc, T)              ((T *) vgc_malloc(gc, sizeof(T)))
#define vgc_var(gc, T, name)        T *name = vgc_new(gc, T)

// Auxilary API macros
#define vgcx_begin()                void *vgc__bp = malloc(sizeof(vgc_GC));\
                                    VGC_GLOBAL_GC = (vgc_GC *) vgc__bp;\
                                    vgc_start(VGC_GLOBAL_GC, &vgc__bp);\
                                    (void) 0
#define vgcx_end()                  vgc_stop(VGC_GLOBAL_GC)
#define vgcx_calloc(count, size)    vgc_calloc(VGC_GLOBAL_GC, count, size)
#define vgcx_free(ptr)              (vgc_free(VGC_GLOBAL_GC, ptr))
#define vgcx_malloc(size)           vgc_malloc(VGC_GLOBAL_GC, size)
#define vgcx_new(T)                 vgc_new(VGC_GLOBAL_GC, T)
#define vgcx_realloc(ptr, size)     vgc_realloc(VGC_GLOBAL_GC, ptr, size)
#define vgcx_var(T, name)           vgc_var(VGC_GLOBAL_GC, T, name)

// Auxilary API macros (exclusive to C)
#if !defined(__cplusplus)
#define new(T)                  vgcx_new(T)
#define var(T, name)            vgcx_var(T, name)
#endif

#endif // VGC__VGC_H
