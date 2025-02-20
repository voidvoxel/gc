#if !defined(VGC__VGC_HPP)
#define VGC__VGC_HPP

#include <unordered_map>

#include "vgc.h"


#define VGCPP__GET_THREAD_ID()  (std::hash<std::thread::id>{}(std::this_thread::get_id()))


#define VGCPP_THREAD_ID (VGCPP__GET_THREAD_ID())


namespace vgc
{
    using ThreadGCMap = std::unordered_map<size_t, vgc::GarbageCollector *>;

    ThreadGCMap __thread_gc_map;

    size_t get_thread_id();

    class GarbageCollector
    {
    public:
        /// @brief Start an instance of the Void Garbage Collector.
        /// @tparam T The type of object at the BoS (Base of Stack).
        /// @param stack_bp The base-pointer to start collecting from.
        template <typename T>
        GarbageCollector(T *stack_bp);

        /// @brief Start an instance of the Void Garbage Collector.
        /// @tparam T The type of object at the BoS (Base of Stack).
        /// @param stack_bp The base-pointer to start collecting from.
        /// @param initial_size The initial size of the GC heap.
        /// @param min_size The minimum size of the GC heap.
        /// @param downsize_load_factor The down-size load factor.
        /// @param upsize_load_factor The up-size load factor.
        /// @param sweep_factor The sweep factor.
        template <typename T>
        GarbageCollector(T *stack_bp, size_t initial_size, size_t min_size, double downsize_load_factor, double upsize_load_factor, double sweep_factor);

        /// @brief Stop this instance of the Void Garbage Collector, freeing any remaining held resources.
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

        template <typename T, typename... Args>
        static T * new_(Args... args);

        /// @brief Create a new managed object.
        /// @tparam T The type of object to create.
        /// @tparam ...Args The types of the object's constructor's arguments.
        /// @param ...args A list of arguments to pass to the object's constructor.
        /// @return A pointer to the managed object.
        template <typename T, typename... Args>
        T * make_managed(Args... args);

        /// @brief Allocate a block of memory.
        /// @param size The size of the block of managed memory to allocate.
        /// @return A pointer to the allocated block of memory.
        void * malloc(size_t size);

        /// @brief Allocate a block of memory.
        /// @return A pointer to the allocated block of memory.
        template <typename T>
        T * malloc();

        /// @brief Allocate a block of static memory.
        /// @param size The size of the block of managed memory to allocate.
        /// @param dtor The deconstructor function to call upon deallocation.
        /// @return A pointer to the allocated block of memory.
        void * malloc_static(size_t size, void (*dtor)(void *));

        /// @brief Allocate a block of memory.
        /// @param size The size of the block of managed memory to allocate.
        /// @param dtor The deconstructor function to call upon deallocation.
        /// @return A pointer to the allocated block of memory.
        void * malloc_ext(size_t size, void (*dtor)(void *));

        /// @brief Allocate a block of memory.
        /// @tparam T The type of object to allocate memory for.
        /// @return A pointer to the allocated object.
        template <typename T>
        T * malloc_ext();

        /// @brief Allocate a block of memory.
        /// @tparam T The type of object to allocate memory for.
        /// @param dtor The deconstructor function to call upon deallocation.
        /// @return A pointer to the allocated object.
        template <typename T>
        T * malloc_ext(void (*dtor)(void *));

        /// @brief Allocate multiple blocks of managed memory at a time.
        /// @param count The number of blocks to allocate.
        /// @param size The size of each block to allocate.
        /// @return A pointer to the allocated array of blocks *(the number of elements in array is equal to `count`)*.
        void * calloc(size_t count, size_t size);

        /// @brief Allocate multiple blocks of managed memory at a time.
        /// @param count The number of blocks to allocate.
        /// @param size The size of each block to allocate.
        /// @param dtor The deconstructor function to call upon deallocation.
        /// @return A pointer to the allocated array of blocks *(the number of elements in array is equal to `count`)*.
        void * calloc_ext(size_t count, size_t size, void (*dtor)(void *));

        /// @brief Reallocate/resize a block of managed memory.
        /// @param ptr A pointer to the block of memory to reallocate/resize.
        /// @param size The new size of the block.
        /// @return A pointer to the relocated block of memory, now with the requested size.
        void * realloc(void *ptr, size_t size);

        /// @brief Deallocates the space previously allocated by `gc_malloc()`, `gc_calloc()`, `gc_aligned_alloc()` *(since C11)*, or `gc_realloc()`.
        /// @param ptr A pointer to the memory to deallocate.
        void free(void *ptr);

        template <typename T>
        T * make_static(T *ptr);

        /// @brief Returns a pointer to a null-terminated byte string, which is a duplicate of the string pointed to by str1.
        /// @param str1 A pointer to the null-terminated byte string to duplicate.
        /// @return A pointer to the allocated string, or a null pointer if an error occurred.
        char* strdup (const char *str1);
    private:
        vgc_GC _instance;
    };
}


#define VGCPP__BEGIN()  {\
    vgc::GarbageCollector *VGCPP__THREAD_GC = vgc::__thread_gc_map[VGCPP_THREAD_ID];\
    (void) 0

#define vgcpp_begin()   VGCPP__BEGIN()

#define VGCPP__END()        VGCPP__THREAD_GC = nullptr;\
                        }\
                        (void) 0

#define vgcpp_end()     VGCPP__END()

#define VGCPP__NEW(T)    (VGCPP__THREAD_GC->make_managed<T>)


#endif // VGC__VGC_HPP
