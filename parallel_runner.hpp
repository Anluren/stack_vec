#pragma once

#include <cstddef>
#include <utility>
#include <string_view>
#include <array>
#include <tuple>
#include <type_traits>

namespace parallel_runner_internal {

/**
 * @brief Helper struct for creating ParallelRunner steps
 */
template<typename Func>
struct StepWrapper {
    Func func;
    std::string_view error_msg;
};

// Deduction guide for CTAD
template<typename Func>
StepWrapper(Func, std::string_view) -> StepWrapper<std::decay_t<Func>>;

} // namespace parallel_runner_internal

template<typename Func>
auto parallel_step(Func&& f, std::string_view msg) {
    return parallel_runner_internal::StepWrapper{std::forward<Func>(f), msg};
}

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
 *     parallel_step([]() { return true; }, "Check 1 failed"),
 *     parallel_step([]() { return false; }, "Check 2 failed"),
 *     parallel_step([]() { return true; }, "Check 3 failed")
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

/**
 * @brief Helper function to create a ParallelRunner with automatic type deduction
 * 
 * This function allows you to create a ParallelRunner that stores each callable
 * with its actual type, avoiding std::function overhead. Use the parallel_step() 
 * helper function for clean syntax.
 * 
 * Example usage:
 * @code
 * auto runner = make_parallel_runner(
 *     parallel_step([]() { return true; }, "Step 1 failed"),
 *     parallel_step([]() { return false; }, "Step 2 failed"),
 *     parallel_step([]() { return true; }, "Step 3 failed")
 * );
 * runner.run();
 * @endcode
 */
template<typename... Funcs>
auto make_parallel_runner(parallel_runner_internal::StepWrapper<Funcs>&&... steps) {
    return ParallelRunner<Funcs...>{std::tuple<std::pair<Funcs, std::string_view>...>{std::make_pair(std::move(steps.func), steps.error_msg)...}};
}
