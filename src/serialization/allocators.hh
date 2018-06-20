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
        void *realloc(void *original_ptr, size_t original_size, size_t new_size);
        static void free(void *ptr);
    };

    template <size_t size>
    class fixed_linear_allocator : public linear_allocator {
    public:
        static const bool kNeedFree = false;

        char fixed_buffer[size];

        fixed_linear_allocator();

        static void free(void *ptr);
        static void Free(void *ptr);
    };

}

#endif //RPC_WINE_ALLOCATORS_HH
