/*
 * gc - A simple mark and sweep garbage collector for C.
 */

#if !defined(VGC__VGC_H)
#define VGC__VGC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/// @brief A deconstructor to call after freeing managed memory.
typedef void (*vgc_Deconstructor)(void *);

/**
 * The allocation object.
 *
 * The allocation object holds all metadata for a memory location
 * in one place.
 */
typedef struct vgc_Allocation {
    void *ptr;                      // mem pointer
    size_t size;                    // allocated size in bytes
    char tag;                       // the tag for mark-and-sweep
    vgc_Deconstructor dtor;         // destructor
    struct vgc_Allocation *next;    // separate chaining
} vgc_Allocation;

/**
 * The allocation hash map.
 *
 * The core data structure is a hash map that holds the allocation
 * objects and allows O(1) retrieval given the memory location. Collision
 * resolution is implemented using separate chaining.
 */
typedef struct vgc_AllocationMap {
    size_t capacity;
    size_t min_capacity;
    double downsize_factor;
    double upsize_factor;
    double sweep_factor;
    size_t sweep_limit;
    size_t size;
    vgc_Allocation **allocs;
} vgc_AllocationMap;

/// @brief A garbage collector, used to manage memory.
typedef struct vgc_GC {
    /// @brief The allocation map.
    struct vgc_AllocationMap *allocs;

    /// @brief Toggling this variable will (temporarily) switch gc on/off.
    bool disabled;

    /// @brief A pointer to the bottom of managed stack.
    void *stack_bp;

    /// @brief The minimum size of the managed heap.
    size_t min_size;
} vgc_GC;

/// @brief A managed buffer of RAM.
typedef struct vgc_Buffer {
    /// @brief The address where the buffer's data is stored in memory.
    void * const address;

    /// @brief The length of the buffer *(in bytes)*.
    const size_t length;
} vgc_Buffer;

/// @brief A managed array of objects.
typedef struct vgc_Array {
    /// @brief The underlying buffer containing the array's objects.
    vgc_Buffer *buffer;

    /// @brief The number of slots the array has.
    const size_t slot_count;

    /// @brief The size *(in bytes)* of each slot.
    const size_t slot_size;
} vgc_Array;

/// @brief A global instance of the garbage collector for use by single-threaded applications.
extern vgc_GC *VGC_GLOBAL_GC;

/// @brief The C standard library function `free`.
void (*vgc__libc_free)(void *block) = free;

/// @brief The C standard library function `malloc`.
void * (*vgc__libc_malloc)(size_t size) = malloc;

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
void * vgc_malloc_static(vgc_GC *gc, size_t size, vgc_Deconstructor dtor);

/// @brief Allocate a block of managed memory.
/// @param gc The garbage collector to use.
/// @param size The size of the block of managed memory *(in bytes)* to allocate.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed memory.
void * vgc_malloc_ext(vgc_GC *gc, size_t size, vgc_Deconstructor dtor);

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
void * vgc_calloc_ext(vgc_GC *gc, size_t count, size_t size, vgc_Deconstructor dtor);

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
/// @param gc The garbage collector to use.
/// @param str1 The string to duplicate.
/// @return A duplicate of `str1`.
char * vgc_strdup(vgc_GC *gc, const char *str1);

/// @brief Create a managed array.
/// @param gc The garbage collector to use.
/// @param tsize The size of an item contained within the array.
/// @param count The number of items the managed array can hold.
/// @return A pointer to the allocated managed array.
vgc_Array * vgc_create_array(vgc_GC *gc, size_t tsize, size_t count);

/// @brief Create a managed array.
/// @param gc The garbage collector to use.
/// @param tsize The size of an item contained within the array.
/// @param count The number of items the managed array can hold.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed array.
vgc_Array * vgc_create_array_ext(vgc_GC *gc, size_t tsize, size_t count, vgc_Deconstructor dtor);

/// @brief Create a managed buffer.
/// @param gc The garbage collector to use.
/// @param size The size of the buffer *(in bytes)* to allocate.
/// @return A pointer to the allocated managed buffer.
vgc_Buffer * vgc_create_buffer(vgc_GC *gc, size_t size);

/// @brief Create a managed buffer.
/// @param gc The garbage collector to use.
/// @param size The size of the buffer *(in bytes)* to allocate.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed buffer.
vgc_Buffer * vgc_create_buffer_ext(vgc_GC *gc, size_t size, vgc_Deconstructor dtor);

/// @brief Create a managed array.
/// @param tsize The size of an item contained within the array.
/// @param count The number of items the managed array can hold.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed array.
#define vgcx_create_array_ext(T, count, dtor)           vgc_create_array_ext(VGC_GLOBAL_GC, sizeof(T), count, dtor)

/// @brief Create a managed array.
/// @param gc The garbage collector to use.
/// @param T The type of an item contained within the array.
/// @param count The number of items the managed array can hold.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed array.
#define vgcx_create_array_pro(gc, T, count, dtor)       vgc_create_array_ext(gc, sizeof(T), count, dtor)

/// @brief Create a managed array.
/// @param T The type of an item contained within the array.
/// @param count The number of items the managed array can hold.
/// @return A pointer to the allocated managed array.
#define vgcx_create_array(T, count)     vgc_create_array(VGC_GLOBAL_GC, sizeof(T), count)

/// @brief Destroy a managed array.
/// @param array The array to destroy.
void vgc_destroy_array(vgc_Array *array);

// Core API macros

/// @brief Create a managed object.
/// @param gc The garbage collector to use.
/// @param T The type of the new object.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed object.
#define vgcx_new_ext(gc, T, dtor)       ((T *) vgc_malloc_ext(gc, sizeof(T), dtor))

/// @brief Create a managed object.
/// @param gc The garbage collector to use.
/// @param T The type of the new object.
/// @param dtor The deconstructor to call after freeing the managed memory.
/// @return A pointer to the allocated managed object.
#define vgcx_new(T)     vgcx_new_ext(VGC_GLOBAL_GC, T, NULL)

/// @brief Create a managed object and store it in a variable.
/// @param gc The garbage collector to use.
/// @param T The type of the new object.
/// @param name The name of the new variable.
/// @return A pointer to the allocated managed object.
#define vgcx_var_ext(gc, T, name, dtor)       T *name = vgcx_new_ext(gc, T, dtor)

/// @brief Create a managed object and store it in a variable.
/// @param T The type of the new object.
/// @param name The name of the new variable.
/// @return A pointer to the allocated managed object.
#define vgcx_var(T, name)       vgcx_var_ext(VGC_GLOBAL_GC, T, name, NULL)

// Auxilary API macros

/// @brief Begin the global garbage collector for all single-threaded applications.
#define vgcx_start()                    void *vgc__bp = malloc(sizeof(vgc_GC));\
                                        VGC_GLOBAL_GC = (vgc_GC *) vgc__bp;\
                                        vgc_start(VGC_GLOBAL_GC, &vgc__bp);\
                                        (void) 0
#define VGCX_BEGIN                      vgcx_start()

/// @brief Stop the global garbage collector for all single-threaded applications.
#define vgcx_stop()                     vgc_stop(VGC_GLOBAL_GC)
#define VGCX_END                        vgcx_stop()
#define vgcx_calloc(count, size)        vgc_calloc(VGC_GLOBAL_GC, count, size)
#define vgcx_free(ptr)                  (vgc_free(VGC_GLOBAL_GC, ptr))
#define vgcx_malloc(size)               vgc_malloc(VGC_GLOBAL_GC, size)
#define vgcx_carray(T, count)           vgcx_calloc(sizeof(T), count)
#define vgcx_free_array(T, array)       vgc_free_array(VGC_GLOBAL_GC, array)
#define vgcx_malloc_array(T, count)     vgc_malloc_array(VGC_GLOBAL_GC, sizeof(T), count)
#define vgcx_realloc(ptr, size)         vgc_realloc(VGC_GLOBAL_GC, ptr, size)

#define vgcx_create_stack()             void *_VGCX_STACK_BP = NULL
#define VGCX_CREATE_STACK               vgcx_create_stack()
#define vgcx_get_stack()                (&_VGCX_STACK_BP)
#define VGCX_STACK                      vgcx_get_stack()

// Auxilary API macros (exclusive to C)
#if !defined(__cplusplus)
#if !defined(new)
#define new(T)                  vgcx_new(T)
#endif // new
#if !defined(var)
#define var(T, name)            vgcx_var(T, name)
#endif // var
#endif // __cplusplus

#endif // VGC__VGC_H
