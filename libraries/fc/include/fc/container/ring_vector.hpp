
#pragma once

namespace fc {

template<typename T>
class ring_vector {
public:
    ring_vector(ssize_t capacity)
        : head_(0)
        , tail_(0)
        , capacity_(capacity) {
        assert(capacity > 0);
        buf_.resize(capacity);
    }

public:
    template<typename ... ARGS>
    void
    push_back(const T& item) {
        buf_[tail_] = item;

        if(++tail_ >= capacity_) {
            tail_ = 0;
        }
        if(head_ == tail_) {
            expand();
        }
    }

    void
    pop_front() {
        assert(head_ != tail_);

        front().~T();
        if(++head_ >= capacity_) {
            head_ = 0;
        }
    }

    void
    pop_back() {
        assert(head_ != tail_);

        back().~T();
        if(--tail_ < 0) {
            tail_ = capacity_ - 1;
        }
    }

    void
    clear() {
        for(auto i = 0; i < size(); i++) {
            buf_[(head_ + i) % capacity_].~T();
        }

        head_ = 0;
        tail_ = 0;
    }

    T&
    back() {
        assert(head_ != tail_);
        return buf_[(tail_ + capacity_ - 1) % capacity_];
    }

    const T&
    back() const {
        assert(head_ != tail_);
        return buf_[(tail_ + capacity_ - 1) % capacity_];
    }

    T&
    front() {
        assert(head_ != tail_);
        return buf_[head_];
    }

    const T&
    front() const {
        assert(head_ != tail_);
        return buf_[head_];
    }

    size_t
        size() const {
        auto length = tail_ - head_;
        if(length < 0) {
            length += capacity_;
        }
        return length;
    }

    bool
    empty() const {
        return head_ == tail_;
    }

    T&
    operator[](size_t index) {
        return buf_[(head_ + index) % capacity_];
    }

    const T&
    operator[](size_t index) const {
        return buf_[(head_ + index) % capacity_];
    }

private:
    void
    expand() {
        auto new_vec = std::vector<T>();
        new_vec.resize(capacity_ * 2);
        for(auto i = 0u; i < capacity_; i++) {
            new_vec[i] = buf_[(head_ + i) % capacity_];
        }

        head_      = 0;
        tail_      = capacity_;
        capacity_ *= 2;

        buf_.swap(new_vec);
    }

private:
    std::vector<T> buf_;
    ssize_t        head_;
    ssize_t        tail_;
    ssize_t        capacity_;
};

}  // namespace fc
