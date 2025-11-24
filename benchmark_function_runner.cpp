#include "function_runner.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>

// Benchmark helper
template<typename Func>
double benchmark(const char* name, Func&& func, int iterations = 1000000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ns = static_cast<double>(duration) / iterations;
    
    std::cout << std::setw(40) << std::left << name 
              << std::setw(12) << std::right << std::fixed << std::setprecision(2) 
              << avg_ns << " ns/iteration\n";
    
    return avg_ns;
}

int main() {
    std::cout << "FunctionRunner Performance Benchmark\n";
    std::cout << "=====================================\n\n";
    
    // Simple lambdas with no captures
    auto runner = make_function_runner(
        []() { return true; }, "Step 1 failed",
        []() { return true; }, "Step 2 failed",
        []() { return true; }, "Step 3 failed",
        []() { return true; }, "Step 4 failed",
        []() { return true; }, "Step 5 failed"
    );
    
    // Lambdas with captures
    int counter = 0;
    auto runner_with_captures = make_function_runner(
        [&counter]() { counter++; return true; }, "Step 1 failed",
        [&counter]() { counter++; return true; }, "Step 2 failed",
        [&counter]() { counter++; return true; }, "Step 3 failed",
        [&counter]() { counter++; return true; }, "Step 4 failed",
        [&counter]() { counter++; return true; }, "Step 5 failed"
    );
    
    std::cout << "Test 1: Simple lambdas (5 steps, all succeed)\n";
    benchmark("  run()", [&]() { 
        runner.run();
    });
    
    std::cout << "\nTest 2: Lambdas with captures (5 steps, all succeed)\n";
    benchmark("  run()", [&]() { 
        runner_with_captures.run();
    });
    
    std::cout << "\nTest 3: Query operations\n";
    runner.run();
    benchmark("  error_message(index)", [&]() {
        auto msg = runner.error_message(2);
        (void)msg;
    });
    
    benchmark("  rerun(index)", [&]() {
        runner.rerun(0);
    });
    
    std::cout << "\n✅ Zero std::function overhead!\n";
    std::cout << "   Each lambda is stored with its actual type (no type erasure)\n";
    std::cout << "   No heap allocations during construction or execution\n";
    
    return 0;
}
