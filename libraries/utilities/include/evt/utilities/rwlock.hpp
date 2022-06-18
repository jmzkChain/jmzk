/**
 *  @file
 *  @copyright defined in jmzk/LICENSE.txt
 */
#pragma once

#include <stdint.h>
#include <atomic>
#include <boost/noncopyable.hpp>

// Scalable Reader-Writer Synchronization for Shared-Memory Multiprocessors

/* Pause instruction to prevent excess processor bus usage */
#define cpu_relax() asm volatile("pause\n": : :"memory")

#define RWLOCK_WIFLAG   0x01u
#define RWLOCK_WAFLAG   0x02u
#define RWLOCK_RCINCR   0x04u

namespace jmzk { namespace utilities {

class scoped_lock : boost::noncopyable {
public:
    scoped_lock() : next(nullptr), blocked(0) {}

public:
    scoped_lock*    next;
    std::atomic_int blocked;
};

template <typename T>
class rwlock_read_guard : boost::noncopyable {
public:
    rwlock_read_guard(T& rwlock) : lock_(), rwlock_(rwlock) {
        rwlock.start_read(&lock_);
    }

    ~rwlock_read_guard() {
        rwlock_.end_read(&lock_);
    }

private:
    scoped_lock lock_;
    T&          rwlock_;
};

template <typename T>
class rwlock_write_guard : boost::noncopyable {
public:
    rwlock_write_guard(T& rwlock) : lock_(), rwlock_(rwlock) {
        rwlock.start_write(&lock_);
    }

    ~rwlock_write_guard() {
        rwlock_.end_write(&lock_);
    }

private:
    scoped_lock lock_;
    T&          rwlock_;
};

// Reader-preference queue-based lock with local-only spinning
// From http://cs.rochester.edu/research/synchronization/pseudocode/rw.html#s_rp
class rprw_lock : boost::noncopyable {
public:
    // layout of rdr_cnt_and_flags:
    //  31     ...      2       1              0
    // +-----------------+-------------+-----------------+
    // | interested rdrs | active wtr? | interested wtr? |
    // +-----------------+-------------+-----------------+

public:
    rprw_lock() : read_head_(nullptr),
                  write_tail_(nullptr),
                  write_head_(nullptr),
                  rdr_cnt_and_flags_(0) {}

    inline void
    start_write(scoped_lock* lock) {
        lock->blocked.store(1, std::memory_order_relaxed);
        lock->next = nullptr;
        auto pred = write_tail_.load(std::memory_order_acquire);
        while(!write_tail_.compare_exchange_weak(pred, lock, std::memory_order_release, std::memory_order_relaxed)) { }

        if(pred == nullptr) {
            write_head_.store(lock, std::memory_order_release);
            if(rdr_cnt_and_flags_.fetch_or(RWLOCK_WIFLAG) == 0) {
                uint32_t flag = RWLOCK_WIFLAG;
                if(rdr_cnt_and_flags_.compare_exchange_strong(flag, RWLOCK_WAFLAG)) {
                    return;
                }
            }
            // else readers will wake up the writer
        }
        else {
            pred->next = lock;
        }

        int wait = 0;
        // wait here is important to performance.
        for (int i = 0; i < wait; i++) {
            cpu_relax();
        }
        while (lock->blocked.load(std::memory_order_relaxed)) {
            wait *= 2; // exponential backoff if can't get lock
            for (int i = 0; i < wait; i++) {
                cpu_relax();
            }
        }
    }

    inline void
    end_write(scoped_lock* lock) {
        write_head_.store(nullptr, std::memory_order_release);
        // clear wtr flag and test for waiting readers
        if(rdr_cnt_and_flags_.fetch_and(~RWLOCK_WAFLAG) != 0) {
            // waiting readers exist
            auto head = read_head_.load(std::memory_order_acquire);
            while(!read_head_.compare_exchange_weak(head, nullptr, std::memory_order_release, std::memory_order_relaxed)) { }

            if(head != nullptr) {
                head->blocked.store(0, std::memory_order_relaxed);
            }
        }

        // testing next is strictly an optimization
        if(lock->next != nullptr || !write_tail_.compare_exchange_strong(lock, nullptr)) {
            while(lock->next == nullptr) {}  // resolve successor
            write_head_.store(lock->next, std::memory_order_release);
            if(rdr_cnt_and_flags_.fetch_or(RWLOCK_WIFLAG) == 0) {
                uint32_t flag = RWLOCK_WIFLAG;
                if(rdr_cnt_and_flags_.compare_exchange_strong(flag, RWLOCK_WAFLAG)) {
                    write_head_.load()->blocked.store(0, std::memory_order_relaxed);
                    return;
                }
            }
            // else readers will wake up the writer
        }
    }

    inline void
    start_read(scoped_lock* lock) {
        // incr reader count, test if writer active
        if(rdr_cnt_and_flags_.fetch_add(RWLOCK_RCINCR) & RWLOCK_WAFLAG) {
            lock->blocked.store(1, std::memory_order_relaxed);
            lock->next = read_head_.load(std::memory_order_acquire);
            while (!read_head_.compare_exchange_weak(lock->next, lock, std::memory_order_release, std::memory_order_relaxed)) { }

            if((rdr_cnt_and_flags_.load() & RWLOCK_WAFLAG) == 0) {
                // writer no longer active; wake any waiting readers
                auto head = read_head_.load(std::memory_order_relaxed);
                while (!read_head_.compare_exchange_weak(head, nullptr, std::memory_order_release, std::memory_order_relaxed)) { }
                if (head != nullptr) {
                    head->blocked.store(0, std::memory_order_relaxed);
                }
            }

            int wait = 0;
            // wait here is important to performance.
            for (int i = 0; i < wait; i++) {
                cpu_relax();
            }
            while (lock->blocked.load(std::memory_order_relaxed)) {
                wait *= 2; // exponential backoff if can't get lock
                for (int i = 0; i < wait; i++) {
                    cpu_relax();
                }
            }

            if(lock->next != nullptr) {
                lock->next->blocked.store(0, std::memory_order_relaxed);
            }
        }
    }

    inline void
    end_read(scoped_lock* lock) {
        // if I am the last reader, resume the first waiting writer (if any)
        if(rdr_cnt_and_flags_.fetch_sub(RWLOCK_RCINCR) == (RWLOCK_RCINCR + RWLOCK_WIFLAG)) {
            uint32_t flag = RWLOCK_WIFLAG;
            if(rdr_cnt_and_flags_.compare_exchange_strong(flag, RWLOCK_WAFLAG)) {
                write_head_.load()->blocked.store(0, std::memory_order_relaxed);
            }
        }
    }

private:
    std::atomic<scoped_lock*> read_head_;
    std::atomic<scoped_lock*> write_tail_;
    std::atomic<scoped_lock*> write_head_;
    std::atomic_uint          rdr_cnt_and_flags_;
};

}}  // namespace jmzk::utilities

#undef cpu_relax
#undef RWLOCK_WIFLAG
#undef RWLOCK_WAFLAG
#undef RWLOCK_RCINCR
