#pragma once

#include <cstddef>

/**
 * @brief A lightweight view over a contiguous buffer of elements
 * 
 * This class provides an iterable interface for raw buffers described by
 * a pointer and size, enabling use with range-based for loops and STL algorithms.
 * 
 * @tparam T The type of elements in the buffer
 * 
 * Example usage:
 * @code
 * int buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
 * BufferView view{buffer, 10};
 * 
 * // Range-based for loop
 * for (int val : view) {
 *     std::cout << val << " ";
 * }
 * 
 * // STL algorithms
 * std::sort(view.begin(), view.end());
 * auto it = std::find(view.begin(), view.end(), 5);
 * @endcode
 */
template<typename T>
struct BufferView {
    T* m_data;           ///< Pointer to the buffer
    std::size_t m_size;  ///< Number of elements in the buffer
    
    /**
     * @brief Get iterator to the beginning of the buffer
     * @return Pointer to the first element
     */
    T* begin() const noexcept { return m_data; }
    
    /**
     * @brief Get iterator to the end of the buffer
     * @return Pointer to one past the last element
     */
    T* end() const noexcept { return m_data + m_size; }
    
    /**
     * @brief Get const iterator to the beginning of the buffer
     * @return Const pointer to the first element
     */
    const T* cbegin() const noexcept { return m_data; }
    
    /**
     * @brief Get const iterator to the end of the buffer
     * @return Const pointer to one past the last element
     */
    const T* cend() const noexcept { return m_data + m_size; }
    
    /**
     * @brief Access element at index (unchecked)
     * @param idx Index of the element
     * @return Reference to the element
     */
    T& operator[](std::size_t idx) noexcept { return m_data[idx]; }
    
    /**
     * @brief Access element at index (unchecked, const)
     * @param idx Index of the element
     * @return Const reference to the element
     */
    const T& operator[](std::size_t idx) const noexcept { return m_data[idx]; }
    
    /**
     * @brief Check if the buffer is empty
     * @return true if size is 0, false otherwise
     */
    bool empty() const noexcept { return m_size == 0; }
};

// Deduction guide for C++17 CTAD
template<typename T>
BufferView(T*, std::size_t) -> BufferView<T>;
