#ifndef STACK_ALLOCATOR_HPP
#define STACK_ALLOCATOR_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>
#include <cassert>

/**
 * @brief Custom allocator that uses a fixed-size buffer for allocations
 * 
 * This allocator owns a fixed-size buffer and allocates memory from it sequentially.
 * It's designed for use cases where heap allocation overhead should be avoided.
 * 
 * @tparam T The type of objects to allocate
 * @tparam N The size of the buffer in bytes
 * @tparam AlignAccess Whether to enforce alignment (default: true)
 * 
 * @note When AlignAccess is false, allocations are packed without padding for maximum space efficiency
 * @note This allocator does not throw exceptions; allocation failures return nullptr
 * @note Each allocator instance owns its own buffer
 */
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

    /**
     * @brief Default constructor
     * 
     * Initializes the allocator with an empty buffer (offset = 0)
     */
    StackAllocator() noexcept 
        : m_offset(0) {
    }

    /**
     * @brief Copy constructor
     * 
     * Creates a new allocator with its own buffer. Does not copy the buffer contents.
     * Required by C++ allocator requirements for use with std::vector.
     * 
     * @param other The allocator to copy from (only used for type compatibility)
     * @note Each allocator instance has its own independent buffer
     */
    StackAllocator(const StackAllocator&) noexcept
        : m_offset(0) {
    }

    /**
     * @brief Rebind copy constructor
     * 
     * Allows conversion from allocators of different types with the same buffer size.
     * Creates a new allocator with its own buffer.
     * 
     * @tparam U The value type of the other allocator
     * @param other The allocator to convert from
     */
    template<typename U>
    StackAllocator(const StackAllocator<U, N, AlignAccess>&) noexcept
        : m_offset(0) {
    }

    /**
     * @brief Allocate memory for n objects of type T
     * 
     * Allocates memory from the fixed-size buffer. If AlignAccess is true,
     * the allocation will be properly aligned for type T.
     * 
     * @param n Number of objects to allocate
     * @return Pointer to allocated memory, or nullptr if allocation fails
     * 
     * @note This function never throws. On failure, it asserts in debug builds and returns nullptr.
     * @note Allocations are made sequentially from the buffer
     */
    pointer allocate(size_type n) noexcept {
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
        
        if (aligned_offset + bytes_needed > N) {
            // Out of buffer space - return nullptr instead of throwing
            assert(false && "StackAllocator: buffer overflow");
            return nullptr;
        }

        pointer result = reinterpret_cast<pointer>(reinterpret_cast<char*>(&m_buffer) + aligned_offset);
        m_offset = aligned_offset + bytes_needed;
        
        return result;
    }

    /**
     * @brief Deallocate memory previously allocated
     * 
     * This implementation only reclaims space if the deallocation matches the most recent
     * allocation (stack-like behavior). Otherwise, the space remains used until the allocator
     * is destroyed.
     * 
     * @param p Pointer to the memory to deallocate
     * @param n Number of objects being deallocated
     * 
     * @note This function never throws
     */
    void deallocate(pointer p, size_type n) noexcept {
        // For stack allocator, we typically don't free individual allocations
        // The buffer is freed when it goes out of scope
        // We could implement a simple stack-like deallocation if the pointer
        // matches the last allocation
        
        const char* ptr_as_char = reinterpret_cast<const char*>(p);
        const size_type bytes = n * sizeof(T);
        char* buffer_ptr = reinterpret_cast<char*>(&m_buffer);
        
        // If this was the last allocation, we can reclaim the space
        if (ptr_as_char + bytes == buffer_ptr + m_offset) {
            m_offset = ptr_as_char - buffer_ptr;
        }
    }

    /**
     * @brief Compare allocators for equality
     * 
     * Two allocators are equal if they refer to the same buffer.
     * 
     * @tparam U Value type of the other allocator
     * @tparam M Buffer size of the other allocator
     * @tparam A AlignAccess setting of the other allocator
     * @param other The allocator to compare with
     * @return true if allocators share the same buffer, false otherwise
     */
    template<typename U, std::size_t M, bool A>
    bool operator==(const StackAllocator<U, M, A>& other) const noexcept {
        return &m_buffer == &other.m_buffer;
    }

    /**
     * @brief Compare allocators for inequality
     * 
     * @tparam U Value type of the other allocator
     * @tparam M Buffer size of the other allocator
     * @tparam A AlignAccess setting of the other allocator
     * @param other The allocator to compare with
     * @return true if allocators don't share the same buffer, false otherwise
     */
    template<typename U, std::size_t M, bool A>
    bool operator!=(const StackAllocator<U, M, A>& other) const noexcept {
        return !(*this == other);
    }

    // Allow access to private members for rebind
    template<typename, std::size_t, bool>
    friend class StackAllocator;

private:
    /// Fixed-size buffer for allocations (aligned or unaligned based on AlignAccess)
    typename std::conditional<AlignAccess,
        typename std::aligned_storage<N, alignof(T)>::type,
        char[N]>::type m_buffer;
    /// Current offset into the buffer for next allocation
    size_type m_offset;
};

/**
 * @brief Helper wrapper around std::vector with StackAllocator
 * 
 * This class provides a convenient interface for using std::vector with a fixed-size
 * buffer allocator. It automatically reserves the full capacity on construction to
 * prevent reallocations.
 * 
 * @tparam T The type of elements in the vector
 * @tparam N The maximum number of elements (not bytes)
 * @tparam AlignAccess Whether to enforce alignment (default: false for max efficiency)
 * 
 * @note This class is not copyable (copy constructor and assignment are deleted)
 * @note Move operations are supported
 * @note Capacity is fixed at N elements
 */
template<typename T, std::size_t N, bool AlignAccess = false>
class StackVector {
public:
    using allocator_type = StackAllocator<T, N * sizeof(T), AlignAccess>;
    using vector_type = std::vector<T, allocator_type>;

    /**
     * @brief Default constructor
     * 
     * Creates a StackVector and reserves the full capacity upfront to prevent
     * reallocations that would exceed the fixed buffer size.
     */
    StackVector() 
        : m_alloc()
        , m_vec(m_alloc) {
        // Reserve the full capacity upfront to avoid reallocations
        m_vec.reserve(N);
    }

    /// Copy constructor is deleted (buffer cannot be efficiently copied)
    StackVector(const StackVector&) = delete;
    /// Copy assignment is deleted (buffer cannot be efficiently copied)
    StackVector& operator=(const StackVector&) = delete;

    /// Move constructor is allowed
    StackVector(StackVector&&) = default;
    /// Move assignment is allowed
    StackVector& operator=(StackVector&&) = default;

    /**
     * @brief Get reference to the underlying std::vector
     * @return Reference to the internal vector
     */
    vector_type& get() noexcept { return m_vec; }
    
    /**
     * @brief Get const reference to the underlying std::vector
     * @return Const reference to the internal vector
     */
    const vector_type& get() const noexcept { return m_vec; }

    // Convenience forwarding methods
    
    /** @brief Add element to the end (copy) */
    void push_back(const T& value) { m_vec.push_back(value); }
    
    /** @brief Add element to the end (move) */
    void push_back(T&& value) noexcept(noexcept(std::declval<vector_type>().push_back(std::move(value)))) { 
        m_vec.push_back(std::move(value)); 
    }
    
    /**
     * @brief Construct element in-place at the end
     * @tparam Args Types of arguments to forward to T's constructor
     * @param args Arguments to forward to T's constructor
     */
    template<typename... Args>
    void emplace_back(Args&&... args) noexcept(noexcept(std::declval<vector_type>().emplace_back(std::forward<Args>(args)...))) {
        m_vec.emplace_back(std::forward<Args>(args)...);
    }

    /** @brief Access element at index (unchecked) */
    T& operator[](std::size_t idx) noexcept { return m_vec[idx]; }
    /** @brief Access element at index (unchecked, const) */
    const T& operator[](std::size_t idx) const noexcept { return m_vec[idx]; }

    /** @brief Get iterator to beginning */
    typename vector_type::iterator begin() noexcept { return m_vec.begin(); }
    /** @brief Get iterator to end */
    typename vector_type::iterator end() noexcept { return m_vec.end(); }
    /** @brief Get const iterator to beginning */
    typename vector_type::const_iterator begin() const noexcept { return m_vec.begin(); }
    /** @brief Get const iterator to end */
    typename vector_type::const_iterator end() const noexcept { return m_vec.end(); }
    /** @brief Get const iterator to beginning */
    typename vector_type::const_iterator cbegin() const noexcept { return m_vec.cbegin(); }
    /** @brief Get const iterator to end */
    typename vector_type::const_iterator cend() const noexcept { return m_vec.cend(); }

    /** @brief Get number of elements */
    std::size_t size() const noexcept { return m_vec.size(); }
    /** @brief Get capacity (always N) */
    std::size_t capacity() const noexcept { return m_vec.capacity(); }
    /** @brief Check if empty */
    bool empty() const noexcept { return m_vec.empty(); }
    /** @brief Remove all elements */
    void clear() noexcept { m_vec.clear(); }
    /** @brief Reserve capacity (no-op if n <= N) */
    void reserve(std::size_t n) { m_vec.reserve(n); }

    /** @brief Get pointer to underlying data */
    T* data() noexcept { return m_vec.data(); }
    /** @brief Get const pointer to underlying data */
    const T* data() const noexcept { return m_vec.data(); }

private:
    /// The allocator instance that owns the fixed-size buffer
    allocator_type m_alloc;
    /// The vector using the stack allocator
    vector_type m_vec;
};

#endif // STACK_ALLOCATOR_HPP
