#include "stack_allocator.hpp"
#include <iostream>
#include <vector>

int main() {
    // Example 1: Using StackVector helper class (unaligned, default)
    std::cout << "Example 1: Using StackVector (unaligned access)\n";
    StackVector<int, 10> stack_vec;  // AlignAccess defaults to false
    
    for (int i = 0; i < 5; ++i) {
        stack_vec.push_back(i * 10);
    }
    
    std::cout << "Stack vector contents: ";
    for (const auto& val : stack_vec) {
        std::cout << val << " ";
    }
    std::cout << "\nSize: " << stack_vec.size() << "\n\n";

    // Example 2: Using StackVector with aligned access
    std::cout << "Example 2: Using StackVector (aligned access)\n";
    StackVector<int, 10, true> aligned_vec;  // AlignAccess = true
    
    for (int i = 0; i < 5; ++i) {
        aligned_vec.push_back(i * 20);
    }
    
    std::cout << "Aligned vector contents: ";
    for (const auto& val : aligned_vec) {
        std::cout << val << " ";
    }
    std::cout << "\nSize: " << aligned_vec.size() << "\n\n";

    // Example 3: Manual allocation with buffer (unaligned)
    std::cout << "Example 3: Manual buffer allocation (unaligned)\n";
    constexpr std::size_t buffer_size = 1024;
    char buffer[buffer_size];  // No alignas needed for unaligned access
    
    StackAllocator<double, buffer_size, false> alloc(buffer);
    std::vector<double, StackAllocator<double, buffer_size, false>> vec(alloc);
    
    for (int i = 0; i < 10; ++i) {
        vec.push_back(i * 3.14);
    }
    
    std::cout << "Manual vector contents: ";
    for (const auto& val : vec) {
        std::cout << val << " ";
    }
    std::cout << "\nSize: " << vec.size() << "\n\n";

    // Example 4: With custom types
    std::cout << "Example 4: With custom struct\n";
    struct Point {
        float x, y, z;
        Point(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    };
    
    StackVector<Point, 5> points;
    points.emplace_back(1.0f, 2.0f, 3.0f);
    points.emplace_back(4.0f, 5.0f, 6.0f);
    points.emplace_back(7.0f, 8.0f, 9.0f);
    
    std::cout << "Points: ";
    for (const auto& p : points) {
        std::cout << "(" << p.x << "," << p.y << "," << p.z << ") ";
    }
    std::cout << "\n\n";

    // Example 5: Testing capacity limits
    std::cout << "Example 5: Testing capacity\n";
    StackVector<int, 3> small_vec;
    small_vec.push_back(1);
    small_vec.push_back(2);
    small_vec.push_back(3);
    
    try {
        // This should throw std::bad_alloc as we exceed the buffer
        small_vec.push_back(4);
    } catch (const std::bad_alloc& e) {
        std::cout << "Caught expected exception: buffer full\n";
    }

    return 0;
}
