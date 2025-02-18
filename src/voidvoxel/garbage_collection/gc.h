/*
 * gc - A simple mark and sweep garbage collector for C.
 */

#if !defined(VOIDVOXEL__GARBAGE_COLLECTION__GC_H)
#define VOIDVOXEL__GARBAGE_COLLECTION__GC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct AllocationMap;

typedef struct gc_GarbageCollector {
    struct AllocationMap *allocs; // allocation map
    bool paused;                  // (temporarily) switch gc on/off
    void *stack_bp;                    // bottom of stack
    size_t min_size;
} gc_GarbageCollector;

extern gc_GarbageCollector GC_GLOBAL_INSTANCE;

 /*
  * Starting, stopping, pausing, resuming and running the GC.
  */

/// @brief Run the garbage collector, freeing up any unreachable memory resources that are no longer being used.
/// @return The amount of memory freed (in bytes).
size_t gc_collect(gc_GarbageCollector *gc);

/// @brief Disable garbage collection.
void gc_disable(gc_GarbageCollector *gc);

/// @brief Enable garbage collection.
void gc_enable(gc_GarbageCollector *gc);

void gc_start(gc_GarbageCollector *gc, void *stack_bp);

void gc_start_ext(gc_GarbageCollector *gc, void *stack_bp, size_t initial_size, size_t min_size, double downsize_load_factor, double upsize_load_factor, double sweep_factor);

size_t gc_stop(gc_GarbageCollector *gc);

 /*
  * Allocating and deallocating memory.
  */

void * gc_malloc(gc_GarbageCollector *gc, size_t size);
void * gc_malloc_static(gc_GarbageCollector *gc, size_t size, void (*dtor)(void *));
void * gc_malloc_ext(gc_GarbageCollector *gc, size_t size, void (*dtor)(void *));
void * gc_calloc(gc_GarbageCollector *gc, size_t count, size_t size);
void * gc_calloc_ext(gc_GarbageCollector *gc, size_t count, size_t size, void (*dtor)(void *));
void * gc_realloc(gc_GarbageCollector *gc, void *ptr, size_t size);
void gc_free(gc_GarbageCollector *gc, void *ptr);

 /*
  * Lifecycle management
  */

void *gc_make_static(gc_GarbageCollector *gc, void *ptr);

 /*
  * Helper functions and stdlib replacements.
  */

char * gc_strdup(gc_GarbageCollector *gc, const char *s);

 #endif /* !VOIDVOXEL__GARBAGE_COLLECTION__GC_H */
