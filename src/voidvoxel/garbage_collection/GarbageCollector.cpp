#if !defined(VOIDVOXEL__GARBAGE_COLLECTION__GARBAGE_COLLECTOR_CPP)
#define VOIDVOXEL__GARBAGE_COLLECTION__GARBAGE_COLLECTOR_CPP

#include <memory>
#include <type_traits>

#include "gc.hpp"

#include "gc.c"


namespace voidvoxel
{
namespace garbage_collection
{
    template <typename T>
    GarbageCollector::GarbageCollector(T *stack_bp)
    {
        // Start the garbage collector.
        gc_start(&this->_instance, stack_bp);
    }

    template <typename T>
    GarbageCollector::GarbageCollector(T *stack_bp, size_t initial_size, size_t min_size, double downsize_load_factor, double upsize_load_factor, double sweep_factor)
    {
        // Start the garbage collector.
        gc_start_ext(&this->_instance, stack_bp, initial_size, min_size, downsize_load_factor, upsize_load_factor, sweep_factor);
    }

    GarbageCollector::~GarbageCollector()
    {
        // Stop the garbage collector.
        this->stop();
    }

    /*
    Starting, stopping, pausing, resuming and running the GC.
    */

    size_t GarbageCollector::collect()
    {
        // Collect garbage.
        return gc_collect(&this->_instance);
    }

    void GarbageCollector::pause()
    {
        // Pause the collection of garbage.
        return gc_disable(&this->_instance);
    }

    void GarbageCollector::resume()
    {
        // Resume the collection of garbage.
        return gc_enable(&this->_instance);
    }

    size_t GarbageCollector::stop()
    {
        // Stop the garbage collector.
        return gc_stop(&this->_instance);
    }

    /*
    Allocating and deallocating memory.
    */

    template <typename T, typename... Args>
    T * GarbageCollector::make_managed(Args... args)
    {
        T *instance = nullptr;

        instance = this->malloc_ext<T>();

        return new (instance) T (args...);
    }

    void * GarbageCollector::malloc(size_t size)
    {
        return gc_malloc(&this->_instance, size);
    }

    template <typename T>
    T * GarbageCollector::malloc()
    {
        return (T *) this->malloc(sizeof(T));
    }

    void * GarbageCollector::malloc_static(size_t size, void (*dtor)(void *))
    {
        return gc_malloc_static(&this->_instance, size, dtor);
    }

    void * GarbageCollector::malloc_ext(size_t size, void (*dtor)(void *))
    {
        return gc_malloc_ext(&this->_instance, size, dtor);
    }

    template <typename T>
    T * GarbageCollector::malloc_ext(void (*dtor)(void *))
    {
        return (T *) this->malloc_ext(sizeof(T), dtor);
    }

    template <typename T>
    T * GarbageCollector::malloc_ext()
    {
        void (*dtor)(void *) = [](void *memory) mutable
        {
            T *instance = (T *) memory;

            instance->__del__();
        };

        return (T *) this->malloc_ext(sizeof(T), dtor);
    }

    void * GarbageCollector::calloc(size_t count, size_t size)
    {
        return gc_calloc(&this->_instance, count, size);
    }

    void * GarbageCollector::calloc_ext(size_t count, size_t size, void (*dtor)(void *))
    {
        return gc_calloc_ext(&this->_instance, count, size, dtor);
    }

    void * GarbageCollector::realloc(void *ptr, size_t size)
    {
        return gc_realloc(&this->_instance, ptr, size);
    }

    void GarbageCollector::free(void *ptr)
    {
        gc_free(&this->_instance, ptr);
    }

    /*
    Lifecycle management
    */

    template <typename T>
    T * GarbageCollector::make_static(T *ptr)
    {
        return gc_make_static(&this->_instance, ptr);
    }

    /*
    Helper functions and stdlib replacements.
    */

    char * GarbageCollector::strdup(const char *s)
    {
        return gc_strdup(&this->_instance, s);
    }
}
}

#endif // VOIDVOXEL__GARBAGE_COLLECTION__GARBAGE_COLLECTOR_CPP
