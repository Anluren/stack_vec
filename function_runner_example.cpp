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
    std::cout << "=== Example 1: Using make_function_runner (no template parameter needed) ===\n";
    
    auto runner1 = make_function_runner(
        step([]() -> bool {
            std::cout << "Running check 1...\n";
            return true;
        }, "Check 1 failed: initialization error"),
        
        step([]() -> bool {
            std::cout << "Running check 2...\n";
            return false;  // This will fail
        }, "Check 2 failed: validation error"),
        
        step([]() -> bool {
            std::cout << "Running check 3...\n";
            return true;
        }, "Check 3 failed: connection error")
    );
    
    int failed_idx = runner1.run();
    
    if (failed_idx >= 0) {
        std::cout << "Failed at index: " << failed_idx << "\n\n";
    } else {
        std::cout << "All checks passed!\n\n";
    }
    
    std::cout << "=== Example 2: System startup sequence ===\n";
    
    auto startup = make_function_runner(
        step(initialize_system, "Failed to initialize system"),
        step(connect_to_database, "Failed to connect to database"),
        step(load_configuration, "Failed to load configuration"),
        step(start_server, "Failed to start server")
    );
    
    std::cout << "Total startup steps: " << startup.size() << "\n";
    
    failed_idx = startup.run();
    
    if (failed_idx >= 0) {
        std::cout << "Startup failed at step " << failed_idx << "\n\n";
    } else {
        std::cout << "Startup completed successfully!\n\n";
    }
    
    std::cout << "=== Example 3: Explicit template parameter (if you prefer) ===\n";
    
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
    
    std::cout << "\n=== Example 4: Using failed_step(), error_message(), and rerun() APIs ===\n";
    
    auto diagnostic_runner = make_function_runner(
        step([]() -> bool {
            std::cout << "Step A: Pre-flight check...\n";
            return true;
        }, "Pre-flight check failed"),
        
        step([]() -> bool {
            std::cout << "Step B: Network connection...\n";
            return false;  // This will fail
        }, "Network connection failed"),
        
        step([]() -> bool {
            std::cout << "Step C: Final verification...\n";
            return true;
        }, "Final verification failed")
    );
    
    int result = diagnostic_runner.run();
    
    if (result >= 0) {
        std::cout << "Run failed at step " << result << "\n";
        std::cout << "Failed step index from API: " << diagnostic_runner.failed_step() << "\n";
        
        // Test index-based API
        std::cout << "Error message (by index): " << diagnostic_runner.error_message(result) << "\n";
        
        // Test pointer-based API
        std::cout << "Error message (by pointer): " << diagnostic_runner.error_message(&diagnostic_runner.m_steps[result].func) << "\n";
        
        std::cout << "\nAttempting to rerun the failed step (by index)...\n";
        bool rerun_result = diagnostic_runner.rerun(result);
        std::cout << "Rerun result: " << (rerun_result ? "Success" : "Failed again") << "\n";
        
        std::cout << "\nAttempting to rerun using pointer...\n";
        rerun_result = diagnostic_runner.rerun(&diagnostic_runner.m_steps[result].func);
        std::cout << "Rerun result: " << (rerun_result ? "Success" : "Failed again") << "\n";
    } else {
        std::cout << "All steps completed successfully!\n";
    }
    
    return 0;
}
