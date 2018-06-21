#ifndef RPC_WINE_MESSAGE_QUEUE_HH
#define RPC_WINE_MESSAGE_QUEUE_HH

#include <atomic>
#include <cstddef>

namespace rpc_wine {

    template <typename T, size_t queue_size>
    class message_queue {
    private:
        T queue[queue_size];

        std::atomic_uint next_add { 0 };
        std::atomic_uint next_send { 0 };
        std::atomic_uint pending { 0 };

    public:
        message_queue() = default;

        T* get_next_add_message() {
            if (this->pending.load() >= queue_size)
                return nullptr;

            auto index = (this->next_add++) % queue_size;
            return &this->queue[index];
        }

        void commit_add() {
            ++this->pending;
        }

        T* get_next_send_message() {
            auto index = (this->next_send++) % queue_size;
            return &this->queue[index];
        }

        void commit_send() {
            --this->pending;
        }

        bool has_pending_sends() const {
            return this->pending.load() != 0;
        }
    };

}

#endif //RPC_WINE_MESSAGE_QUEUE_HH
