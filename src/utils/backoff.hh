#ifndef RPC_WINE_BACKOFF_HH
#define RPC_WINE_BACKOFF_HH

#include <cstdint>
#include <random>

namespace rpc_wine {

    class backoff {
    private:
        int64_t min;
        int64_t max;
        int64_t current;

        int fails;

        std::mt19937_64 random_generator;
        std::uniform_real_distribution<> distribution;

    public:
        backoff(int64_t min, int64_t max);

        void reset();
        int64_t next_delay();
    };

}

#endif //RPC_WINE_BACKOFF_HH
