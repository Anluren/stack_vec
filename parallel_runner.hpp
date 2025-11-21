#pragma once

#include <cstddef>
#include <utility>
#include <functional>
#include <string_view>
#include <array>

namespace parallel_runner_internal {

/**
 * @brief Helper struct for creating ParallelRunner steps
 */
template<typename Func>
struct StepWrapper {
    Func func;
    std::string_view error_msg;
};

} // namespace parallel_runner_internal

template<typename Func>
parallel_runner_internal::StepWrapper<Func> parallel_step(Func&& f, std::string_view msg) {
    return parallel_runner_internal::StepWrapper<Func>{std::forward<Func>(f), msg};
}

/**
 * @brief A parallel runner that executes all functions and stores their results
 * 
 * Unlike FunctionRunner which stops on first failure, ParallelRunner executes
 * all functions regardless of individual failures and stores all results.
 * 
 * @tparam N Number of functions to execute
 * 
 * Example usage:
 * @code
 * ParallelRunner<3> runner{{
 *     {[]() { return true; }, "Check 1 failed"},
 *     {[]() { return false; }, "Check 2 failed"},
 *     {[]() { return true; }, "Check 3 failed"}
 * }};
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
template<std::size_t N>
class ParallelRunner {
public:
    /// Type for a function-message pair
    struct Step {
        std::function<bool()> func;
        std::string_view error_msg;
    };

    /// Array of function steps
    Step m_steps[N];
    
    /// Array to store results of each function
    mutable std::array<bool, N> m_results{};
    
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
        for (std::size_t i = 0; i < N; ++i) {
            m_results[i] = m_steps[i].func();
        }
        m_executed = true;
    }

    /**
     * @brief Get the result of a specific step
     * @param index The step index
     * @return true if the step succeeded, false if failed or not yet executed
     */
    bool result(std::size_t index) const noexcept {
        if (index < N && m_executed) {
            return m_results[index];
        }
        return false;
    }

    /**
     * @brief Get all results as an array
     * @return Array of all execution results
     */
    const std::array<bool, N>& results() const noexcept {
        return m_results;
    }

    /**
     * @brief Check if all steps succeeded
     * @return true if all steps returned true, false otherwise
     */
    bool all_succeeded() const noexcept {
        if (!m_executed) return false;
        for (std::size_t i = 0; i < N; ++i) {
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
        for (std::size_t i = 0; i < N; ++i) {
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
        for (std::size_t i = 0; i < N; ++i) {
            if (m_results[i]) ++count;
        }
        return count;
    }

    /**
     * @brief Count how many steps failed
     * @return Number of failed steps
     */
    std::size_t failure_count() const noexcept {
        return m_executed ? (N - success_count()) : 0;
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
            m_results[index] = m_steps[index].func();
            return m_results[index];
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
                m_results[i] = m_steps[i].func();
                return m_results[i];
            }
        }
        return false;
    }

    /**
     * @brief Rerun all steps that failed
     * @return Number of steps that succeeded on rerun
     */
    std::size_t rerun_failed() const {
        if (!m_executed) return 0;
        
        std::size_t success_count = 0;
        for (std::size_t i = 0; i < N; ++i) {
            if (!m_results[i]) {
                m_results[i] = m_steps[i].func();
                if (m_results[i]) {
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
        return N;
    }
};

/**
 * @brief Helper function to create a ParallelRunner with automatic size deduction
 * 
 * This function allows you to create a ParallelRunner without explicitly specifying
 * the template parameter N. Use the parallel_step() helper function for clean syntax.
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
auto make_parallel_runner(parallel_runner_internal::StepWrapper<Funcs>... steps) {
    constexpr std::size_t N = sizeof...(Funcs);
    return ParallelRunner<N>{{typename ParallelRunner<N>::Step{steps.func, steps.error_msg}...}};
}

