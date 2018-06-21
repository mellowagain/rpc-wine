#include "../rpc_wine.hh"
#include "io_thread.hh"

io_thread_holder::~io_thread_holder() {
    this->stop();
}

void io_thread_holder::start() {
    this->keep_running.store(true);
    this->thread = std::thread([&]() -> void {
        const std::chrono::duration<int64_t, std::milli> max_wait { 500LL };
        rpcw_update_connection();

        while (this->keep_running.load()) {
            std::unique_lock<std::mutex> lock(this->mutex);
            this->wait_for_io_activity.wait_for(lock, max_wait);
            rpcw_update_connection();
        }
    });
}

void io_thread_holder::stop() {
    this->keep_running.exchange(false);
    this->notify();

    if (this->thread.joinable())
        this->thread.join();
}

void io_thread_holder::notify() {
    this->wait_for_io_activity.notify_all();
}
