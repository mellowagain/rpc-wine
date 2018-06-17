#ifndef RPC_WINE_IO_THREAD_HH
#define RPC_WINE_IO_THREAD_HH

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class io_thread_holder {
private:
    std::atomic_bool keep_running { true };
    std::mutex mutex;
    std::condition_variable wait_for_io_activity;
    std::thread thread;

public:
    ~io_thread_holder();
    void start();
    void stop();
    void notify();

};

#endif //RPC_WINE_IO_THREAD_HH
