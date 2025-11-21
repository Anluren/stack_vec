#include "function_runner.hpp"
#include <iostream>
#include <functional>

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

// Functions with arguments for std::bind example
bool connect_to_server(const std::string& host, int port) {
    std::cout << "Connecting to " << host << ":" << port << "\n";
    return true;
}

bool validate_range(int value, int min, int max) {
    std::cout << "Validating " << value << " in range [" << min << ", " << max << "]\n";
    return value >= min && value <= max;
}

bool authenticate_user(const std::string& username, const std::string& password) {
    std::cout << "Authenticating user: " << username << "\n";
    return username == "admin" && password == "secret";
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
   
    std::cout << "size of runner1: " << sizeof(runner1) << "\n";
    
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
    
    std::cout << "size of startup: " << sizeof(startup) << "\n";
    std::cout << "Total startup steps: " << startup.size() << "\n";
    
    failed_idx = startup.run();
    
    if (failed_idx >= 0) {
        std::cout << "Startup failed at step " << failed_idx << "\n\n";
    } else {
        std::cout << "Startup completed successfully!\n\n";
    }
    
    std::cout << "=== Example 3: Multiple independent steps ===\n";
    
    auto tasks = make_function_runner(
        step([]() { std::cout << "Task 1 complete\n"; return true; }, "Task 1 failed"),
        step([]() { std::cout << "Task 2 complete\n"; return true; }, "Task 2 failed"),
        step([]() { std::cout << "Task 3 complete\n"; return true; }, "Task 3 failed")
    );
    
    failed_idx = tasks.run();
   
    std::cout << "size of tasks: " << sizeof(tasks) << "\n";
    
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
        
        std::cout << "\nAttempting to rerun the failed step (by index)...\n";
        bool rerun_result = diagnostic_runner.rerun(result);
        std::cout << "Rerun result: " << (rerun_result ? "Success" : "Failed again") << "\n";
        
        std::cout << "\nAttempting second rerun...\n";
        rerun_result = diagnostic_runner.rerun(result);
        std::cout << "Rerun result: " << (rerun_result ? "Success" : "Failed again") << "\n";
    } else {
        std::cout << "All steps completed successfully!\n";
    }
    
    std::cout << "\n=== Example 5: Using std::bind with functions that have arguments ===\n";
    
    auto bind_runner = make_function_runner(
        step(std::bind(connect_to_server, "localhost", 8080), 
             "Server connection failed"),
        
        step(std::bind(validate_range, 42, 0, 100), 
             "Range validation failed"),
        
        step(std::bind(authenticate_user, "admin", "secret"), 
             "Authentication failed")
    );
    
    result = bind_runner.run();
    
    if (result >= 0) {
        std::cout << "Failed at step " << result << ": " 
                  << bind_runner.error_message(result) << "\n";
    } else {
        std::cout << "All steps with std::bind succeeded!\n";
    }
    
    std::cout << "\n=== Example 6: Using lambdas with captures (alternative to std::bind) ===\n";
    
    std::string host = "192.168.1.100";
    int port = 3000;
    int value = 75;
    
    auto lambda_runner = make_function_runner(
        step([&](){ return connect_to_server(host, port); }, 
             "Server connection failed"),
        
        step([&](){ return validate_range(value, 0, 100); }, 
             "Range validation failed"),
        
        step([](){ return authenticate_user("admin", "secret"); }, 
             "Authentication failed")
    );
    
    result = lambda_runner.run();
    
    if (result >= 0) {
        std::cout << "Failed at step " << result << ": " 
                  << lambda_runner.error_message(result) << "\n";
    } else {
        std::cout << "All steps with lambda captures succeeded!\n";
    }
    
    return 0;
}
