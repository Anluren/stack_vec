#include "buffer_view.hpp"
#include <iostream>
#include <algorithm>
#include <numeric>

int main() {
    // Example 1: Basic usage with array
    std::cout << "Example 1: Basic iteration\n";
    int buffer[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    BufferView view{buffer, 10};
    
    std::cout << "Buffer contents: ";
    for (int val : view) {
        std::cout << val << " ";
    }
    std::cout << "\n\n";

    // Example 2: Using with STL algorithms
    std::cout << "Example 2: STL algorithms\n";
    auto it = std::find(view.begin(), view.end(), 5);
    if (it != view.end()) {
        std::cout << "Found 5 at position: " << (it - view.begin()) << "\n";
    }
    
    std::cout << "Sum: " << std::accumulate(view.begin(), view.end(), 0) << "\n\n";

    // Example 3: Modifying through view
    std::cout << "Example 3: Modifying elements\n";
    for (auto& val : view) {
        val *= 2;
    }
    
    std::cout << "After doubling: ";
    for (int val : view) {
        std::cout << val << " ";
    }
    std::cout << "\n\n";

    // Example 4: Sorting
    std::cout << "Example 4: Sorting in reverse\n";
    int unsorted[] = {5, 2, 8, 1, 9, 3, 7, 4, 6};
    BufferView<int> sort_view{unsorted, 9};
    
    std::sort(sort_view.begin(), sort_view.end(), std::greater<int>());
    
    std::cout << "Sorted (descending): ";
    for (int val : sort_view) {
        std::cout << val << " ";
    }
    std::cout << "\n\n";

    // Example 5: Const view
    std::cout << "Example 5: Const buffer view\n";
    const int const_buffer[] = {10, 20, 30, 40, 50};
    BufferView<const int> const_view{const_buffer, 5};
    
    std::cout << "Const buffer: ";
    for (int val : const_view) {
        std::cout << val << " ";
    }
    std::cout << "\n";

    return 0;
}
