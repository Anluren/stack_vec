#include "function_runner.hpp"
#include <iostream>

// Some test functions that return bool (true = success, false = failure)
bool initialize_system() {
    std::cout << "Initializing system...\n";
    return true;
}

bool connect_to_database() {
    std::cout << "Connecting to database...\n";
    return true;
}

bool load_configuration() {
    std::cout << "Loading configuration...\n";
    return false;  // Simulate failure here
}

bool start_server() {
    std::cout << "Starting server...\n";
    return true;
}

int main() {
    std::cout << "=== Example 1: Basic usage with lambdas ===\n";
    
    FunctionRunner<3> runner1{{
        {[]() -> bool {
            std::cout << "Running check 1...\n";
            return true;
        }, "Check 1 failed: initialization error"},
        
        {[]() -> bool {
            std::cout << "Running check 2...\n";
            return false;  // This will fail
        }, "Check 2 failed: validation error"},
        
        {[]() -> bool {
            std::cout << "Running check 3...\n";
            return true;
        }, "Check 3 failed: connection error"}
    }};
    
    int failed_idx = runner1.run();
    
    if (failed_idx >= 0) {
        std::cout << "Failed at index: " << failed_idx << "\n\n";
    } else {
        std::cout << "All checks passed!\n\n";
    }
    
    std::cout << "=== Example 2: System startup sequence ===\n";
    
    FunctionRunner<4> startup{{
        {initialize_system, "Failed to initialize system"},
        {connect_to_database, "Failed to connect to database"},
        {load_configuration, "Failed to load configuration"},
        {start_server, "Failed to start server"}
    }};
    
    std::cout << "Total startup steps: " << startup.size() << "\n";
    
    failed_idx = startup.run();
    
    if (failed_idx >= 0) {
        std::cout << "Startup failed at step " << failed_idx << "\n\n";
    } else {
        std::cout << "Startup completed successfully!\n\n";
    }
    
    std::cout << "=== Example 3: All functions succeed ===\n";
    
    FunctionRunner<3> tasks{{
        {[]() { std::cout << "Task 1 complete\n"; return true; }, "Task 1 failed"},
        {[]() { std::cout << "Task 2 complete\n"; return true; }, "Task 2 failed"},
        {[]() { std::cout << "Task 3 complete\n"; return true; }, "Task 3 failed"}
    }};
    
    failed_idx = tasks.run();
    
    if (failed_idx >= 0) {
        std::cout << "Failed at index: " << failed_idx << "\n";
    } else {
        std::cout << "All tasks completed successfully!\n";
    }
    
    return 0;
}
