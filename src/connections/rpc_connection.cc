#include <cstring>

#include "../serialization/writers.hh"
#include "rpc_connection.hh"

static rpc_wine::connection instance;

rpc_wine::connection *rpc_wine::connection::create(const char *app_id) {
    instance.connection = base_connection::create();
    strcpy(instance.app_id, app_id); // Yea discord there is it's called strcpy
    return &instance;
}

void rpc_wine::connection::destroy(rpc_wine::connection *&connection) {
    connection->close_connection();
    base_connection::destroy(connection->connection);
    delete connection;
}

void rpc_wine::connection::open_connection() {
    if (this->state == connection_state::CONNECTED)
        return;

    if (this->state == connection_state::DISCONNECTED && !this->connection->open_connection())
        return;

    if (this->state == connection_state::SENT_HANDSHAKE) {
        serialization::json_document message;

        if (read(message)) {
            const char *cmd = serialization::get_string_member(&message, "cmd");
            const char *event = serialization::get_string_member(&message, "evt");

            if (cmd != nullptr && event != nullptr && strcmp(cmd, "DISPATCH") == 0 && strcmp(event, "READY") == 0) {
                this->state = connection_state::CONNECTED;

                if (this->on_connect != nullptr)
                    this->on_connect(message);
            }
        }
    } else {
        this->send_frame.op_code = connection_op_code::HANDSHAKE;
        this->send_frame.length = serialization::write_handshake(
                this->send_frame.message, sizeof(this->send_frame.message), 1, app_id
        );

        if (this->connection->write(&this->send_frame, sizeof(message_frame_header) + this->send_frame.length)) {
            state = connection_state::SENT_HANDSHAKE;
            return;
        }

        this->close_connection();
    }
}

void rpc_wine::connection::close_connection() {
    if (this->on_disconnect != nullptr && (this->state == connection_state::CONNECTED || this->state == connection_state::SENT_HANDSHAKE))
        this->on_disconnect(this->last_error_code, this->last_error_message);

    this->connection->close_connection();
    this->state = connection_state::DISCONNECTED;
}

bool rpc_wine::connection::read(serialization::json_document &message) {
    if (this->state != connection_state::CONNECTED && this->state != connection_state::SENT_HANDSHAKE)
        return false;

    message_frame read_frame {};

    while (true) {
        bool successful_read = this->connection->read(&read_frame, sizeof(message_frame_header));

        if (!successful_read) {
            if (!this->connection->is_open) {
                this->last_error_code = (int) connection_error_code::PIPE_CLOSED;
                strcpy(this->last_error_message, "Pipe closed");
                this->close_connection();
            }

            return false;
        }

        if (read_frame.length > 0) {
            successful_read = this->connection->read(&read_frame.message, read_frame.length);

            if (!successful_read) {
                this->last_error_code = (int) connection_error_code::READ_CORRUPT;
                strcpy(this->last_error_message, "Partial data in frame");
                this->close_connection();

                return false;
            }

            read_frame.message[read_frame.length] = 0;
        }

        switch (read_frame.op_code) {
            case connection_op_code::FRAME:
                message.ParseInsitu(read_frame.message);

                return true;
            case connection_op_code::CLOSE:
                message.ParseInsitu(read_frame.message);

                this->last_error_code = serialization::get_int_member(&message, "code");
                strcpy(this->last_error_message, serialization::get_string_member(&message, "message", ""));
                this->close_connection();

                return false;
            case connection_op_code::PING:
                read_frame.op_code = connection_op_code::PONG;

                if (!this->connection->write(&read_frame, sizeof(message_frame_header) + read_frame.length))
                    this->close_connection();

                break;
            case connection_op_code::PONG:
                // You know this reminds me of my actual school
                // We used to have a replacement teacher that wasn't allowed to do a actual sports lesson with us
                // so we only played ping pong when she replaced our actual sports teacher.
                // A few years before these ping pong lessons that kid always used to play with the
                // other guys. He was pretty good at it despite never playing it outside of these small
                // break ping pong matches. That gained him fast popularity. He got tons of friends and
                // everybody wanted him in their match. Fast forward 5 years and that kid lost the skill in ping pong.
                // Now that he no longer has skill, nobody wants to play with that kid anymore. You could see him
                // just sit around in the sports lesson because nobody wanted him in their match. People stopped
                // caring about him because he no longer posses the skills he used to have.
                //
                // Please just let me in your match, I'm just want to have fun and play with you guys
                break;
            default:
                this->last_error_code = (int) connection_error_code::READ_CORRUPT;
                strcpy(this->last_error_message, "Bad IPC frame");
                this->close_connection();

                return false;
        }
    }
}

bool rpc_wine::connection::write(void *data, size_t length) {
    this->send_frame.op_code = connection_op_code::FRAME;
    memcpy(this->send_frame.message, data, length);
    this->send_frame.length = length;

    if (this->connection->write(&this->send_frame, sizeof(message_frame_header) + length))
        return true;

    this->close_connection();
    return false;
}
