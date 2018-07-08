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

void *rpc_wine::serialization::linear_allocator::realloc(void*, size_t, size_t new_size) {
    if (new_size == 0)
        return nullptr;

    return this->malloc(new_size);
}

void rpc_wine::serialization::linear_allocator::free(void*) {
    // Will never be called because kNeedFree is false
}
