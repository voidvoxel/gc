#if !defined(VGC__VGC_C)
#define VGC__VGC_C

#include "vgc.h"

#include <errno.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGLEVEL LOGLEVEL_DEBUG

typedef enum vgc_LogLevel {
    LOGLEVEL_CRITICAL,
    LOGLEVEL_WARNING,
    LOGLEVEL_INFO,
    LOGLEVEL_DEBUG,
    LOGLEVEL_NONE
} vgc_LogLevel;

#if defined(DISABLE_LOGGING)

static const char * log_level_strings [] = { "CRIT", "WARN", "INFO", "DEBG", "NONE" };

#define log(level, fmt, ...) \
    do { if (level <= LOGLEVEL) fprintf(stderr, "[%s] %s:%s:%llu: " fmt "\n", log_level_strings[level], __func__, __FILE__, (long long unsigned int) __LINE__, __VA_ARGS__); } while (0)

#else

static vgc_LogLevel log(vgc_LogLevel log_level, ...)
{
    return log_level;
}

#endif // DEBUG

#define LOG_CRITICAL(fmt, ...) log(LOGLEVEL_CRITICAL, fmt, __VA_ARGS__)
#define LOG_WARNING(fmt, ...) log(LOGLEVEL_WARNING, fmt, __VA_ARGS__)
#define LOG_INFO(fmt, ...) log(LOGLEVEL_INFO, fmt, __VA_ARGS__)
#define LOG_DEBUG(fmt, ...) log(LOGLEVEL_DEBUG, fmt, __VA_ARGS__)

/*
 * Set log level for this compilation unit. If set to LOGLEVEL_DEBUG,
 * the garbage collector will be very chatty.
 */
#undef LOGLEVEL
#define LOGLEVEL LOGLEVEL_INFO

/*
 * The size of a pointer.
 */
#define VGC_PTRSIZE sizeof(void *)

/*
 * Allocations can temporarily be tagged as "marked" an part of the
 * mark-and-sweep implementation or can be tagged as "roots" which are
 * not automatically garbage collected. The latter allows the implementation
 * of global variables.
 */
#define VGC_TAG_NONE 0x0
#define VGC_TAG_ROOT 0x1
#define VGC_TAG_MARK 0x2

/*
 * Support for windows c compiler is added by adding this macro.
 * Tested on: Microsoft (R) C/C++ Optimizing Compiler Version 19.24.28314 for x86
 */
#if defined(_MSC_VER)
#include <intrin.h>

#define __builtin_frame_address(x)  ((void)(x), _AddressOfReturnAddress())
#endif

/*
 * Define a globally available GC object; this allows all code that
 * includes the gc.h header to access a global static garbage collector.
 * Convenient for single-threaded code, insufficient for multi-threaded
 * use cases. Use the VGC_NO_GLOBAL_GC flag to toggle.
 */
#ifndef VGC_NO_GLOBAL_GC
/// @brief A global garbage collector for all single-threaded applications.
vgc_GC *VGC_GLOBAL_GC;
#endif

static void vgc__array_set_buffer(vgc_Array *array, vgc_Buffer * value);

static void vgc__array_set_slot_count(vgc_Array *array, size_t value);

static void vgc__array_set_slot_size(vgc_Array *array, size_t value);

static void vgc__buffer_set_address(vgc_Buffer *buffer, void * value);

static void vgc__buffer_set_length(vgc_Buffer *buffer, size_t value);

static bool is_prime(size_t n) {
    /* https://stackoverflow.com/questions/1538644/c-determine-if-a-number-is-prime */
    if (n <= 3)
        return n > 1;     // as 2 and 3 are prime
    else if (n % 2==0 || n % 3==0)
        return false;     // check if n is divisible by 2 or 3
    else {
        for (size_t i=5; i*i<=n; i+=6) {
            if (n % i == 0 || n%(i + 2) == 0)
                return false;
        }
        return true;
    }
}

static size_t next_prime(size_t n) {
    while (!is_prime(n)) ++n;
    return n;
}

/**
 * Create a new allocation object.
 *
 * Creates a new allocation object using the system `malloc`.
 *
 * @param[in] ptr The pointer to the memory to manage.
 * @param[in] size The size of the memory range pointed to by `ptr`.
 * @param[in] dtor A pointer to a destructor function that should be called
 *                 before freeing the memory pointed to by `ptr`.
 * @returns Pointer to the new allocation instance.
 */
static vgc_Allocation * vgc_allocation_new(void *ptr, size_t size, vgc_Deconstructor dtor) {
    vgc_Allocation *a = (vgc_Allocation*) malloc(sizeof(vgc_Allocation));
    a->ptr = ptr;
    a->size = size;
    a->tag = VGC_TAG_NONE;
    a->dtor = dtor;
    a->next = NULL;
    return a;
}

/**
 * Delete an allocation object.
 *
 * Deletes the allocation object pointed to by `a`, but does *not*
 * free the memory pointed to by `a->ptr`.
 *
 * @param a The allocation object to delete.
 */
static void vgc_allocation_delete(vgc_Allocation *a) {
    free(a);
}

/**
 * Determine the current load factor of an `AllocationMap`.
 *
 * Calculates the load factor of the hash map as the quotient of the size and
 * the capacity of the hash map.
 *
 * @param am The allocationo map to calculate the load factor for.
 * @returns The load factor of the allocation map `am`.
 */
static double vgc_allocation_map_load_factor(vgc_AllocationMap * am) {
    return (double) am->size / (double) am->capacity;
}

static vgc_AllocationMap * vgc_allocation_map_new(size_t min_capacity,
        size_t capacity,
        double sweep_factor,
        double downsize_factor,
        double upsize_factor) {
    vgc_AllocationMap * am = (vgc_AllocationMap *) malloc(sizeof(vgc_AllocationMap));
    am->min_capacity = next_prime(min_capacity);
    am->capacity = next_prime(capacity);
    if (am->capacity < am->min_capacity) am->capacity = am->min_capacity;
    am->sweep_factor = sweep_factor;
    am->sweep_limit = (int) (sweep_factor * am->capacity);
    am->downsize_factor = downsize_factor;
    am->upsize_factor = upsize_factor;
    am->allocs = (vgc_Allocation**) calloc(am->capacity, sizeof(vgc_Allocation*));
    am->size = 0;
    LOG_DEBUG("Created allocation map (cap=%lld, siz=%lld)", (uint64_t) am->capacity, (uint64_t) am->size);
    return am;
}

static void vgc_allocation_map_delete(vgc_AllocationMap * am) {
    // Iterate over the map
    LOG_DEBUG("Deleting allocation map (cap=%lld, siz=%lld)",
              (uint64_t) am->capacity, (uint64_t) am->size);
    vgc_Allocation *alloc, *tmp;
    for (size_t i = 0; i < am->capacity; ++i) {
        if ((alloc = am->allocs[i])) {
            // Make sure to follow the chain inside a bucket
            while (alloc) {
                tmp = alloc;
                alloc = alloc->next;
                // free the management structure
                vgc_allocation_delete(tmp);
            }
        }
    }
    free(am->allocs);
    free(am);
}

static size_t vgc_hash(void *ptr) {
    return ((uintptr_t)ptr) >> 3;
}

static void vgc_allocation_map_resize(vgc_AllocationMap * am, size_t new_capacity) {
    if (new_capacity <= am->min_capacity) {
        return;
    }
    // Replaces the existing items array in the hash table
    // with a resized one and pushes items into the new, correct buckets
    LOG_DEBUG("Resizing allocation map (cap=%lld, siz=%lld) -> (cap=%lld)",
              (uint64_t) am->capacity, (uint64_t) am->size, (uint64_t) new_capacity);
    vgc_Allocation **resized_allocs = (vgc_Allocation**) calloc(new_capacity, sizeof(vgc_Allocation*));

    for (size_t i = 0; i < am->capacity; ++i) {
        vgc_Allocation *alloc = am->allocs[i];
        while (alloc) {
            vgc_Allocation *next_alloc = alloc->next;
            size_t new_index = vgc_hash(alloc->ptr) % new_capacity;
            alloc->next = resized_allocs[new_index];
            resized_allocs[new_index] = alloc;
            alloc = next_alloc;
        }
    }
    free(am->allocs);
    am->capacity = new_capacity;
    am->allocs = resized_allocs;
    am->sweep_limit = am->size + am->sweep_factor * (am->capacity - am->size);
}

static bool vgc_allocation_map_resize_to_fit(vgc_AllocationMap * am) {
    double load_factor = vgc_allocation_map_load_factor(am);
    if (load_factor > am->upsize_factor) {
        LOG_DEBUG("Load factor %0.3g > %0.3g. Triggering upsize.",
                  load_factor, am->upsize_factor);
        vgc_allocation_map_resize(am, next_prime(am->capacity * 2));
        return true;
    }
    if (load_factor < am->downsize_factor) {
        LOG_DEBUG("Load factor %0.3g < %0.3g. Triggering downsize.",
                  load_factor, am->downsize_factor);
        vgc_allocation_map_resize(am, next_prime(am->capacity / 2));
        return true;
    }
    return false;
}

static vgc_Allocation * vgc_allocation_map_get(vgc_AllocationMap * am, void *ptr) {
    size_t index = vgc_hash(ptr) % am->capacity;
    vgc_Allocation *cur = am->allocs[index];
    while(cur) {
        if (cur->ptr == ptr) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

static vgc_Allocation * vgc_allocation_map_put(vgc_AllocationMap * am,
        void *ptr,
        size_t size,
        vgc_Deconstructor dtor) {
    size_t index = vgc_hash(ptr) % am->capacity;
    LOG_DEBUG("PUT request for allocation ix=%lld", (uint64_t) index);
    vgc_Allocation *alloc = vgc_allocation_new(ptr, size, dtor);
    vgc_Allocation *cur = am->allocs[index];
    vgc_Allocation *prev = NULL;
    /* Upsert if ptr is already known (e.g. dtor update). */
    while(cur != NULL) {
        if (cur->ptr == ptr) {
            // found it
            alloc->next = cur->next;
            if (!prev) {
                // position 0
                am->allocs[index] = alloc;
            } else {
                // in the list
                prev->next = alloc;
            }
            vgc_allocation_delete(cur);
            LOG_DEBUG("AllocationMap Upsert at ix=%lld", (uint64_t) index);
            return alloc;

        }
        prev = cur;
        cur = cur->next;
    }
    /* Insert at the front of the separate chaining list */
    cur = am->allocs[index];
    alloc->next = cur;
    am->allocs[index] = alloc;
    am->size++;
    LOG_DEBUG("AllocationMap insert at ix=%lld", (uint64_t) index);
    void *p = alloc->ptr;
    if (vgc_allocation_map_resize_to_fit(am)) {
        alloc = vgc_allocation_map_get(am, p);
    }
    return alloc;
}

static void vgc_allocation_map_remove(vgc_AllocationMap * am,
                                     void *ptr,
                                     bool allow_resize) {
    // ignores unknown keys
    size_t index = vgc_hash(ptr) % am->capacity;
    vgc_Allocation *cur = am->allocs[index];
    vgc_Allocation *prev = NULL;
    vgc_Allocation *next;
    while(cur != NULL) {
        next = cur->next;
        if (cur->ptr == ptr) {
            // found it
            if (!prev) {
                // first item in list
                am->allocs[index] = cur->next;
            } else {
                // not the first item in the list
                prev->next = cur->next;
            }
            vgc_allocation_delete(cur);
            am->size--;
        } else {
            // move on
            prev = cur;
        }
        cur = next;
    }
    if (allow_resize) {
        vgc_allocation_map_resize_to_fit(am);
    }
}

static void * vgc_mcalloc(size_t count, size_t size) {
    if (!count) return malloc(size);
    return calloc(count, size);
}

static bool vgc_needs_sweep(vgc_GC *gc) {
    return gc->allocs->size > gc->allocs->sweep_limit;
}

static void * vgc_allocate(vgc_GC *gc, size_t count, size_t size, vgc_Deconstructor dtor) {
    /* Allocation logic that generalizes over malloc/calloc. */

    /* Check if we reached the high-water mark and need to clean up */
    if (vgc_needs_sweep(gc) && !gc->disabled) {
        size_t freed_mem = vgc_collect(gc);
        LOG_DEBUG("Garbage collection cleaned up %llu bytes.", freed_mem);
    }
    /* With cleanup out of the way, attempt to allocate memory */
    void *ptr = vgc_mcalloc(count, size);
    size_t alloc_size = count ? count * size : size;
    /* If allocation fails, force an out-of-policy run to free some memory and try again. */
    if (!ptr && !gc->disabled && (errno == EAGAIN || errno == ENOMEM)) {
        vgc_collect(gc);
        ptr = vgc_mcalloc(count, size);
    }
    /* Start managing the memory we received from the system */
    if (ptr) {
        LOG_DEBUG("Allocated %zu bytes at %p", alloc_size, (void *) ptr);
        vgc_Allocation *alloc = vgc_allocation_map_put(gc->allocs, ptr, alloc_size, dtor);
        /* Deal with metadata allocation failure */
        if (alloc) {
            LOG_DEBUG("Managing %zu bytes at %p", alloc_size, (void *) alloc->ptr);
            ptr = alloc->ptr;
        } else {
            /* We failed to allocate the metadata, fail cleanly. */
            free(ptr);
            ptr = NULL;
        }
    }
    return ptr;
}

static void vgc_make_root(vgc_GC *gc, void * const ptr) {
    vgc_Allocation *alloc = vgc_allocation_map_get(gc->allocs, ptr);
    if (alloc) {
        alloc->tag |= VGC_TAG_ROOT;
    }
}

void * vgc_malloc(vgc_GC *gc, size_t const size) {
    return vgc_malloc_ext(gc, size, NULL);
}

vgc_Array * vgc_create_array(vgc_GC *gc, size_t tsize, size_t count) {
    return vgc_create_array_ext(gc, tsize, count, NULL);
}

vgc_Array * vgc_create_array_ext(vgc_GC *gc, size_t tsize, size_t count, vgc_Deconstructor dtor) {
    // Allocate the memory required by the array.
    vgc_Array *array = vgcx_new_ext(gc, vgc_Array, dtor);

    // Allocate an underlying buffer for the array to store its values.
    vgc_Buffer *buffer = vgc_create_buffer(gc, count * tsize);

    // Set the underlying buffer that the array represents.
    vgc__array_set_buffer(array, buffer);

    // Set the number of slots the array contains.
    vgc__array_set_slot_count(array, count);

    // Set the size of a single slot in the array.
    vgc__array_set_slot_size(array, tsize);

    return array;
}

vgc_Buffer * vgc_create_buffer(vgc_GC *gc, size_t size) {
    return vgc_create_buffer_ext(gc, size, NULL);
}

vgc_Buffer * vgc_create_buffer_ext(vgc_GC *gc, size_t size, vgc_Deconstructor dtor) {
    // Create a new buffer.
    vgc_Buffer *buffer = vgcx_new_ext(gc, vgc_Buffer, dtor);

    // If a destructor was provided:
    if (dtor == NULL) {
        // Allocate the buffer's memory.
        vgc__buffer_set_address(buffer, vgc_malloc(gc, size));
        vgc__buffer_set_length(buffer, size);
    }
    // Otherwise:
    else {
        // Allocate the buffer's memory.
        vgc__buffer_set_address(buffer, vgc_malloc_ext(gc, size, dtor));
        vgc__buffer_set_length(buffer, size);
    }

    return buffer;
}

void * vgc_malloc_static(vgc_GC *gc, size_t size, vgc_Deconstructor dtor) {
    void *ptr = vgc_malloc_ext(gc, size, dtor);
    vgc_make_root(gc, ptr);
    return ptr;
}

void * vgc_make_static(vgc_GC *gc, void *ptr) {
    vgc_make_root(gc, ptr);
    return ptr;
}

void * vgc_malloc_ext(vgc_GC *gc, size_t size, vgc_Deconstructor dtor) {
    return vgc_allocate(gc, 0, size, dtor);
}


void * vgc_calloc(vgc_GC *gc, size_t count, size_t size) {
    return vgc_calloc_ext(gc, count, size, NULL);
}


void * vgc_calloc_ext(vgc_GC *gc, size_t count, size_t size,
                    vgc_Deconstructor dtor) {
    return vgc_allocate(gc, count, size, dtor);
}


void * vgc_realloc(vgc_GC *gc, void *p, size_t size) {
    vgc_Allocation *alloc = vgc_allocation_map_get(gc->allocs, p);
    if (p && !alloc) {
        // the user passed an unknown pointer
        errno = EINVAL;
        return NULL;
    }
    void *q = realloc(p, size);
    if (!q) {
        // realloc failed but p is still valid
        return NULL;
    }
    if (!p) {
        // allocation, not reallocation
        vgc_Allocation *alloc = vgc_allocation_map_put(gc->allocs, q, size, NULL);
        return alloc->ptr;
    }
    if (p == q) {
        // successful reallocation w/o copy
        alloc->size = size;
    } else {
        // successful reallocation w/ copy
        vgc_Deconstructor dtor = alloc->dtor;
        vgc_allocation_map_remove(gc->allocs, p, true);
        vgc_allocation_map_put(gc->allocs, q, size, dtor);
    }
    return q;
}

void vgc_free(vgc_GC *gc, void *ptr) {
    vgc_Allocation *alloc = vgc_allocation_map_get(gc->allocs, ptr);
    if (alloc) {
        if (alloc->dtor) {
            alloc->dtor(ptr);
        }
        vgc_allocation_map_remove(gc->allocs, ptr, true);
        free(ptr);
    } else {
        LOG_WARNING("Ignoring request to free unknown pointer %p", (void *) ptr);
    }
}

void vgc_start(vgc_GC *gc, void *stack_bp) {
    vgc_start_ext(gc, stack_bp, 1024, 1024, 0.2, 0.8, 0.5);
}

void vgc_start_ext(vgc_GC *gc,
                  void *stack_bp,
                  size_t initial_capacity,
                  size_t min_capacity,
                  double downsize_load_factor,
                  double upsize_load_factor,
                  double sweep_factor) {
    double downsize_limit = downsize_load_factor > 0.0 ? downsize_load_factor : 0.2;
    double upsize_limit = upsize_load_factor > 0.0 ? upsize_load_factor : 0.8;
    sweep_factor = sweep_factor > 0.0 ? sweep_factor : 0.5;
    gc->disabled = false;
    gc->stack_bp = stack_bp;
    initial_capacity = initial_capacity < min_capacity ? min_capacity : initial_capacity;
    gc->allocs = vgc_allocation_map_new(min_capacity, initial_capacity,
                                       sweep_factor, downsize_limit, upsize_limit);
    LOG_DEBUG("Created new garbage collector (cap=%lld, siz=%lld).", (uint64_t)(gc->allocs->capacity),
              (uint64_t)(gc->allocs->size));
}

void vgc_disable(vgc_GC *gc) {
    gc->disabled = true;
}

void vgc_enable(vgc_GC *gc) {
    gc->disabled = false;
}

void vgc_mark_alloc(vgc_GC *gc, void *ptr) {
    vgc_Allocation *alloc = vgc_allocation_map_get(gc->allocs, ptr);
    /* Mark if alloc exists and is not tagged already, otherwise skip */
    if (alloc && !(alloc->tag & VGC_TAG_MARK)) {
        LOG_DEBUG("Marking allocation (ptr=%p)", ptr);
        alloc->tag |= VGC_TAG_MARK;
        /* Iterate over allocation contents and mark them as well */
        LOG_DEBUG("Checking allocation (ptr=%p, size=%llu) contents", ptr, alloc->size);
        for (char *p = (char*) alloc->ptr;
                p <= (char*) alloc->ptr + alloc->size - VGC_PTRSIZE;
                ++p) {
            LOG_DEBUG("Checking allocation (ptr=%p) @%llu with value %p",
                      ptr, p-((char*) alloc->ptr), *(void **)p);
            vgc_mark_alloc(gc, *(void **)p);
        }
    }
}

void vgc_mark_stack(vgc_GC *gc) {
    LOG_DEBUG("Marking the stack (gc@%p) in increments of %lld", (void *) gc, (uint64_t)(sizeof(char)));
    void *stack_sp = __builtin_frame_address(0);
    void *stack_bp = gc->stack_bp;
    /* The stack grows towards smaller memory addresses, hence we scan stack_sp->stack_bp.
     * Stop scanning once the distance between stack_sp & stack_bp is too small to hold a valid pointer */
    for (char *p = (char*) stack_sp; p <= (char*) stack_bp - VGC_PTRSIZE; ++p) {
        vgc_mark_alloc(gc, *(void **)p);
    }
}

void vgc_mark_roots(vgc_GC *gc) {
    LOG_DEBUG("Marking roots%s", "");
    for (size_t i = 0; i < gc->allocs->capacity; ++i) {
        vgc_Allocation *chunk = gc->allocs->allocs[i];
        while (chunk) {
            if (chunk->tag & VGC_TAG_ROOT) {
                LOG_DEBUG("Marking root @ %p", chunk->ptr);
                vgc_mark_alloc(gc, chunk->ptr);
            }
            chunk = chunk->next;
        }
    }
}

void vgc_mark(vgc_GC *gc) {
    /* Note: We only look at the stack and the heap, and ignore BSS. */
    LOG_DEBUG("Initiating GC mark (gc@%p)", (void *) gc);
    /* Scan the heap for roots */
    vgc_mark_roots(gc);
    /* Dump registers onto stack and scan the stack */
    void (*volatile _mark_stack)(vgc_GC*) = vgc_mark_stack;
    jmp_buf ctx;
    memset(&ctx, 0, sizeof(jmp_buf));
    setjmp(ctx);
    _mark_stack(gc);
}

size_t vgc_sweep(vgc_GC *gc) {
    LOG_DEBUG("Initiating GC sweep (gc@%p)", (void *) gc);
    size_t total = 0;
    for (size_t i = 0; i < gc->allocs->capacity; ++i) {
        vgc_Allocation *chunk = gc->allocs->allocs[i];
        vgc_Allocation *next = NULL;
        /* Iterate over separate chaining */
        while (chunk) {
            if (chunk->tag & VGC_TAG_MARK) {
                LOG_DEBUG("Found used allocation %p (ptr=%p)", (void *) chunk, (void *) chunk->ptr);
                /* unmark */
                chunk->tag &= ~VGC_TAG_MARK;
                chunk = chunk->next;
            } else {
                LOG_DEBUG("Found unused allocation %p (%llu bytes @ ptr=%p)", (void *) chunk, chunk->size, (void *) chunk->ptr);
                /* no reference to this chunk, hence delete it */
                total += chunk->size;
                if (chunk->dtor) {
                    chunk->dtor(chunk->ptr);
                }
                free(chunk->ptr);
                /* and remove it from the bookkeeping */
                next = chunk->next;
                vgc_allocation_map_remove(gc->allocs, chunk->ptr, false);
                chunk = next;
            }
        }
    }
    vgc_allocation_map_resize_to_fit(gc->allocs);
    return total;
}

/**
 * Unset the ROOT tag on all roots on the heap.
 *
 * @param gc A pointer to a garbage collector instance.
 */
void vgc_unroot_roots(vgc_GC *gc) {
    LOG_DEBUG("Unmarking roots%s", "");
    for (size_t i = 0; i < gc->allocs->capacity; ++i) {
        vgc_Allocation *chunk = gc->allocs->allocs[i];
        while (chunk) {
            if (chunk->tag & VGC_TAG_ROOT) {
                chunk->tag &= ~VGC_TAG_ROOT;
            }
            chunk = chunk->next;
        }
    }
}

size_t vgc_stop(vgc_GC *gc) {
    vgc_unroot_roots(gc);
    size_t collected = vgc_sweep(gc);
    vgc_allocation_map_delete(gc->allocs);
    return collected;
}

size_t vgc_collect(vgc_GC *gc) {
    LOG_DEBUG("Initiating GC run (gc@%p)", (void *) gc);
    vgc_mark(gc);
    return vgc_sweep(gc);
}

char * vgc_strdup (vgc_GC *gc, const char *str1) {
    size_t len = strlen(str1) + 1;
    void *instance = vgc_malloc(gc, len);

    if (instance == NULL) {
        return NULL;
    }
    return (char*) memcpy(instance, str1, len);
}

static void vgc__array_set_buffer(vgc_Array *array, vgc_Buffer * value) {
    void *struct_bp = (void *) array;
    * (vgc_Buffer * *)((size_t) struct_bp + VGC_PTRSIZE * 0) = value;
}

static void vgc__array_set_slot_count(vgc_Array *array, size_t value) {
    void *struct_bp = (void *) array;
    * (size_t *)((size_t) struct_bp + VGC_PTRSIZE * 1) = value;
}

static void vgc__array_set_slot_size(vgc_Array *array, size_t value) {
    void *struct_bp = (void *) array;
    * (size_t *)((size_t) struct_bp + VGC_PTRSIZE * 2) = value;
}

static void vgc__buffer_set_address(vgc_Buffer *buffer, void * value) {
    void *struct_bp = (void *) buffer;
    * (void * *)((size_t) struct_bp + VGC_PTRSIZE * 0) = value;
}

static void vgc__buffer_set_length(vgc_Buffer *buffer, size_t value) {
    void *struct_bp = (void *) buffer;
    * (size_t *)((size_t) struct_bp + VGC_PTRSIZE * 1) = value;
}

#endif // VGC__VGC_C
