# Stack Vector - Custom Allocator for std::vector

A custom allocator implementation that allows `std::vector` to use a fixed-size buffer (typically stack-allocated) instead of heap allocation.

## Features

- **Fixed-size buffer allocation**: Each allocator instance owns its own fixed-size buffer
- **Configurable alignment**: Optional alignment support via template parameter (defaults to unaligned for maximum space efficiency)
- **Type-safe**: Template-based design with proper alignment handling when needed
- **STL-compatible**: Works with `std::vector` and follows allocator requirements
- **Two usage patterns**:
  - `StackVector<T, N, AlignAccess>`: Helper class that wraps `std::vector` with the custom allocator
  - Manual usage with `StackAllocator<T, N, AlignAccess>` for advanced control
- **Fixed capacity**: Reserves full capacity upfront to prevent reallocations (no buffer growth support needed)
- **Simple API**: No need to manually manage buffers - allocator owns its storage
- **No exceptions**: Returns nullptr on allocation failure for maximum performance (asserts in debug builds)

## Usage

### Simple approach with StackVector

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

### Manual approach with direct allocator usage

```cpp
#include "stack_allocator.hpp"
#include <vector>

// Allocator owns a 1024-byte buffer
StackAllocator<int, 1024, false> alloc;
std::vector<int, StackAllocator<int, 1024, false>> vec(alloc);
vec.reserve(100);  // Reserve capacity upfront to avoid reallocations

vec.push_back(42);
```

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
./example
```

## Limitations

- Fixed capacity determined at compile time (no buffer growth)
- Exceeding capacity returns nullptr (asserts in debug builds)
- Deallocation only reclaims space for the most recent allocation (stack-like behavior)
- `StackVector` reserves full capacity upfront to prevent reallocation issues
- When using manual approach, call `vec.reserve()` upfront to avoid reallocation failures
- No exception throwing for performance (returns nullptr on allocation failure)

## Implementation Details

- **Buffer ownership**: Each allocator instance owns its own fixed-size buffer as a member variable
- **Alignment**: Optional alignment via third template parameter (`AlignAccess`)
  - `false` (default): No alignment, maximum space efficiency, supports unaligned access
  - `true`: Proper alignment for types, may waste some buffer space but can be faster on some architectures
- **Memory management**: Tracks offset within buffer for sequential allocations
- **Rebind support**: Works with containers that rebind allocators (like `std::vector`)
- **Capacity management**: `StackVector` automatically reserves full capacity to prevent reallocation
- **Allocator copying**: Copy constructors create new buffers (allocators are not truly copyable in the traditional sense)

## When to Use

- Performance-critical code where heap allocation overhead matters
- Embedded systems with limited dynamic memory
- Small vectors with known maximum size
- Avoiding heap fragmentation

## License

Free to use and modify.
