#if !defined(GC_HPP)
#define GC_HPP

#include <stddef.h>

#include "gc.h"


namespace voidvoxel
{
namespace garbage_collection
{
    class GarbageCollector
    {
    public:
        /// @brief Start the garbage collector.
        /// @tparam T The type of object at the BoS (Base of Stack).
        /// @param stack_bp
        template <typename T>
        GarbageCollector(T *stack_bp);

        template <typename T>
        GarbageCollector(T *stack_bp, size_t initial_size, size_t min_size, double downsize_load_factor, double upsize_load_factor, double sweep_factor);

        ~GarbageCollector();

        /// @brief Run the garbage collector, freeing up any unreachable memory resources that are no longer being used.
        /// @return The amount of memory freed (in bytes).
        size_t collect();

        /// @brief Pause the garbage collector.
        void pause();

        /// @brief Resume garbage collection.
        void resume();

        /// @brief Stop the garbage collector and prepare it for disposal.
        /// @return The size of the remaining memory (in bytes) that was freed upon stopping.
        size_t stop();

        /*
        * Allocating and deallocating memory.
        */

        template <typename T, typename... Args>
        static T * new_(Args... args);

        template <typename T, typename... Args>
        T * make_managed(Args... args);

        void * malloc(size_t size);

        template <typename T>
        T * malloc();

        void * malloc_static(size_t size, void (*dtor)(void *));

        void * malloc_ext(size_t size, void (*dtor)(void *));

        template <typename T>
        T * malloc_ext();

        template <typename T>
        T * malloc_ext(void (*dtor)(void *));

        void * calloc(size_t count, size_t size);

        void * calloc_ext(size_t count, size_t size, void (*dtor)(void *));

        void * realloc(void *ptr, size_t size);

        void free(void *ptr);

        /*
        * Lifecycle management
        */

        template <typename T>
        T * make_static(T *ptr);

        /*
        * Helper functions and stdlib replacements.
        */

        char* strdup (const char *s);
    private:
        gc_GarbageCollector _instance;
    };
}
}

#endif // GC_HPP
