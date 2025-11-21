#pragma once

#include <cstddef>
#include <utility>
#include <iostream>
#include <functional>
#include <string_view>

namespace function_runner_internal {

/**
 * @brief Helper struct for creating FunctionRunner steps
 * 
 * This allows for cleaner syntax when using make_function_runner:
 * @code
 * auto runner = make_function_runner(
 *     step([]() { return true; }, "Step 1 failed"),
 *     step([]() { return false; }, "Step 2 failed")
 * );
 * @endcode
 */
template<typename Func>
struct StepWrapper {
    Func func;
    std::string_view error_msg;
};

} // namespace function_runner_internal

template<typename Func>
function_runner_internal::StepWrapper<Func> step(Func&& f, std::string_view msg) {
    return function_runner_internal::StepWrapper<Func>{std::forward<Func>(f), msg};
}

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
        std::string_view error_msg;
    };

    /// Array of function steps
    Step m_steps[N];
    
    /// Index of the failed step, or -1 if no failure
    mutable int m_failed_step = -1;

    /**
     * @brief Run all registered functions sequentially
     * 
     * Executes each function in order. If a function returns false,
     * stops execution and records the failed step index.
     * 
     * @return Index of the first failed function, or -1 if all succeeded
     */
    int run() const {
        for (std::size_t i = 0; i < N; ++i) {
            if (!m_steps[i].func()) {
                m_failed_step = static_cast<int>(i);
                return m_failed_step;
            }
        }
        m_failed_step = -1;
        return -1;
    }

    /**
     * @brief Get the index of the failed step
     * @return Index of the failed step, or -1 if all succeeded or run() hasn't been called
     */
    int failed_step() const noexcept {
        return m_failed_step;
    }

    /**
     * @brief Get the error message for a specific step by index
     * @param index The step index
     * @return The error message for the given step, or empty string if out of bounds
     */
    std::string_view error_message(std::size_t index) const noexcept {
        if (index < N) {
            return m_steps[index].error_msg;
        }
        return "";
    }

    /**
     * @brief Get the error message for a specific step by function pointer
     * @param func Pointer to the std::function to look up
     * @return The error message for the given step, or empty string if not found
     */
    std::string_view error_message(const std::function<bool()>* func) const noexcept {
        for (std::size_t i = 0; i < N; ++i) {
            if (&m_steps[i].func == func) {
                return m_steps[i].error_msg;
            }
        }
        return "";
    }

    /**
     * @brief Rerun a specific step by index
     * @param index The step index to rerun
     * @return true if the step succeeded, false if it failed or index out of bounds
     */
    bool rerun(std::size_t index) const {
        if (index < N) {
            return m_steps[index].func();
        }
        return false;
    }

    /**
     * @brief Rerun a specific step by function pointer
     * @param func Pointer to the std::function to rerun
     * @return true if the step succeeded, false if it failed or function not found
     */
    bool rerun(const std::function<bool()>* func) const {
        for (std::size_t i = 0; i < N; ++i) {
            if (&m_steps[i].func == func) {
                return m_steps[i].func();
            }
        }
        return false;
    }

    /**
     * @brief Get the number of function steps
     * @return Number of functions in the runner
     */
    static constexpr std::size_t size() noexcept {
        return N;
    }
};

/**
 * @brief Helper function to create a FunctionRunner with automatic size deduction
 * 
 * This function allows you to create a FunctionRunner without explicitly specifying
 * the template parameter N. Use the step() helper function for clean syntax.
 * 
 * Example usage:
 * @code
 * auto runner = make_function_runner(
 *     step([]() { return true; }, "Step 1 failed"),
 *     step([]() { return false; }, "Step 2 failed"),
 *     step([]() { return true; }, "Step 3 failed")
 * );
 * runner.run();
 * @endcode
 */
template<typename... Funcs>
auto make_function_runner(function_runner_internal::StepWrapper<Funcs>... steps) {
    constexpr std::size_t N = sizeof...(Funcs);
    return FunctionRunner<N>{{typename FunctionRunner<N>::Step{steps.func, steps.error_msg}...}};
}


