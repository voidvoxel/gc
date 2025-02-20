#if !defined(VGC__VGC_CPP)
#define VGC__VGC_CPP

#include <memory>
#include <thread>

#include "vgc.hpp"

#include "vgc.c"


namespace vgc
{
    ThreadGCMap __thread_gc_map;

    void __thread_begin(void *stack_bp)
    {
        vgc::__thread_gc_map[VGCPP_THREAD_ID] = new GarbageCollector(stack_bp);
    }

    void __thread_end()
    {
        delete vgc::__thread_gc_map[VGCPP_THREAD_ID];
    }

    void *stop_global_instance(GarbageCollector *gc)
    {
        gc->stop();

        return nullptr;
    }

    /*
    ** class GarbageCollector
    */

    template <typename T>
    GarbageCollector::GarbageCollector(T *stack_bp)
    {
        // Start the garbage collector.
        vgc_start(&this->_instance, stack_bp);
    }

    template <typename T>
    GarbageCollector::GarbageCollector(T *stack_bp, size_t initial_size, size_t min_size, double downsize_load_factor, double upsize_load_factor, double sweep_factor)
    {
        // Start the garbage collector.
        vgc_start_ext(&this->_instance, stack_bp, initial_size, min_size, downsize_load_factor, upsize_load_factor, sweep_factor);
    }

    GarbageCollector::~GarbageCollector()
    {
        // Stop the garbage collector.
        this->stop();
    }

    size_t GarbageCollector::collect()
    {
        // Collect garbage.
        return vgc_collect(&this->_instance);
    }

    void GarbageCollector::pause()
    {
        // Pause the collection of garbage.
        return vgc_disable(&this->_instance);
    }

    void GarbageCollector::resume()
    {
        // Resume the collection of garbage.
        return vgc_enable(&this->_instance);
    }

    size_t GarbageCollector::stop()
    {
        // Stop the garbage collector.
        return vgc_stop(&this->_instance);
    }

    template <typename T, typename... Args>
    T * GarbageCollector::make_managed(Args... args)
    {
        T *instance = nullptr;

        instance = this->malloc_ext<T>();

        return new (instance) T (args...);
    }

    void * GarbageCollector::malloc(size_t size)
    {
        return vgc_malloc(&this->_instance, size);
    }

    template <typename T>
    T * GarbageCollector::malloc()
    {
        return (T *) this->malloc(sizeof(T));
    }

    void * GarbageCollector::malloc_static(size_t size, void (*dtor)(void *))
    {
        return vgc_malloc_static(&this->_instance, size, dtor);
    }

    void * GarbageCollector::malloc_ext(size_t size, void (*dtor)(void *))
    {
        return vgc_malloc_ext(&this->_instance, size, dtor);
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

            delete instance;

            // T *deconstruction_instance = malloc(&this->_instance, sizeof(T));

            // delete deconstruction_instance;
        };

        return (T *) this->malloc_ext(sizeof(T), dtor);
    }

    void * GarbageCollector::calloc(size_t count, size_t size)
    {
        return vgc_calloc(&this->_instance, count, size);
    }

    void * GarbageCollector::calloc_ext(size_t count, size_t size, void (*dtor)(void *))
    {
        return vgc_calloc_ext(&this->_instance, count, size, dtor);
    }

    void * GarbageCollector::realloc(void *ptr, size_t size)
    {
        return vgc_realloc(&this->_instance, ptr, size);
    }

    void GarbageCollector::free(void *ptr)
    {
        vgc_free(&this->_instance, ptr);
    }

    template <typename T>
    T * GarbageCollector::make_static(T *ptr)
    {
        return vgc_make_static(&this->_instance, ptr);
    }

    char * GarbageCollector::strdup(const char *s)
    {
        return vgc_strdup(&this->_instance, s);
    }
}

template <typename T>
T *vgcpp_new()
{
    return VGCPP__NEW(T);
}

int main(int argc, char const *argv[])
{
    vgcpp_begin();

    auto x = vgcpp_new<int>();

    vgcpp_end();

    return 0;
}


#endif // VGC__VGC_CPP
