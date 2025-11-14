#ifndef STACK_ALLOCATOR_HPP
#define STACK_ALLOCATOR_HPP

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

template<typename T, std::size_t N, bool AlignAccess = true>
class StackAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template<typename U>
    struct rebind {
        using other = StackAllocator<U, N, AlignAccess>;
    };

    StackAllocator(char* buffer) noexcept 
        : buffer_(buffer)
        , buffer_size_(N)
        , offset_(0) {
    }

    template<typename U>
    StackAllocator(const StackAllocator<U, N, AlignAccess>& other) noexcept
        : buffer_(other.buffer_)
        , buffer_size_(other.buffer_size_)
        , offset_(other.offset_) {
    }

    pointer allocate(size_type n) {
        if (n == 0) {
            return nullptr;
        }

        size_type aligned_offset = offset_;
        
        // Only align if AlignAccess is true
        if constexpr (AlignAccess) {
            const size_type alignment = alignof(T);
            aligned_offset = (offset_ + alignment - 1) & ~(alignment - 1);
        }
        
        const size_type bytes_needed = n * sizeof(T);
        
        if (aligned_offset + bytes_needed > buffer_size_) {
            throw std::bad_alloc();
        }

        pointer result = reinterpret_cast<pointer>(buffer_ + aligned_offset);
        offset_ = aligned_offset + bytes_needed;
        
        return result;
    }

    void deallocate(pointer p, size_type n) noexcept {
        // For stack allocator, we typically don't free individual allocations
        // The buffer is freed when it goes out of scope
        // We could implement a simple stack-like deallocation if the pointer
        // matches the last allocation
        
        const char* ptr_as_char = reinterpret_cast<const char*>(p);
        const size_type bytes = n * sizeof(T);
        
        // If this was the last allocation, we can reclaim the space
        if (ptr_as_char + bytes == buffer_ + offset_) {
            offset_ = ptr_as_char - buffer_;
        }
    }

    template<typename U, std::size_t M, bool A>
    bool operator==(const StackAllocator<U, M, A>& other) const noexcept {
        return buffer_ == other.buffer_;
    }

    template<typename U, std::size_t M, bool A>
    bool operator!=(const StackAllocator<U, M, A>& other) const noexcept {
        return !(*this == other);
    }

    // Allow access to private members for rebind
    template<typename, std::size_t, bool>
    friend class StackAllocator;

private:
    char* buffer_;
    size_type buffer_size_;
    size_type offset_;
};

// Helper class to manage the buffer and create vectors with stack allocator
template<typename T, std::size_t N, bool AlignAccess = false>
class StackVector {
public:
    using allocator_type = StackAllocator<T, N * sizeof(T), AlignAccess>;
    using vector_type = std::vector<T, allocator_type>;

    StackVector() 
        : alloc_(reinterpret_cast<char*>(&buffer_)) 
        , vec_(alloc_) {
        // Reserve the full capacity upfront to avoid reallocations
        vec_.reserve(N);
    }

    // Access the underlying vector
    vector_type& get() { return vec_; }
    const vector_type& get() const { return vec_; }

    // Convenience forwarding methods
    void push_back(const T& value) { vec_.push_back(value); }
    void push_back(T&& value) { vec_.push_back(std::move(value)); }
    
    template<typename... Args>
    void emplace_back(Args&&... args) {
        vec_.emplace_back(std::forward<Args>(args)...);
    }

    T& operator[](std::size_t idx) { return vec_[idx]; }
    const T& operator[](std::size_t idx) const { return vec_[idx]; }

    typename vector_type::iterator begin() { return vec_.begin(); }
    typename vector_type::iterator end() { return vec_.end(); }
    typename vector_type::const_iterator begin() const { return vec_.begin(); }
    typename vector_type::const_iterator end() const { return vec_.end(); }
    typename vector_type::const_iterator cbegin() const { return vec_.cbegin(); }
    typename vector_type::const_iterator cend() const { return vec_.cend(); }

    std::size_t size() const { return vec_.size(); }
    std::size_t capacity() const { return vec_.capacity(); }
    bool empty() const { return vec_.empty(); }
    void clear() { vec_.clear(); }
    void reserve(std::size_t n) { vec_.reserve(n); }

    T* data() { return vec_.data(); }
    const T* data() const { return vec_.data(); }

private:
    // Use alignment only if AlignAccess is true
    typename std::conditional<AlignAccess, 
        typename std::aligned_storage<N * sizeof(T), alignof(T)>::type,
        char[N * sizeof(T)]>::type buffer_;
    allocator_type alloc_;
    vector_type vec_;
};

#endif // STACK_ALLOCATOR_HPP
