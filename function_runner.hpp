#pragma once

#include <cstddef>
#include <utility>
#include <iostream>
#include <functional>

/**
 * @brief A function runner that executes a fixed sequence of functions and tracks failures
 * 
 * This class manages a fixed-size array of functions paired with error messages.
 * When run, it executes each function sequentially until one fails or all succeed.
 * 
 * Functions should return bool where:
 * - true = success, continue to next function
 * - false = failure, stop execution
 * 
 * @tparam N Number of functions to execute
 * 
 * Example usage:
 * @code
 * FunctionRunner<3> runner{{
 *     {[]() { return true; }, "Step 1 failed"},
 *     {[]() { return false; }, "Step 2 failed"},
 *     {[]() { return true; }, "Step 3 failed"}
 * }};
 * 
 * int failed_idx = runner.run();
 * if (failed_idx >= 0) {
 *     std::cout << "Failed at step: " << failed_idx << "\n";
 * }
 * @endcode
 */
template<std::size_t N>
class FunctionRunner {
public:
    /// Type for a function-message pair
    struct Step {
        std::function<bool()> func;
        const char* error_msg;
    };

    /// Array of function steps
    Step m_steps[N];

    /**
     * @brief Run all registered functions sequentially
     * 
     * Executes each function in order. If a function returns false,
     * prints its error message to stderr and stops execution.
     * 
     * @return Index of the first failed function, or -1 if all succeeded
     */
    int run() const {
        for (std::size_t i = 0; i < N; ++i) {
            if (!m_steps[i].func()) {
                std::cerr << "Error: " << m_steps[i].error_msg << "\n";
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    /**
     * @brief Get the number of function steps
     * @return Number of functions in the runner
     */
    static constexpr std::size_t size() noexcept {
        return N;
    }
};

// Deduction guide for C++17
template<typename... Steps>
FunctionRunner(Steps...) -> FunctionRunner<sizeof...(Steps)>;


