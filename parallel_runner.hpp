#pragma once

#include <cstddef>
#include <utility>
#include <string_view>
#include <array>
#include <tuple>
#include <type_traits>

namespace parallel_runner_internal {

// Helper to check if all odd-indexed arguments are convertible to string_view
// and all even-indexed arguments are callable
template<typename... Args>
struct validate_alternating_args;

template<typename Func, typename Msg>
struct validate_alternating_args<Func, Msg> {
    static_assert(std::is_invocable_v<Func>,
                  "Functions (even-indexed arguments) must be callable with no arguments");
    static_assert(std::is_convertible_v<Msg, std::string_view>,
                  "Error messages (odd-indexed arguments) must be convertible to std::string_view");
    static constexpr bool value = true;
};

template<typename Func, typename Msg, typename... Rest>
struct validate_alternating_args<Func, Msg, Rest...> {
    static_assert(std::is_invocable_v<Func>,
                  "Functions (even-indexed arguments) must be callable with no arguments");
    static_assert(std::is_convertible_v<Msg, std::string_view>,
                  "Error messages (odd-indexed arguments) must be convertible to std::string_view");
    static constexpr bool value = validate_alternating_args<Rest...>::value;
};

template<typename... Args>
constexpr bool validate_alternating_args_v = validate_alternating_args<Args...>::value;

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

} // namespace parallel_runner_internal

/**
 * @brief A parallel runner that executes all functions and stores their results
 * 
 * Unlike FunctionRunner which stops on first failure, ParallelRunner executes
 * all functions regardless of individual failures and stores all results.
 * 
 * This class stores each function with its actual type (no type erasure), providing
 * zero-overhead execution with no heap allocations.
 * 
 * @tparam Funcs The types of callable objects to execute
 * 
 * Example usage:
 * @code
 * auto runner = make_parallel_runner(
 *     []() { return true; }, "Check 1 failed",
 *     []() { return false; }, "Check 2 failed",
 *     []() { return true; }, "Check 3 failed"
 * );
 * 
 * runner.run();
 * 
 * for (size_t i = 0; i < runner.size(); ++i) {
 *     if (!runner.result(i)) {
 *         std::cout << "Step " << i << " failed: " 
 *                   << runner.error_message(i) << "\n";
 *     }
 * }
 * @endcode
 */
template<typename... Funcs>
class ParallelRunner {
public:
    /// Tuple storing each function with its error message
    std::tuple<std::pair<Funcs, std::string_view>...> m_steps;
    
    /// Array to store results of each function
    mutable std::array<bool, sizeof...(Funcs)> m_results{};
    
    /// Flag indicating whether run() has been called
    mutable bool m_executed = false;

    /**
     * @brief Run all registered functions in parallel (conceptually)
     * 
     * Executes all functions and stores their results regardless of
     * individual failures. This allows checking all validation conditions
     * or running all independent operations.
     */
    void run() const {
        run_impl(std::index_sequence_for<Funcs...>{});
        m_executed = true;
    }

    /**
     * @brief Get the result of a specific step
     * @param index The step index
     * @return true if the step succeeded, false if failed or not yet executed
     */
    bool result(std::size_t index) const noexcept {
        if (index < sizeof...(Funcs) && m_executed) {
            return m_results[index];
        }
        return false;
    }

    /**
     * @brief Get all results as an array
     * @return Array of all execution results
     */
    const std::array<bool, sizeof...(Funcs)>& results() const noexcept {
        return m_results;
    }

    /**
     * @brief Check if all steps succeeded
     * @return true if all steps returned true, false otherwise
     */
    bool all_succeeded() const noexcept {
        if (!m_executed) return false;
        for (std::size_t i = 0; i < sizeof...(Funcs); ++i) {
            if (!m_results[i]) return false;
        }
        return true;
    }

    /**
     * @brief Check if any step succeeded
     * @return true if at least one step returned true, false otherwise
     */
    bool any_succeeded() const noexcept {
        if (!m_executed) return false;
        for (std::size_t i = 0; i < sizeof...(Funcs); ++i) {
            if (m_results[i]) return true;
        }
        return false;
    }

    /**
     * @brief Count how many steps succeeded
     * @return Number of successful steps
     */
    std::size_t success_count() const noexcept {
        if (!m_executed) return 0;
        std::size_t count = 0;
        for (std::size_t i = 0; i < sizeof...(Funcs); ++i) {
            if (m_results[i]) ++count;
        }
        return count;
    }

    /**
     * @brief Count how many steps failed
     * @return Number of failed steps
     */
    std::size_t failure_count() const noexcept {
        return m_executed ? (sizeof...(Funcs) - success_count()) : 0;
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
     * @brief Rerun all steps that failed
     * @return Number of steps that succeeded on rerun
     */
    std::size_t rerun_failed() const {
        if (!m_executed) return 0;
        
        std::size_t success_count = 0;
        for (std::size_t i = 0; i < sizeof...(Funcs); ++i) {
            if (!m_results[i]) {
                if (rerun(i)) {
                    ++success_count;
                }
            }
        }
        return success_count;
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
    void run_impl(std::index_sequence<Is...>) const {
        ((m_results[Is] = std::get<Is>(m_steps).first()), ...);
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
        (void)((Is == index ? (m_results[index] = std::get<Is>(m_steps).first(), result = m_results[index], true) : false) || ...);
        return result;
    }
};

namespace parallel_runner_internal {

// Helper to construct ParallelRunner from extracted tuples
template<typename FuncsTuple, typename MsgsTuple, std::size_t... Is>
auto make_runner_from_pairs(FuncsTuple&& funcs, MsgsTuple&& msgs, std::index_sequence<Is...>) {
    auto pairs = make_pairs(std::forward<FuncsTuple>(funcs), std::forward<MsgsTuple>(msgs));
    using RunnerType = ParallelRunner<std::decay_t<decltype(std::get<Is>(funcs))>...>;
    return RunnerType{std::move(pairs)};
}

} // namespace parallel_runner_internal

/**
 * @brief Helper function to create a ParallelRunner with automatic type deduction
 * 
 * This function allows you to create a ParallelRunner that stores each callable
 * with its actual type, avoiding std::function overhead.
 * 
 * Example usage with alternating arguments:
 * @code
 * auto runner = make_parallel_runner(
 *     []() { return true; }, "Step 1 failed",
 *     []() { return false; }, "Step 2 failed"
 * );
 * @endcode
 */
// Overload for alternating arguments (func, msg, func, msg, ...)
template<typename First, typename Second, typename... Rest>
auto make_parallel_runner(First&& first, Second&& second, Rest&&... rest) {
    static_assert((sizeof...(Rest) + 2) % 2 == 0, "Arguments must come in pairs (function, error_message)");
    static_assert(parallel_runner_internal::validate_alternating_args_v<First, Second, Rest...>,
                  "Arguments must alternate: function, message, function, message, ...");
    
    constexpr auto num_pairs = (sizeof...(Rest) + 2) / 2;
    auto indices = std::make_index_sequence<num_pairs>{};
    
    auto funcs = parallel_runner_internal::extract_funcs(indices, std::forward<First>(first), std::forward<Second>(second), std::forward<Rest>(rest)...);
    auto msgs = parallel_runner_internal::extract_msgs(indices, std::forward<First>(first), std::forward<Second>(second), std::forward<Rest>(rest)...);
    
    return parallel_runner_internal::make_runner_from_pairs(std::move(funcs), std::move(msgs), indices);
}
