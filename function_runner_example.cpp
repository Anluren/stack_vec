#include <functional>
#include <iostream>

#include "function_runner.hpp"

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
        []() -> bool {
            std::cout << "Running check 1...\n";
            return true;
        },
        "Check 1 failed: initialization error",

        []() -> bool {
            std::cout << "Running check 2...\n";
            return false;  // This will fail
        },
        "Check 2 failed: validation error",

        []() -> bool {
            std::cout << "Running check 3...\n";
            return true;
        },
        "Check 3 failed: connection error");

    int failed_idx = runner1.run();

    if (failed_idx >= 0) {
        std::cout << "Failed at index: " << failed_idx << "\n\n";
    } else {
        std::cout << "All checks passed!\n\n";
    }

    std::cout << "=== Example 2: System startup sequence ===\n";

    auto startup = make_function_runner(initialize_system, "Failed to initialize system",
                                        connect_to_database, "Failed to connect to database",
                                        load_configuration, "Failed to load configuration",
                                        start_server, "Failed to start server");

    std::cout << "Total startup steps: " << startup.size() << "\n";

    failed_idx = startup.run();

    if (failed_idx >= 0) {
        std::cout << "Startup failed at step " << failed_idx << "\n\n";
    } else {
        std::cout << "Startup completed successfully!\n\n";
    }

    std::cout << "=== Example 3: Multiple independent steps ===\n";

    auto tasks = make_function_runner(
        []() {
            std::cout << "Task 1 complete\n";
            return true;
        },
        "Task 1 failed",
        []() {
            std::cout << "Task 2 complete\n";
            return true;
        },
        "Task 2 failed",
        []() {
            std::cout << "Task 3 complete\n";
            return true;
        },
        "Task 3 failed");

    failed_idx = tasks.run();

    if (failed_idx >= 0) {
        std::cout << "Failed at index: " << failed_idx << "\n";
    } else {
        std::cout << "All tasks completed successfully!\n";
    }

    std::cout << "\n=== Example 4: Using failed_step(), error_message(), and rerun() APIs ===\n";

    auto diagnostic_runner = make_function_runner(
        []() -> bool {
            std::cout << "Step A: Pre-flight check...\n";
            return true;
        },
        "Pre-flight check failed",

        []() -> bool {
            std::cout << "Step B: Network connection...\n";
            return false;  // This will fail
        },
        "Network connection failed",

        []() -> bool {
            std::cout << "Step C: Final verification...\n";
            return true;
        },
        "Final verification failed");

    int result = diagnostic_runner.run();

    if (result >= 0) {
        std::cout << "Run failed at step " << result << "\n";
        std::cout << "Failed step index from API: " << diagnostic_runner.failed_step() << "\n";

        // Test index-based API
        std::cout << "Error message (by index): " << diagnostic_runner.error_message(result)
                  << "\n";

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
        std::bind(connect_to_server, "localhost", 8080), "Server connection failed",

        std::bind(validate_range, 42, 0, 100), "Range validation failed",

        std::bind(authenticate_user, "admin", "secret"), "Authentication failed");

    result = bind_runner.run();

    if (result >= 0) {
        std::cout << "Failed at step " << result << ": " << bind_runner.error_message(result)
                  << "\n";
    } else {
        std::cout << "All steps with std::bind succeeded!\n";
    }

    std::cout << "\n=== Example 6: Using lambdas with captures (alternative to std::bind) ===\n";

    std::string host = "192.168.1.100";
    int port = 3000;
    int value = 75;

    auto lambda_runner = make_function_runner(
        [&]() { return connect_to_server(host, port); }, "Server connection failed",

        [&]() { return validate_range(value, 0, 100); }, "Range validation failed",

        []() { return authenticate_user("admin", "secret"); }, "Authentication failed");

    result = lambda_runner.run();

    if (result >= 0) {
        std::cout << "Failed at step " << result << ": " << lambda_runner.error_message(result)
                  << "\n";
    } else {
        std::cout << "All steps with lambda captures succeeded!\n";
    }

    std::cout << "\n=== Example 7: Clean syntax demonstration ===\n";

    auto direct_runner = make_function_runner(
        []() {
            std::cout << "Direct step 1\n";
            return true;
        },
        "Direct step 1 failed",

        []() {
            std::cout << "Direct step 2\n";
            return false;
        },
        "Direct step 2 failed",

        []() {
            std::cout << "Direct step 3\n";
            return true;
        },
        "Direct step 3 failed");

    result = direct_runner.run();

    if (result >= 0) {
        std::cout << "Failed at step " << result << ": " << direct_runner.error_message(result)
                  << "\n";
    } else {
        std::cout << "All direct steps succeeded!\n";
    }

    std::cout << "\n=== Example 8: Functions returning errno-style error codes ===\n";

    // Functions that return int error codes (0 = success, non-zero = error code)
    auto errno_runner = make_function_runner(
        []() -> int {
            std::cout << "Opening file...\n";
            return 0;  // Success
        },
        "Failed to open file",

        []() -> int {
            std::cout << "Reading data...\n";
            return 5;  // Error code 5 (e.g., EIO - I/O error)
        },
        "Failed to read data",

        []() -> int {
            std::cout << "Processing data...\n";
            return 0;  // Success
        },
        "Failed to process data");

    result = errno_runner.run();

    if (result >= 0) {
        int error_code = errno_runner.result();
        std::cout << "Failed at step " << result << "\n";
        std::cout << "Error code: " << error_code << "\n";
        std::cout << "Error message: " << errno_runner.error_message(result) << "\n";
    } else {
        std::cout << "All operations succeeded!\n";
    }

    std::cout << "\n=== Size Summary ===\n";
    std::cout << "runner1 (3 lambdas):           " << sizeof(runner1) << " bytes\n";
    std::cout << "startup (4 functions):         " << sizeof(startup) << " bytes\n";
    std::cout << "tasks (3 lambdas):             " << sizeof(tasks) << " bytes\n";
    std::cout << "diagnostic_runner (3 lambdas): " << sizeof(diagnostic_runner) << " bytes\n";
    std::cout << "bind_runner (3 std::bind):     " << sizeof(bind_runner) << " bytes\n";
    std::cout << "lambda_runner (3 captures):    " << sizeof(lambda_runner) << " bytes\n";
    std::cout << "direct_runner (3 lambdas):     " << sizeof(direct_runner) << " bytes\n";
    std::cout << "errno_runner (3 lambdas):      " << sizeof(errno_runner) << " bytes\n";
    
    std::cout << "\n=== Size Breakdown ===\n";
    std::cout << "Each runner stores:\n";
    std::cout << "  - std::tuple of (function, string_view) pairs\n";
    std::cout << "  - 1 int for m_failed_step (4 bytes)\n";
    std::cout << "  - Result storage of return_type (4 bytes for int/bool)\n";
    std::cout << "  - Each std::string_view is 16 bytes (pointer + size)\n";
    std::cout << "\nCalculation examples:\n";
    std::cout << "  Simple lambda (no captures):     ~1 byte (empty class)\n";
    std::cout << "  Function pointer:                 8 bytes\n";
    std::cout << "  Lambda with captures:             depends on capture size\n";
    std::cout << "  std::bind object:                 ~32 bytes (stores function + bound args)\n";
    std::cout << "\nFormula: sizeof(tuple<pair<Func, string_view>...>) + sizeof(int)\n";
    std::cout << "  runner1: tuple<3 x (1 + 16)> + 4 = 51 + padding → 80 bytes\n";
    std::cout << "  startup: tuple<4 x (8 + 16)> + 4 = 96 + padding → 104 bytes\n";
    std::cout << "  bind_runner: tuple<3 x (32 + 16)> + 4 = 144 + padding → 128 bytes\n";
    
    std::cout << "\nNote: All storage is inline, no heap allocations!\n";

    return 0;
}
