# Stack Vec - C++17 Utilities Collection

A collection of C++17 utilities including zero-overhead function runners and stack-based vector allocator.

## Components

### FunctionRunner
Sequential execution with early exit on failure. Executes functions in order and stops at the first failure.

### ParallelRunner  
Executes all functions and collects results. Returns `std::array<bool, N>` with success/failure for each step.

### StackAllocator
Custom allocator that allows `std::vector` to use a fixed-size buffer (typically stack-allocated) instead of heap allocation.

## Features

**Function Runners:**
- **Zero heap allocations**: All storage is inline using `std::tuple` and `std::array`
- **Compile-time validation**: Ensures arguments alternate between callables and messages
- **Type-safe**: Full C++17 template metaprogramming with `std::is_invocable_v`
- **Clean API**: Direct argument syntax - no wrapper functions needed
- **Efficient storage**: Typically 56-168 bytes depending on callable types
- **Performance**: ~60-70ns for 5-step execution (measured with Google Benchmark)
- **Flexible callables**: Supports lambdas, function pointers, `std::bind`, member functions

**StackAllocator:**
- **Fixed-size buffer allocation**: Each allocator instance owns its own fixed-size buffer
- **Configurable alignment**: Optional alignment support via template parameter (defaults to unaligned for maximum space efficiency)
- **Type-safe**: Template-based design with proper alignment handling when needed
- **STL-compatible**: Works with `std::vector` and follows allocator requirements
- **Simple API**: `StackVector` wrapper provides easy-to-use interface
- **No exceptions**: Returns nullptr on allocation failure for maximum performance (asserts in debug builds)

## Usage

### FunctionRunner - Sequential with Early Exit

```cpp
#include "function_runner.hpp"

auto runner = make_function_runner(
    [] { return initialize_system(); }, "Initialize system",
    [] { return connect_database(); },  "Connect to database",
    [] { return load_config(); },       "Load configuration"
);

if (runner.run()) {
    std::cout << "All steps succeeded!\n";
} else {
    std::cout << "Failed at step " << runner.failed_step() 
              << ": " << runner.failed_step_name() << "\n";
}
```

### ParallelRunner - Execute All and Collect Results

```cpp
#include "parallel_runner.hpp"

auto checks = make_parallel_runner(
    [] { return check_disk_space(); },  "Disk space",
    [] { return check_memory(); },      "Memory",
    [] { return check_network(); },     "Network"
);

auto results = checks.run();  // std::array<bool, 3>

// Print results
checks.print_summary(results);

// Or access individually
if (!results[1]) {
    std::cout << "Memory check failed\n";
}
```

### Flexible Callable Types

```cpp
// Function pointers
bool init_logger();
bool init_config();

auto startup = make_parallel_runner(
    init_logger, "Logger",
    init_config, "Config"
);

// Lambda captures
int counter = 0;
auto with_state = make_function_runner(
    [&] { return ++counter > 0; }, "Increment",
    [&] { return counter < 10; },  "Check limit"
);

// std::bind
auto bind_runner = make_function_runner(
    std::bind(&MyClass::method1, &obj), "Method 1",
    std::bind(&MyClass::method2, &obj), "Method 2"
);
```

### StackAllocator - Fixed-Size Vector Buffer

```cpp
#include "stack_allocator.hpp"

// Create a vector of up to 10 integers with fixed-size buffer (unaligned by default)
StackVector<int, 10> vec;
vec.push_back(42);
vec.push_back(100);

// Or with aligned access if needed for performance
StackVector<int, 10, true> aligned_vec;

// Access like a normal vector
for (const auto& val : vec) {
    std::cout << val << " ";
}
```

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .

# Run examples
./function_runner_example
./parallel_runner_example

# Run benchmarks
./benchmark_function_runner
./benchmark_parallel_runner
```

## Storage Efficiency

**Function Runners:**

All storage is inline with zero heap allocations:

**FunctionRunner:**
- `std::tuple` of (callable, `string_view`) pairs
- 1 `int` (4 bytes) for tracking failed step
- Typical sizes: 80-128 bytes

**ParallelRunner:**
- `std::tuple` of (callable, `string_view`) pairs  
- `std::array<bool, N>` for results (N bytes)
- Typical sizes: 56-168 bytes

**Size breakdown by callable type:**
- Simple lambda (no captures): ~1 byte
- Function pointer: 8 bytes
- Lambda with captures: varies by capture size
- `std::bind` object: ~24-32 bytes
- Each `string_view`: 16 bytes (pointer + size)

**Example calculations:**
```
3 simple lambdas:    tuple<3 x (1 + 16)> + 4 = 51 + padding →  80 bytes
4 function pointers: tuple<4 x (8 + 16)> + 4 = 96 + padding → 104 bytes
3 std::bind:         tuple<3 x (32 + 16)> + 4 = 144 + padding → 128 bytes
```

**StackAllocator:**

- **Buffer ownership**: Each allocator instance owns its own fixed-size buffer as a member variable
- **Namespace**: Internal allocator is in `stack_alloc_internal` namespace to indicate it's not part of public API
- **Alignment**: Optional alignment via third template parameter (`AlignAccess`)
  - `false` (default): No alignment, maximum space efficiency, supports unaligned access
  - `true`: Proper alignment for types, may waste some buffer space but can be faster on some architectures
- **Fixed capacity**: Reserves full capacity upfront to prevent reallocations (no buffer growth support needed)

## Implementation Details

**Function Runners:**

**Compile-time Validation:**
- `validate_alternating_args` template struct ensures arguments alternate between callables and messages
- Uses `std::is_invocable_v<F>` to verify callables return `bool`
- Uses `std::is_convertible_v<M, std::string_view>` for messages
- Provides clear error messages at compile time

**Storage Strategy:**
- `std::tuple` stores heterogeneous callable types without type erasure
- No virtual functions or `std::function` overhead
- `std::index_sequence` for compile-time iteration
- Fold expressions for clean template code

**Performance:**
- Zero heap allocations (all inline storage)
- ~60-70ns for 5-step execution (Google Benchmark)
- Inlined execution path for maximum speed
- No runtime polymorphism overhead

**API Design:**
- Alternating argument syntax: `make_function_runner(func, msg, func, msg, ...)`
- Type deduction eliminates need for explicit template parameters
- Messages stored as `std::string_view` (16 bytes each)
- Supports any callable returning `bool`

**StackAllocator:**

- **Memory management**: Tracks offset within buffer for sequential allocations
- **Rebind support**: Works with containers that rebind allocators (like `std::vector`)
- **Capacity management**: `StackVector` automatically reserves full capacity to prevent reallocation
- **Allocator copying**: Copy constructors create new buffers (allocators are not truly copyable in the traditional sense)
- **Deallocation**: Only reclaims space for the most recent allocation (stack-like behavior)

## When to Use

**FunctionRunner:**
- Initialization sequences where order matters
- Setup/teardown operations that should stop on first failure
- Validation pipelines with early exit
- Startup sequences for applications

**ParallelRunner:**
- Health checks that should all run regardless of failures
- Pre-flight validations that need complete results
- Test suites that should run all tests
- Diagnostic operations that collect all status

**StackAllocator:**
- Performance-critical code where heap allocation overhead matters
- Embedded systems with limited dynamic memory
- Small vectors with known maximum size
- Avoiding heap fragmentation

## Limitations

**StackAllocator:**
- Fixed capacity determined at compile time (no buffer growth)
- Exceeding capacity returns nullptr (asserts in debug builds)
- `StackVector` reserves full capacity upfront to prevent reallocation issues
- When using manual approach, call `vec.reserve()` upfront to avoid reallocation failures

## License

Free to use and modify.
