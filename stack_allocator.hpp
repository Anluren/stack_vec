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
        : m_buffer(buffer)
        , m_buffer_size(N)
        , m_offset(0) {
    }

    template<typename U>
    StackAllocator(const StackAllocator<U, N, AlignAccess>& other) noexcept
        : m_buffer(other.m_buffer)
        , m_buffer_size(other.m_buffer_size)
        , m_offset(other.m_offset) {
    }

    pointer allocate(size_type n) {
        if (n == 0) {
            return nullptr;
        }

        size_type aligned_offset = m_offset;
        
        // Only align if AlignAccess is true
        if constexpr (AlignAccess) {
            const size_type alignment = alignof(T);
            aligned_offset = (m_offset + alignment - 1) & ~(alignment - 1);
        }
        
        const size_type bytes_needed = n * sizeof(T);
        
        if (aligned_offset + bytes_needed > m_buffer_size) {
            throw std::bad_alloc();
        }

        pointer result = reinterpret_cast<pointer>(m_buffer + aligned_offset);
        m_offset = aligned_offset + bytes_needed;
        
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
        if (ptr_as_char + bytes == m_buffer + m_offset) {
            m_offset = ptr_as_char - m_buffer;
        }
    }

    template<typename U, std::size_t M, bool A>
    bool operator==(const StackAllocator<U, M, A>& other) const noexcept {
        return m_buffer == other.m_buffer;
    }

    template<typename U, std::size_t M, bool A>
    bool operator!=(const StackAllocator<U, M, A>& other) const noexcept {
        return !(*this == other);
    }

    // Allow access to private members for rebind
    template<typename, std::size_t, bool>
    friend class StackAllocator;

private:
    char* m_buffer;
    size_type m_buffer_size;
    size_type m_offset;
};

// Helper class to manage the buffer and create vectors with stack allocator
template<typename T, std::size_t N, bool AlignAccess = false>
class StackVector {
public:
    using allocator_type = StackAllocator<T, N * sizeof(T), AlignAccess>;
    using vector_type = std::vector<T, allocator_type>;

    StackVector() 
        : m_alloc(reinterpret_cast<char*>(&m_buffer)) 
        , m_vec(m_alloc) {
        // Reserve the full capacity upfront to avoid reallocations
        m_vec.reserve(N);
    }

    // Access the underlying vector
    vector_type& get() { return m_vec; }
    const vector_type& get() const { return m_vec; }

    // Convenience forwarding methods
    void push_back(const T& value) { m_vec.push_back(value); }
    void push_back(T&& value) { m_vec.push_back(std::move(value)); }
    
    template<typename... Args>
    void emplace_back(Args&&... args) {
        m_vec.emplace_back(std::forward<Args>(args)...);
    }

    T& operator[](std::size_t idx) { return m_vec[idx]; }
    const T& operator[](std::size_t idx) const { return m_vec[idx]; }

    typename vector_type::iterator begin() { return m_vec.begin(); }
    typename vector_type::iterator end() { return m_vec.end(); }
    typename vector_type::const_iterator begin() const { return m_vec.begin(); }
    typename vector_type::const_iterator end() const { return m_vec.end(); }
    typename vector_type::const_iterator cbegin() const { return m_vec.cbegin(); }
    typename vector_type::const_iterator cend() const { return m_vec.cend(); }

    std::size_t size() const { return m_vec.size(); }
    std::size_t capacity() const { return m_vec.capacity(); }
    bool empty() const { return m_vec.empty(); }
    void clear() { m_vec.clear(); }
    void reserve(std::size_t n) { m_vec.reserve(n); }

    T* data() { return m_vec.data(); }
    const T* data() const { return m_vec.data(); }

private:
    // Use alignment only if AlignAccess is true
    typename std::conditional<AlignAccess, 
        typename std::aligned_storage<N * sizeof(T), alignof(T)>::type,
        char[N * sizeof(T)]>::type m_buffer;
    allocator_type m_alloc;
    vector_type m_vec;
};

#endif // STACK_ALLOCATOR_HPP
