#pragma once

#include <cstddef>
#include <iostream>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace function_runner_internal {

// Helper to extract the return type of the first function (from alternating func, msg pairs)
template <typename Func, typename Msg, typename... Rest>
struct first_return_type {
    using type = std::invoke_result_t<Func>;
};

template <typename... Args>
using first_return_type_t = typename first_return_type<Args...>::type;

// Helper to check if all functions (even-indexed args) have the same return type
template <typename Expected, typename... Args>
struct all_return_same_type;

// Base case: no more arguments
template <typename Expected>
struct all_return_same_type<Expected> {
    static constexpr bool value = true;
};

// Base case: only error message remaining (after last function was processed)
template <typename Expected, typename Msg>
struct all_return_same_type<Expected, Msg> {
    static constexpr bool value = true;
};

// Recursive case: check function (even index) and skip message (odd index)
template <typename Expected, typename Func, typename Msg, typename... Rest>
struct all_return_same_type<Expected, Func, Msg, Rest...> {
    static constexpr bool value = std::is_same_v<Expected, std::invoke_result_t<Func>> &&
                                  all_return_same_type<Expected, Rest...>::value;
};

template <typename Expected, typename... Args>
constexpr bool all_return_same_type_v = all_return_same_type<Expected, Args...>::value;

// Helper to check if value indicates failure
// For bool: false is failure
// For other types: non-zero is failure (treating return value as error code)
template <typename T>
bool is_failure(T value) {
    if constexpr (std::is_same_v<T, bool>) {
        return !value;  // false is failure for bool
    } else {
        return value != T{};  // non-zero is failure for other types
    }
}

// Helper to check if all odd-indexed arguments are convertible to string_view
// and all even-indexed arguments are callable
template <typename... Args>
struct validate_alternating_args;

template <typename Func, typename Msg>
struct validate_alternating_args<Func, Msg> {
    static_assert(std::is_invocable_v<Func>,
                  "Functions (even-indexed arguments) must be callable with no arguments");
    static_assert(std::is_convertible_v<Msg, std::string_view>,
                  "Error messages (odd-indexed arguments) must be convertible to std::string_view");
    static constexpr bool value = true;
};

template <typename Func, typename Msg, typename... Rest>
struct validate_alternating_args<Func, Msg, Rest...> {
    static_assert(std::is_invocable_v<Func>,
                  "Functions (even-indexed arguments) must be callable with no arguments");
    static_assert(std::is_convertible_v<Msg, std::string_view>,
                  "Error messages (odd-indexed arguments) must be convertible to std::string_view");
    static constexpr bool value = validate_alternating_args<Rest...>::value;
};

template <typename... Args>
constexpr bool validate_alternating_args_v = validate_alternating_args<Args...>::value;

}  // namespace function_runner_internal

/**
 * @brief A function runner that executes a fixed sequence of functions and tracks failures
 *
 * This class stores each function with its actual type (no type erasure), providing
 * zero-overhead execution with no heap allocations. Each function is stored inline.
 *
 * All functions must return the same type. Success/failure is determined as follows:
 * - For bool: false = failure, true = success
 * - For other types: non-zero = failure (error code), zero = success
 *
 * @tparam Funcs The types of callable objects to execute
 *
 * Example usage:
 * @code
 * // With bool return type
 * auto runner = make_function_runner(
 *     []() { return true; }, "Step 1 failed",
 *     []() { return false; }, "Step 2 failed",
 *     []() { return true; }, "Step 3 failed"
 * );
 *
 * // With int return type (0 = success, non-zero = error code)
 * auto runner2 = make_function_runner(
 *     []() { return 0; }, "Step 1 failed",
 *     []() { return -1; }, "Step 2 failed",  // -1 indicates failure
 *     []() { return 0; }, "Step 3 failed"
 * );
 *
 * int failed_idx = runner.run();
 * if (failed_idx >= 0) {
 *     std::cout << "Failed at step: " << failed_idx << "\n";
 * }
 * @endcode
 */
template <typename... Funcs>
class FunctionRunner {
   public:
    /// The return type of all functions (all must match)
    using return_type = function_runner_internal::first_return_type_t<Funcs..., std::string_view>;

    /// Tuple storing each function with its error message
    std::tuple<std::pair<Funcs, std::string_view>...> m_steps;

    /// Index of the failed step, or -1 if no failure
    mutable int m_failed_step = -1;

    /// Storage for the result of the last executed function
    mutable return_type m_result{};

    /**
     * @brief Run all registered functions sequentially
     *
     * Executes each function in order. If a function returns a failure value,
     * stops execution and records the failed step index.
     * For bool: false is failure. For other types: non-zero is failure.
     *
     * @return Index of the first failed function, or -1 if all succeeded
     */
    int run() const { return run_impl(std::index_sequence_for<Funcs...>{}); }

    /**
     * @brief Get the index of the failed step
     * @return Index of the failed step, or -1 if all succeeded or run() hasn't been called
     */
    int failed_step() const noexcept { return m_failed_step; }

    /**
     * @brief Get the result of the last executed function
     * @return The return value of the last function that was executed
     */
    return_type result() const noexcept { return m_result; }

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
    static constexpr std::size_t size() noexcept { return sizeof...(Funcs); }

   private:
    template <std::size_t... Is>
    int run_impl(std::index_sequence<Is...>) const {
        m_failed_step = -1;
        // Use fold expression with short-circuit evaluation
        // Store each result and check for failure
        (void)((!function_runner_internal::is_failure(m_result = std::get<Is>(m_steps).first()) || (m_failed_step = Is, false)) &&
         ...);
        return m_failed_step;
    }

    template <std::size_t... Is>
    std::string_view error_message_impl(std::size_t index,
                                        std::index_sequence<Is...>) const noexcept {
        std::string_view result = "";
        (void)((Is == index ? (result = std::get<Is>(m_steps).second, true) : false) || ...);
        return result;
    }

    template <std::size_t... Is>
    bool rerun_impl(std::size_t index, std::index_sequence<Is...>) const {
        bool found = false;
        (void)((Is == index ? (m_result = std::get<Is>(m_steps).first(), found = true) : false) || ...);
        return found ? !function_runner_internal::is_failure(m_result) : false;
    }
};

namespace function_runner_internal {

// Helper to extract even-indexed arguments (functions)
template <std::size_t... Is, typename... Args>
auto extract_funcs(std::index_sequence<Is...>, Args&&... args) {
    auto all_args = std::forward_as_tuple(std::forward<Args>(args)...);
    return std::make_tuple(std::get<Is * 2>(std::move(all_args))...);
}

// Helper to extract odd-indexed arguments (error messages)
template <std::size_t... Is, typename... Args>
auto extract_msgs(std::index_sequence<Is...>, Args&&... args) {
    auto all_args = std::forward_as_tuple(std::forward<Args>(args)...);
    return std::make_tuple(std::get<Is * 2 + 1>(std::move(all_args))...);
}

// Helper to create pairs from two tuples
template <typename FuncsTuple, typename MsgsTuple, std::size_t... Is>
auto make_pairs_impl(FuncsTuple&& funcs, MsgsTuple&& msgs, std::index_sequence<Is...>) {
    return std::make_tuple(std::make_pair(std::get<Is>(std::forward<FuncsTuple>(funcs)),
                                          std::get<Is>(std::forward<MsgsTuple>(msgs)))...);
}

template <typename FuncsTuple, typename MsgsTuple>
auto make_pairs(FuncsTuple&& funcs, MsgsTuple&& msgs) {
    constexpr auto N = std::tuple_size_v<std::decay_t<FuncsTuple>>;
    return make_pairs_impl(std::forward<FuncsTuple>(funcs), std::forward<MsgsTuple>(msgs),
                           std::make_index_sequence<N>{});
}

// Helper to construct FunctionRunner from extracted tuples
template <typename FuncsTuple, typename MsgsTuple, std::size_t... Is>
auto make_runner_from_pairs(FuncsTuple&& funcs, MsgsTuple&& msgs, std::index_sequence<Is...>) {
    auto pairs = make_pairs(std::forward<FuncsTuple>(funcs), std::forward<MsgsTuple>(msgs));
    using RunnerType = FunctionRunner<std::decay_t<decltype(std::get<Is>(funcs))>...>;
    return RunnerType{std::move(pairs)};
}

}  // namespace function_runner_internal

/**
 * @brief Helper function to create a FunctionRunner with automatic type deduction
 *
 * This function allows you to create a FunctionRunner that stores each callable
 * with its actual type, avoiding std::function overhead.
 *
 * All functions must return the same type.
 * For bool: false = failure, true = success
 * For other types: non-zero = failure (error code), zero = success
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
template <typename First, typename Second, typename... Rest>
auto make_function_runner(First&& first, Second&& second, Rest&&... rest) {
    static_assert((sizeof...(Rest) + 2) % 2 == 0,
                  "Arguments must come in pairs (function, error_message)");
    static_assert(function_runner_internal::validate_alternating_args_v<First, Second, Rest...>,
                  "Arguments must alternate: function, message, function, message, ...");

    // Validate that all functions return the same type
    using ExpectedReturnType = function_runner_internal::first_return_type_t<First, Second, Rest...>;
    static_assert(
        function_runner_internal::all_return_same_type_v<ExpectedReturnType, First, Second, Rest...>,
        "All functions must return the same type");

    constexpr auto num_pairs = (sizeof...(Rest) + 2) / 2;
    auto indices = std::make_index_sequence<num_pairs>{};

    auto funcs = function_runner_internal::extract_funcs(indices, std::forward<First>(first),
                                                         std::forward<Second>(second),
                                                         std::forward<Rest>(rest)...);
    auto msgs = function_runner_internal::extract_msgs(indices, std::forward<First>(first),
                                                       std::forward<Second>(second),
                                                       std::forward<Rest>(rest)...);

    return function_runner_internal::make_runner_from_pairs(std::move(funcs), std::move(msgs),
                                                            indices);
}
