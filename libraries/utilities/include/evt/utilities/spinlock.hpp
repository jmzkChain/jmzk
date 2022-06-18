/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <boost/noncopyable.hpp>

/* Compile read-write barrier */
#define barrier() asm volatile("": : :"memory")

/* Pause instruction to prevent excess processor bus usage */
#define cpu_relax() asm volatile("pause\n": : :"memory")

static inline unsigned short xchg_8(void *ptr, unsigned char x)
{
    __asm__ __volatile__("xchgb %0,%1"
    :"=r" (x)
    :"m" (*(volatile unsigned char *)ptr), "0" (x)
    :"memory");

    return x;
}

#define BUSY 1

namespace jmzk { namespace utilities {

class spinlock : boost::noncopyable {
public:
    inline spinlock() : lock_(0) {}

public:
    inline void
    lock() {
        int wait = 1;
        while (1) {
            if (!xchg_8(&lock_, BUSY)) return;

            // wait here is important to performance.
            for (int i = 0; i < wait; i++) {
                cpu_relax();
            }
            while (lock_) {
                wait *= 2; // exponential backoff if can't get lock
                for (int i = 0; i < wait; i++) {
                    cpu_relax();
                }
            }
        }
    }

    inline int
    try_lock() {
        return !xchg_8(&lock_, BUSY);
    }

    inline void
    unlock() {
        barrier();
        lock_ = 0;
    }

private:
    unsigned char lock_;
};

class spinlock_guard : boost::noncopyable {
public:
    spinlock_guard(spinlock& lock) : lock_(lock) {
        lock_.lock();
    }

    ~spinlock_guard() {
        lock_.unlock();
    }

private:
    spinlock& lock_;
};

}}  // namespace jmzk::utilities

#undef BUSY
#undef barrier
#undef cpu_relax
