#include "parallel_runner.hpp"
#include <iostream>

// Test functions for parallel execution
bool check_disk_space() {
    std::cout << "Checking disk space...\n";
    return true;
}

bool check_memory() {
    std::cout << "Checking memory...\n";
    return true;
}

bool check_network() {
    std::cout << "Checking network...\n";
    return false;  // Simulate failure
}

bool check_permissions() {
    std::cout << "Checking permissions...\n";
    return false;  // Simulate failure
}

int main() {
    std::cout << "=== Example 1: Basic parallel execution ===\n";
    
    auto runner1 = make_parallel_runner(
        parallel_step([]() -> bool {
            std::cout << "Validation 1...\n";
            return true;
        }, "Validation 1 failed"),
        
        parallel_step([]() -> bool {
            std::cout << "Validation 2...\n";
            return false;
        }, "Validation 2 failed"),
        
        parallel_step([]() -> bool {
            std::cout << "Validation 3...\n";
            return true;
        }, "Validation 3 failed")
    );
    
    runner1.run();
    
    std::cout << "\nResults:\n";
    for (std::size_t i = 0; i < runner1.size(); ++i) {
        if (runner1.result(i)) {
            std::cout << "  Step " << i << ": Success\n";
        } else {
            std::cout << "  Step " << i << ": Failed - " 
                      << runner1.error_message(i) << "\n";
        }
    }
    
    std::cout << "\nSummary:\n";
    std::cout << "  Total steps: " << runner1.size() << "\n";
    std::cout << "  Successes: " << runner1.success_count() << "\n";
    std::cout << "  Failures: " << runner1.failure_count() << "\n";
    std::cout << "  All succeeded: " << (runner1.all_succeeded() ? "Yes" : "No") << "\n";
    std::cout << "  Any succeeded: " << (runner1.any_succeeded() ? "Yes" : "No") << "\n";
    
    std::cout << "\n=== Example 2: System health checks ===\n";
    
    auto health_checks = make_parallel_runner(
        parallel_step(check_disk_space, "Insufficient disk space"),
        parallel_step(check_memory, "Insufficient memory"),
        parallel_step(check_network, "Network unavailable"),
        parallel_step(check_permissions, "Permission denied")
    );
    
    health_checks.run();
    
    std::cout << "\nHealth Check Results:\n";
    if (health_checks.all_succeeded()) {
        std::cout << "All health checks passed!\n";
    } else {
        std::cout << "Some health checks failed:\n";
        for (std::size_t i = 0; i < health_checks.size(); ++i) {
            if (!health_checks.result(i)) {
                std::cout << "  - " << health_checks.error_message(i) << "\n";
            }
        }
    }
    
    std::cout << "\n=== Example 3: Rerun failed checks ===\n";
    
    auto checks = make_parallel_runner(
        parallel_step([]() { std::cout << "Check A\n"; return true; }, "Check A failed"),
        parallel_step([]() { std::cout << "Check B\n"; return false; }, "Check B failed"),
        parallel_step([]() { std::cout << "Check C\n"; return false; }, "Check C failed")
    );
    
    checks.run();
    
    std::cout << "\nRetrying failed checks:\n";
    for (std::size_t i = 0; i < checks.size(); ++i) {
        if (!checks.result(i)) {
            std::cout << "Retrying step " << i << "...\n";
            if (checks.rerun(i)) {
                std::cout << "  Retry succeeded!\n";
            } else {
                std::cout << "  Retry failed: " << checks.error_message(i) << "\n";
            }
        }
    }
    
    std::cout << "\n=== Example 4: Results array access ===\n";
    
    auto explicit_runner = make_parallel_runner(
        parallel_step([]() { std::cout << "Task 1\n"; return true; }, "Task 1 failed"),
        parallel_step([]() { std::cout << "Task 2\n"; return true; }, "Task 2 failed")
    );
    
    explicit_runner.run();
    
    const auto& results = explicit_runner.results();
    std::cout << "Results array: [";
    for (std::size_t i = 0; i < results.size(); ++i) {
        std::cout << (results[i] ? "true" : "false");
        if (i < results.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
    
    std::cout << "\n=== Example 5: Rerun all failed steps ===\n";
    
    int retry_count = 0;
    auto retry_runner = make_parallel_runner(
        parallel_step([&]() { 
            std::cout << "Flaky check 1 (attempt " << ++retry_count << ")\n"; 
            return retry_count >= 2;  // Succeeds on second try
        }, "Flaky check 1 failed"),
        
        parallel_step([]() { 
            std::cout << "Always succeeds\n"; 
            return true; 
        }, "Should not fail"),
        
        parallel_step([]() { 
            std::cout << "Always fails\n"; 
            return false; 
        }, "Always fails")
    );
    
    retry_runner.run();
    
    std::cout << "\nInitial results: " << retry_runner.success_count() << "/" 
              << retry_runner.size() << " succeeded\n";
    
    std::cout << "\nRetrying all failed steps...\n";
    std::size_t recovered = retry_runner.rerun_failed();
    
    std::cout << "\nAfter retry: " << retry_runner.success_count() << "/" 
              << retry_runner.size() << " succeeded\n";
    std::cout << "Recovered " << recovered << " step(s)\n";
    
    if (!retry_runner.all_succeeded()) {
        std::cout << "\nRemaining failures:\n";
        for (std::size_t i = 0; i < retry_runner.size(); ++i) {
            if (!retry_runner.result(i)) {
                std::cout << "  - Step " << i << ": " 
                          << retry_runner.error_message(i) << "\n";
            }
        }
    }
    
    return 0;
}
