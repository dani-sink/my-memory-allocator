# 🧠 Custom Memory Allocator

This project implements a low-level dynamic memory allocator in C++, featuring multiple allocation strategies including First Fit, Next Fit, Best Fit, Free List, and Segregated Fit. It mimics how system-level allocators work by managing a heap-like region of memory manually using `sbrk()`.

---

## 📁 File Structure

- `allocator.cpp`: Core implementation of the memory allocator and all allocation strategies.
- `allocator_tests.cpp`: A suite of unit tests covering various scenarios for each allocation strategy.

---

## 🚀 Features

### ✅ Allocation Strategies

The allocator supports the following memory search modes:

- **First Fit**: Finds the first available block that satisfies the request.
- **Next Fit**: Continues search from where the last allocation ended.
- **Best Fit**: Chooses the smallest block that satisfies the request.
- **Free List**: Maintains a list of free blocks and picks the first suitable one.
- **Segregated Fit**: Uses multiple free lists (buckets) based on block size ranges for fast access.

You can switch modes using:

```cpp
init(SearchMode::YourPreferredMode);
```

Choose one of the modes below:

```cpp
enum class SearchMode {
    FirstFit,
    NextFit,
    BestFit,
    FreeList,
    SegregatedFit,
};
```

### 🧱 Block Structure

Each block contains:

- `size`: The size of the block.
- `used`: A flag indicating whether the block is currently allocated or not.
- `next`: A pointer to the next block in the heap.
- `data[]`: Memory space reserved for the user.
-

---

## 🧪 Running the tests

All test cases are written in `allocator_tests.cpp`.

### 🔧 Build

```bash
g++ allocator_tests.cpp -o allocator_tests
```

### ▶️ Run

```bash
./allocator_tests
```

### ✅ Sample Output

```bash
✅ test_basic_allocation passed!
✅ test_nextFit_allocation_one passed!
✅ test_nextFit_allocation_two passed!
✅ test_bestFit_allocation passed!
✅ test_split_block_bestFit passed!
✅ test_coalesce_blocks_bestFit passed!
✅ test_free_list_stategy passed!
✅ test_segregated_fit_strategy passed!
✅ test_segregated_fit_strategy_with_non_exact_sizes passed!

All tests passed!
```

---

## 📚 Notes

- Memory is managed using `sbrk()` so tests must be run on a system that supports it (Unix/Linux/macOS).
- The allocator assumes 8-byte alignment (via word_t).
- The resetHeap() function reclaims and resets all global state between tests.

## 🛠️️ Possible Improvements

- Add `free()` integration for Segregated Fit buckets.
- Introduce memory bounds checking.
- Add support for `realloc()` and `calloc()`.
- Visual debugging/heap dump tool.

## 🙏 Special Thanks

A huge thank you to [Dmitry Soshnikov](http://dmitrysoshnikov.com/compilers/writing-a-memory-allocator/) for his excellent guide on writing memory allocators. <br>
His tutorial provided a clear and structured foundation that greatly helped shape the implementation and ideas in this project.

## 👤 Author

Jean-Daniel Sinkpon
<br>
[LinkedIn](https://linkedin.com/in/daniel-sinkpon) · [GitHub](https://github.com/dani-sink)

## 📄 License

This project is for educational and demonstrational purposes. Feel free to fork and expand it.
