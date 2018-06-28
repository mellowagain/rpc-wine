#ifndef RPC_WINE_ALLOCATORS_HH
#define RPC_WINE_ALLOCATORS_HH

#include <cstddef>

namespace rpc_wine::serialization {

    class linear_allocator {
    private:
        char *buffer;
        char *end;

    public:
        static const bool kNeedFree = false;

        linear_allocator() = default;
        linear_allocator(char *buffer, size_t size);

        void *malloc(size_t size);
        void *realloc(void*, size_t, size_t new_size);
        static void free(void*);
    };

    template <size_t size>
    class fixed_linear_allocator : public linear_allocator {
    public:
        static const bool kNeedFree = false;

        char fixed_buffer[size] {};

        fixed_linear_allocator() : linear_allocator(this->fixed_buffer, size) {
            // Initialized in initializer list
        }

        void *Realloc(void *original_ptr, size_t original_size, size_t new_size) {
            return realloc(original_ptr, original_size, new_size);
        }

        static void Free(void *ptr) {
            free(ptr);
        }

    };

}

#endif //RPC_WINE_ALLOCATORS_HH
