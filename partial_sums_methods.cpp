#include <array>
#include <iostream>
#include <utility>

// Method 1: Current approach - loop-based
template <typename T, T... Is>
constexpr auto partial_sums_loop(std::integer_sequence<T, Is...>) {
    constexpr std::array<T, sizeof...(Is)> values = {Is...};
    std::array<T, sizeof...(Is)> result{};
    result[0] = 0;
    T sum = 0;
    for (std::size_t i = 1; i < sizeof...(Is); ++i) {
        sum += values[i-1];
        result[i] = sum;
    }
    return result;
}

// Method 2: Recursive helper with index_sequence
template <typename T, std::size_t N, std::size_t... Is>
constexpr auto partial_sums_impl(const std::array<T, N>& values, 
                                  std::index_sequence<Is...>) {
    // For each index, sum all elements before it
    return std::array<T, N>{
        (Is == 0 ? T{} : [&values]() {
            T sum{};
            for (std::size_t j = 0; j < Is; ++j) {
                sum += values[j];
            }
            return sum;
        }())...
    };
}

template <typename T, T... Is>
constexpr auto partial_sums_recursive(std::integer_sequence<T, Is...>) {
    constexpr std::array<T, sizeof...(Is)> values = {Is...};
    return partial_sums_impl(values, std::make_index_sequence<sizeof...(Is)>{});
}

// Method 3: Using fold expression with index manipulation
template <typename T, std::size_t... Is>
constexpr auto partial_sums_fold_impl(const std::array<T, sizeof...(Is)>& values,
                                       std::index_sequence<Is...>) {
    return std::array<T, sizeof...(Is)>{
        ((Is == 0) ? T{} : 
         [&values, end = Is]() {
             T sum{};
             for (std::size_t i = 0; i < end; ++i) {
                 sum += values[i];
             }
             return sum;
         }())...
    };
}

template <typename T, T... Is>
constexpr auto partial_sums_fold(std::integer_sequence<T, Is...>) {
    constexpr std::array<T, sizeof...(Is)> values = {Is...};
    return partial_sums_fold_impl(values, std::make_index_sequence<sizeof...(Is)>{});
}

// Method 4: Single-pass accumulation (most efficient)
template <typename T, T... Is>
constexpr auto partial_sums_accumulate(std::integer_sequence<T, Is...>) {
    constexpr std::array<T, sizeof...(Is)> values = {Is...};
    std::array<T, sizeof...(Is)> result{};
    T sum{};
    for (std::size_t i = 0; i < sizeof...(Is); ++i) {
        result[i] = sum;
        sum += values[i];
    }
    return result;
}

// Method 5: Using tuple and structured binding (C++17)
template <std::size_t I, typename T, std::size_t N>
constexpr T sum_up_to(const std::array<T, N>& arr) {
    T sum{};
    for (std::size_t i = 0; i < I; ++i) {
        sum += arr[i];
    }
    return sum;
}

template <typename T, std::size_t... Is>
constexpr auto partial_sums_indexed(const std::array<T, sizeof...(Is)>& values,
                                     std::index_sequence<Is...>) {
    return std::array<T, sizeof...(Is)>{sum_up_to<Is>(values)...};
}

template <typename T, T... Is>
constexpr auto partial_sums_template(std::integer_sequence<T, Is...>) {
    constexpr std::array<T, sizeof...(Is)> values = {Is...};
    return partial_sums_indexed(values, std::make_index_sequence<sizeof...(Is)>{});
}

// Method 6: Stateful lambda (most elegant for single-pass)
template <typename T, T... Is>
constexpr auto partial_sums_lambda(std::integer_sequence<T, Is...>) {
    constexpr std::array<T, sizeof...(Is)> values = {Is...};
    std::array<T, sizeof...(Is)> result{};
    T sum{};
    std::size_t idx = 0;
    ((result[idx++] = sum, sum += Is), ...);
    return result;
}

int main() {
    using TestSeq = std::integer_sequence<int, 1, 2, 3, 4, 5>;
    
    std::cout << "Input: [1, 2, 3, 4, 5]\n";
    std::cout << "Expected: [0, 1, 3, 6, 10]\n\n";
    
    std::cout << "=== Method 1: Loop-based (current) ===\n";
    {
        constexpr auto result = partial_sums_loop(TestSeq{});
        static_assert(result[0] == 0 && result[1] == 1 && result[2] == 3 
                   && result[3] == 6 && result[4] == 10);
        std::cout << "Result: [";
        for (std::size_t i = 0; i < result.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result[i];
        }
        std::cout << "]\n";
    }
    
    std::cout << "\n=== Method 2: Recursive with lambda ===\n";
    {
        constexpr auto result = partial_sums_recursive(TestSeq{});
        static_assert(result[0] == 0 && result[1] == 1 && result[2] == 3 
                   && result[3] == 6 && result[4] == 10);
        std::cout << "Result: [";
        for (std::size_t i = 0; i < result.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result[i];
        }
        std::cout << "]\n";
    }
    
    std::cout << "\n=== Method 3: Fold expression with index ===\n";
    {
        constexpr auto result = partial_sums_fold(TestSeq{});
        static_assert(result[0] == 0 && result[1] == 1 && result[2] == 3 
                   && result[3] == 6 && result[4] == 10);
        std::cout << "Result: [";
        for (std::size_t i = 0; i < result.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result[i];
        }
        std::cout << "]\n";
    }
    
    std::cout << "\n=== Method 4: Single-pass accumulation (BEST) ===\n";
    {
        constexpr auto result = partial_sums_accumulate(TestSeq{});
        static_assert(result[0] == 0 && result[1] == 1 && result[2] == 3 
                   && result[3] == 6 && result[4] == 10);
        std::cout << "Result: [";
        for (std::size_t i = 0; i < result.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result[i];
        }
        std::cout << "]\n";
        std::cout << "✓ Most efficient: O(n) time, single pass\n";
        std::cout << "✓ Clear and simple logic\n";
    }
    
    std::cout << "\n=== Method 5: Template recursion with index ===\n";
    {
        constexpr auto result = partial_sums_template(TestSeq{});
        static_assert(result[0] == 0 && result[1] == 1 && result[2] == 3 
                   && result[3] == 6 && result[4] == 10);
        std::cout << "Result: [";
        for (std::size_t i = 0; i < result.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result[i];
        }
        std::cout << "]\n";
    }
    
    std::cout << "\n=== Method 6: Stateful lambda with fold (ELEGANT) ===\n";
    {
        constexpr auto result = partial_sums_lambda(TestSeq{});
        static_assert(result[0] == 0 && result[1] == 1 && result[2] == 3 
                   && result[3] == 6 && result[4] == 10);
        std::cout << "Result: [";
        for (std::size_t i = 0; i < result.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << result[i];
        }
        std::cout << "]\n";
        std::cout << "✓ Most elegant: uses fold expression\n";
        std::cout << "✓ Single pass with stateful variables\n";
    }
    
    std::cout << "\n=== Comparison ===\n";
    std::cout << "Method 1 (loop):        Simple, clear, O(n)\n";
    std::cout << "Method 2 (recursive):   O(n²) - recomputes sums\n";
    std::cout << "Method 3 (fold):        O(n²) - lambda per element\n";
    std::cout << "Method 4 (accumulate):  O(n) - BEST performance\n";
    std::cout << "Method 5 (template):    O(n²) - template instantiations\n";
    std::cout << "Method 6 (lambda+fold): O(n) - MOST elegant\n";
    
    std::cout << "\n=== Recommendation ===\n";
    std::cout << "✓ Use Method 4 for best performance and clarity\n";
    std::cout << "✓ Use Method 6 for most elegant/modern C++17 style\n";
    
    return 0;
}
