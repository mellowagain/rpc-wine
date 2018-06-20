#include <cassert>

#include "allocators.hh"

rpc_wine::serialization::linear_allocator::linear_allocator(char *buffer, size_t size) {
    this->buffer = buffer;
    this->end = buffer + size;
}

void *rpc_wine::serialization::linear_allocator::malloc(size_t size) {
    char *response = this->buffer;
    this->buffer += size;

    if (this->buffer > this->end) {
        this->buffer = response;
        return nullptr;
    }

    return response;
}

void *rpc_wine::serialization::linear_allocator::realloc(void *original_ptr, size_t original_size, size_t new_size) {
    if (new_size == 0)
        return nullptr;

    assert(original_ptr != nullptr && original_size != 0);

    // I don't know what the fuck is going on with Discord
    (void) original_ptr;
    (void) original_size;

    return this->malloc(new_size);
}

void rpc_wine::serialization::linear_allocator::free(void *ptr) {
    // Here again wtf?!
    (void) ptr;
}

template <size_t size>
rpc_wine::serialization::fixed_linear_allocator<size>::fixed_linear_allocator() : linear_allocator(this->fixed_buffer, size) {
    // Initialized in initializer list
}

template<size_t size>
void rpc_wine::serialization::fixed_linear_allocator<size>::free(void *ptr) {
    // TODO: Unimplemented stub free
}

template<size_t size>
void rpc_wine::serialization::fixed_linear_allocator<size>::Free(void *ptr) {
    free(ptr);
}
