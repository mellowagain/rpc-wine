#include "backoff.hh"

rpc_wine::backoff::backoff(int64_t min, int64_t max) : random_generator((uint64_t) time(nullptr)) {
    this->min = min;
    this->max = max;
    this->current = min;
    this->fails = 0;
}

void rpc_wine::backoff::reset() {
    this->fails = 0;
    this->current = this->min;
}

int64_t rpc_wine::backoff::next_delay() {
    ++this->fails;

    int64_t delay = (int64_t) ((double) current * 2.0 * this->distribution(this->random_generator));
    return std::min(this->current + delay, this->max);
}
