#ifndef RPC_WINE_SERIALIZATION_HH
#define RPC_WINE_SERIALIZATION_HH

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <functional>

#include "allocators.hh"

namespace rpc_wine::serialization {

    constexpr size_t writer_nesting_levels = 2048 / (2 * sizeof(size_t));

    using malloc_allocator = rapidjson::CrtAllocator;
    using pool_allocator = rapidjson::MemoryPoolAllocator<malloc_allocator>;
    using utf8 = rapidjson::UTF8<char>;
    using stack_allocator = fixed_linear_allocator<2048>;
    using json_value = rapidjson::GenericValue<utf8, pool_allocator>;

    class direct_string_buffer {
    private:
        char *buffer;
        char *current;
        char *end;

    public:
        using Ch = char;

        direct_string_buffer(char *buffer, size_t max_length);

        void put(char c);
        void flush();

        // RapidJSON searches for these functions - let's alias them to the ones above
        void Put(char c);
        void Flush();

        size_t get_size() const;
    };

    using json_writer_base = rapidjson::Writer<direct_string_buffer, utf8, utf8, stack_allocator, rapidjson::kWriteNoFlags>;
    class json_writer : public json_writer_base {
    private:
        direct_string_buffer buffer;
        stack_allocator stack_alloc;

    public:
        json_writer(char *destination, size_t max_length);
        size_t size() const;
    };

    using json_document_base = rapidjson::GenericDocument<utf8, pool_allocator, stack_allocator>;
    class json_document : public json_document_base {
    private:
        char parse_buffer[32 * 1024];
        malloc_allocator malloc_alloc;
        pool_allocator pool_alloc;
        stack_allocator stack_alloc;

    public:
        static const int kDefaultChunkCapacity = 32 * 1024;

        json_document();
    };

    inline json_value *get_object_member(json_value *object, const char *name);

    inline int get_int_member(json_value *object, const char *name, int not_found_default = 0);

    inline const char *get_string_member(json_value *object, const char *name, const char *not_found_default = nullptr);

}

#endif //RPC_WINE_SERIALIZATION_HH
