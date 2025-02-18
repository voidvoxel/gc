#if !defined(VOIDVOXEL__GARBAGE_COLLECTION__COLLECTABLE_HPP)
#define VOIDVOXEL__GARBAGE_COLLECTION__COLLECTABLE_HPP

namespace voidvoxel
{
    namespace garbage_collection
    {
        class GarbageCollectable
        {
        public:
            virtual void __del__();
        };
    }
}

#endif // VOIDVOXEL__GARBAGE_COLLECTION__COLLECTABLE_HPP
