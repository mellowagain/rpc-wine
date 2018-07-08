#include "writers.hh"

rpc_wine::serialization::write_object::write_object(rpc_wine::serialization::json_writer &writer) : writer(writer) {
    this->writer.StartObject();
}

template <typename T>
rpc_wine::serialization::write_object::write_object(rpc_wine::serialization::json_writer &writer, T &name) : writer(writer) {
    write_key(this->writer, name);
    this->writer.StartObject();
}

rpc_wine::serialization::write_object::~write_object() {
    this->writer.EndObject();
}

template<typename T>
rpc_wine::serialization::write_array::write_array(rpc_wine::serialization::json_writer &writer, T &name) : writer(writer) {
    write_key(this->writer, name);
    this->writer.StartArray();
}

rpc_wine::serialization::write_array::~write_array() {
    this->writer.EndArray();
}

template<typename T>
void rpc_wine::serialization::write_key(rpc_wine::serialization::json_writer &writer, T &key) {
    writer.Key(key, sizeof(T) - 1);
}

template<typename T>
void rpc_wine::serialization::write_optional_string(rpc_wine::serialization::json_writer &writer, T &key, const char *value) {
    if (value != nullptr && value[0]) {
        writer.Key(key, sizeof(T) - 1);
        writer.String(value);
    }
}

void rpc_wine::serialization::write_nonce(rpc_wine::serialization::json_writer &writer, int nonce) {
    write_key(writer, "nonce");
    writer.String(std::to_string(nonce).c_str());
}

size_t rpc_wine::serialization::write_rich_presence(char *destination, size_t max_length, int nonce, int pid, const discord_rich_presence *presence) {
    json_writer writer(destination, max_length);

    {
        write_object root(writer);

        write_nonce(writer, nonce);

        write_key(writer, "cmd");
        writer.String("SET_ACTIVITY");

        {
            write_object args(writer, "args");

            write_key(writer, "pid");
            writer.Int(pid);

            if (presence != nullptr) {
                write_object activity(writer, "activity");

                write_optional_string(writer, "state", presence->state);
                write_optional_string(writer, "details", presence->details);

                if (presence->start_timestamp != 0 || presence->end_timestamp != 0) {
                    write_object timestamps(writer, "timestamps");

                    if (presence->start_timestamp != 0) {
                        write_key(writer, "start");
                        writer.Int64(presence->start_timestamp);
                    }

                    if (presence->end_timestamp != 0) {
                        write_key(writer, "end");
                        writer.Int64(presence->end_timestamp);
                    }
                }

                if ((presence->large_image_key != nullptr && presence->large_image_key[0]) ||
                    (presence->large_image_text != nullptr && presence->large_image_text[0]) ||
                    (presence->small_image_key != nullptr && presence->small_image_key[0]) ||
                    (presence->small_image_text != nullptr && presence->small_image_text[0])) {
                    write_object assets(writer, "assets");

                    write_optional_string(writer, "large_image", presence->large_image_key);
                    write_optional_string(writer, "large_text", presence->large_image_text);
                    write_optional_string(writer, "small_image", presence->small_image_key);
                    write_optional_string(writer, "small_text", presence->small_image_text);
                }

                if ((presence->party_id != nullptr && presence->party_id[0]) || presence->party_size != 0 || presence->party_max != 0) {
                    write_object party(writer, "party");

                    write_optional_string(writer, "id", presence->party_id);

                    if (presence->party_size != 0 && presence->party_max != 0) {
                        write_array size(writer, "size");

                        writer.Int(presence->party_size);
                        writer.Int(presence->party_max);
                    }
                }

                if ((presence->match_secret != nullptr && presence->match_secret[0]) ||
                    (presence->join_secret != nullptr && presence->join_secret[0]) ||
                    (presence->spectate_secret != nullptr && presence->spectate_secret[0])) {
                    write_object secrets(writer, "secrets");

                    write_optional_string(writer, "match", presence->match_secret);
                    write_optional_string(writer, "join", presence->join_secret);
                    write_optional_string(writer, "spectate", presence->spectate_secret);
                }

                writer.Key("instance");
                writer.Bool(presence->instance != 0);
            }
        }
    }

    return writer.size();
}

size_t rpc_wine::serialization::write_handshake(char *destination, size_t max_length, int version, const char *app_id) {
    json_writer writer(destination, max_length);

    {
        write_object object(writer);

        write_key(writer, "v");
        writer.Int(version);
        write_key(writer, "client_id");
        writer.String(app_id);
    }

    return writer.size();
}

size_t rpc_wine::serialization::write_subscribe(char *destination, size_t max_length, int nonce, const char *event_name) {
    json_writer writer(destination, max_length);

    {
        write_object object(writer);

        write_nonce(writer, nonce);

        write_key(writer, "cmd");
        writer.String("SUBSCRIBE");

        write_key(writer, "evt");
        writer.String(event_name);
    }

    return writer.size();
}

size_t rpc_wine::serialization::write_unsubscribe(char *destination, size_t max_length, int nonce, const char *event_name) {
    json_writer writer(destination, max_length);

    {
        write_object object(writer);

        write_nonce(writer, nonce);

        write_key(writer, "cmd");
        writer.String("UNSUBSCRIBE");

        write_key(writer, "evt");
        writer.String(event_name);
    }

    return writer.size();
}

size_t rpc_wine::serialization::write_join_reply(char *destination, size_t max_length, const char *user_id, int reply, int nonce) {
    json_writer writer(destination, max_length);

    {
        write_object object(writer);

        writer.String(reply == DISCORD_REPLY_YES ? "SEND_ACTIVITY_JOIN_INVITE" : "CLOSE_ACTIVITY_JOIN_REQUEST");

        write_key(writer, "args");
        {
            write_object args(writer);

            write_key(writer, "user_id");
            writer.String(user_id);
        }

        write_nonce(writer, nonce);
    }

    return writer.size();
}
