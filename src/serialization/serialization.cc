#include "serialization.hh"

rpc_wine::serialization::direct_string_buffer::direct_string_buffer(char *buffer, size_t max_length) {
    this->buffer = buffer;
    this->end = buffer + max_length;
    this->current = buffer;
}

void rpc_wine::serialization::direct_string_buffer::put(char c) {
    if (this->current < this->end)
        *this->current++ = c;
}

void rpc_wine::serialization::direct_string_buffer::flush() {
    // https://github.com/discordapp/discord-rpc/blob/master/src/serialization.h#L132
}

void rpc_wine::serialization::direct_string_buffer::Put(char c) {
    this->put(c);
}

void rpc_wine::serialization::direct_string_buffer::Flush() {
    this->flush();
}

size_t rpc_wine::serialization::direct_string_buffer::get_size() const {
    return this->current - this->buffer;
}

rpc_wine::serialization::json_writer::json_writer(char *destination, size_t max_length)
        : json_writer_base(this->buffer, &this->stack_alloc, writer_nesting_levels)
        , buffer(destination, max_length)
        , stack_alloc() {
    // Initialized in initializer list
}

size_t rpc_wine::serialization::json_writer::size() const {
    return this->buffer.get_size();
}

rpc_wine::serialization::json_document::json_document()
        : json_document_base(
            rapidjson::kObjectType, &this->pool_alloc, sizeof(this->stack_alloc.fixed_buffer), &this->stack_alloc
        )
        , pool_alloc(this->parse_buffer, sizeof(this->parse_buffer), kDefaultChunkCapacity, &this->malloc_alloc)
        , stack_alloc() {
    // Initialized in initializer list
}

rpc_wine::serialization::json_value *rpc_wine::serialization::get_object_member(rpc_wine::serialization::json_value *object, const char *name) {
    if (object != nullptr) {
        auto member = object->FindMember(name);

        if (member != object->MemberEnd() && member->value.IsObject())
            return &member->value;
    }

    return nullptr;
}

int rpc_wine::serialization::get_int_member(rpc_wine::serialization::json_value *object, const char *name, int not_found_default) {
    if (object != nullptr) {
        auto member = object->FindMember(name);

        if (member != object->MemberEnd() && member->value.IsInt())
            return member->value.GetInt();
    }

    return 0;
}

const char *rpc_wine::serialization::get_string_member(rpc_wine::serialization::json_value *object, const char *name, const char *not_found_default) {
    if (object != nullptr) {
        auto member = object->FindMember(name);

        if (member != object->MemberEnd() && member->value.IsString())
            return member->value.GetString();
    }

    return nullptr;
}
