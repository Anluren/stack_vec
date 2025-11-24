#pragma once

#include <cstddef>
#include <utility>
#include <iostream>
#include <string_view>
#include <tuple>
#include <type_traits>

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

// Deduction guide for CTAD
template<typename Func>
StepWrapper(Func, std::string_view) -> StepWrapper<std::decay_t<Func>>;

// Type trait to detect if a type is a StepWrapper
template<typename T>
struct is_step_wrapper : std::false_type {};

template<typename Func>
struct is_step_wrapper<StepWrapper<Func>> : std::true_type {};

template<typename T>
constexpr bool is_step_wrapper_v = is_step_wrapper<std::decay_t<T>>::value;

} // namespace function_runner_internal

/**
 * @brief A function runner that executes a fixed sequence of functions and tracks failures
 * 
 * This class stores each function with its actual type (no type erasure), providing
 * zero-overhead execution with no heap allocations. Each function is stored inline.
 * 
 * Functions should return bool where:
 * - true = success, continue to next function
 * - false = failure, stop execution
 * 
 * @tparam Funcs The types of callable objects to execute
 * 
 * Example usage:
 * @code
 * auto runner = make_function_runner(
 *     []() { return true; }, "Step 1 failed",
 *     []() { return false; }, "Step 2 failed",
 *     []() { return true; }, "Step 3 failed"
 * );
 * 
 * int failed_idx = runner.run();
 * if (failed_idx >= 0) {
 *     std::cout << "Failed at step: " << failed_idx << "\n";
 * }
 * @endcode
 */
template<typename... Funcs>
class FunctionRunner {
public:
    /// Tuple storing each function with its error message
    std::tuple<std::pair<Funcs, std::string_view>...> m_steps;
    
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
        return run_impl(std::index_sequence_for<Funcs...>{});
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
        return error_message_impl(index, std::index_sequence_for<Funcs...>{});
    }

    /**
     * @brief Rerun a specific step by index
     * @param index The step index to rerun
     * @return true if the step succeeded, false if it failed or index out of bounds
     */
    bool rerun(std::size_t index) const {
        return rerun_impl(index, std::index_sequence_for<Funcs...>{});
    }

    /**
     * @brief Get the number of function steps
     * @return Number of functions in the runner
     */
    static constexpr std::size_t size() noexcept {
        return sizeof...(Funcs);
    }

private:
    template<std::size_t... Is>
    int run_impl(std::index_sequence<Is...>) const {
        int result = -1;
        // Use fold expression with short-circuit evaluation
        ((std::get<Is>(m_steps).first() || (m_failed_step = result = Is, false)) && ...);
        if (result == -1) {
            m_failed_step = -1;
        }
        return result;
    }

    template<std::size_t... Is>
    std::string_view error_message_impl(std::size_t index, std::index_sequence<Is...>) const noexcept {
        std::string_view result = "";
        (void)((Is == index ? (result = std::get<Is>(m_steps).second, true) : false) || ...);
        return result;
    }

    template<std::size_t... Is>
    bool rerun_impl(std::size_t index, std::index_sequence<Is...>) const {
        bool result = false;
        (void)((Is == index ? (result = std::get<Is>(m_steps).first(), true) : false) || ...);
        return result;
    }
};

namespace function_runner_internal {

// Helper to extract even-indexed arguments (functions)
template<std::size_t... Is, typename... Args>
auto extract_funcs(std::index_sequence<Is...>, Args&&... args) {
    auto all_args = std::forward_as_tuple(std::forward<Args>(args)...);
    return std::make_tuple(std::get<Is * 2>(std::move(all_args))...);
}

// Helper to extract odd-indexed arguments (error messages)
template<std::size_t... Is, typename... Args>
auto extract_msgs(std::index_sequence<Is...>, Args&&... args) {
    auto all_args = std::forward_as_tuple(std::forward<Args>(args)...);
    return std::make_tuple(std::get<Is * 2 + 1>(std::move(all_args))...);
}

// Helper to create pairs from two tuples
template<typename FuncsTuple, typename MsgsTuple, std::size_t... Is>
auto make_pairs_impl(FuncsTuple&& funcs, MsgsTuple&& msgs, std::index_sequence<Is...>) {
    return std::make_tuple(std::make_pair(std::get<Is>(std::forward<FuncsTuple>(funcs)), 
                                          std::get<Is>(std::forward<MsgsTuple>(msgs)))...);
}

template<typename FuncsTuple, typename MsgsTuple>
auto make_pairs(FuncsTuple&& funcs, MsgsTuple&& msgs) {
    constexpr auto N = std::tuple_size_v<std::decay_t<FuncsTuple>>;
    return make_pairs_impl(std::forward<FuncsTuple>(funcs), 
                          std::forward<MsgsTuple>(msgs), 
                          std::make_index_sequence<N>{});
}

// Helper to construct FunctionRunner from extracted tuples
template<typename FuncsTuple, typename MsgsTuple, std::size_t... Is>
auto make_runner_from_pairs(FuncsTuple&& funcs, MsgsTuple&& msgs, std::index_sequence<Is...>) {
    auto pairs = make_pairs(std::forward<FuncsTuple>(funcs), std::forward<MsgsTuple>(msgs));
    using RunnerType = FunctionRunner<std::decay_t<decltype(std::get<Is>(funcs))>...>;
    return RunnerType{std::move(pairs)};
}

} // namespace function_runner_internal

/**
 * @brief Helper function to create a FunctionRunner with automatic type deduction
 * 
 * This function allows you to create a FunctionRunner that stores each callable
 * with its actual type, avoiding std::function overhead.
 * 
 * Example usage with alternating arguments:
 * @code
 * auto runner = make_function_runner(
 *     []() { return true; }, "Step 1 failed",
 *     []() { return false; }, "Step 2 failed"
 * );
 * @endcode
 */
// Overload for alternating arguments (func, msg, func, msg, ...)
template<typename First, typename Second, typename... Rest>
auto make_function_runner(First&& first, Second&& second, Rest&&... rest) {
    static_assert((sizeof...(Rest) + 2) % 2 == 0, "Arguments must come in pairs (function, error_message)");
    
    constexpr auto num_pairs = (sizeof...(Rest) + 2) / 2;
    auto indices = std::make_index_sequence<num_pairs>{};
    
    auto funcs = function_runner_internal::extract_funcs(indices, std::forward<First>(first), std::forward<Second>(second), std::forward<Rest>(rest)...);
    auto msgs = function_runner_internal::extract_msgs(indices, std::forward<First>(first), std::forward<Second>(second), std::forward<Rest>(rest)...);
    
    return function_runner_internal::make_runner_from_pairs(std::move(funcs), std::move(msgs), indices);
}


