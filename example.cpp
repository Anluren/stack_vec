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

    // Example 3: With custom types
    std::cout << "Example 3: With custom struct\n";
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

    // Example 4: Testing capacity limits
    std::cout << "Example 4: Testing capacity\n";
    StackVector<int, 3> small_vec;
    small_vec.push_back(1);
    small_vec.push_back(2);
    small_vec.push_back(3);
    
    std::cout << "Small vec size: " << small_vec.size() << "\n";
    std::cout << "Note: Attempting to exceed capacity will assert in debug builds\n\n";

    // Example 5: Using insert_range with array (integer types only)
    std::cout << "Example 5: Batch insert with insert_range (integer types)\n";
    StackVector<int, 20> batch_vec;
    
    int data[] = {10, 20, 30, 40, 50};
    batch_vec.insert_range(data, 5);  // Insert all 5 elements at once
    
    std::cout << "Batch inserted contents: ";
    for (const auto& val : batch_vec) {
        std::cout << val << " ";
    }
    std::cout << "\nSize: " << batch_vec.size() << "\n\n";

    // Example 6: Using initializer list constructor
    std::cout << "Example 6: Initializer list constructor\n";
    StackVector<int, 10> init_vec = {100, 200, 300, 400, 500};
    
    std::cout << "Initialized vector contents: ";
    for (const auto& val : init_vec) {
        std::cout << val << " ";
    }
    std::cout << "\nSize: " << init_vec.size() << "\n\n";

    // Example 7: Fill constructor with n copies of value
    std::cout << "Example 7: Fill constructor (n copies of value)\n";
    StackVector<int, 10> fill_vec(7, 42);  // 7 copies of 42
    
    std::cout << "Filled vector contents: ";
    for (const auto& val : fill_vec) {
        std::cout << val << " ";
    }
    std::cout << "\nSize: " << fill_vec.size() << "\n";

    return 0;
}
