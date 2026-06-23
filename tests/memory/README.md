
## Unit Test Cases for fermat::memory

### 1. ObjectPool<T, MaxFree>

| Case | Purpose | Key Assertions |
|------|---------|----------------|
| `ObjectPoolGetUninitialize` | Allocate an uninitialized memory block from the pool. | Returned pointer is not null, and its size is at least `sizeof(T)`. |
| `ObjectPoolGetConstruct` | Allocate and construct an object with constructor arguments. | Object is properly constructed (e.g., a counter incremented). |
| `ObjectPoolPutRaw` | Return a raw block to the pool. | After `put_raw`, the same block can be re-acquired (cached). |
| `ObjectPoolPutWithDestructor` | Return a constructed object (call destructor then `put_raw`). | Destructor is called (e.g., via a mock with a flag). |
| `ObjectPoolCacheLimit` | When TLS cache size exceeds `MaxFree`, extra blocks are freed to mimalloc. | Allocate more than `MaxFree` objects, release them, then allocate again – the number of mimalloc allocations should not exceed total objects (i.e., recycling works). |
| `ObjectPoolThreadLocalIsolation` | Each thread has its own cache, no cross-thread sharing. | Two threads allocate and release objects independently; total memory usage remains bounded (no cross-thread pointer reuse). |
| `ObjectPoolConcurrentGetPut` | Multiple threads simultaneously get and put objects. | No data races (TSAN clean), no memory leaks, all allocations succeed. |
| `ObjectPoolWithNonTrivialType` | Pool works with types having custom constructors/destructors (e.g., `std::vector<int>`). | Construction and destruction happen correctly; no memory corruption. |
| `ObjectPoolMaxFreeZero` | Edge case: `MaxFree = 0` should free immediately to mimalloc. | After `put_raw`, the pointer is freed; subsequent `get_uninitialize` returns a new allocation (not the same block). |

### 2. Malloc Functions (`Malloc`, `AlignedMalloc`)

| Case | Purpose | Key Assertions |
|------|---------|----------------|
| `GoodAllocSize` | `good_alloc_size` returns a rounded‑up size (mimalloc bucket size). | For `n` = 1, result ≥ 1; typically larger for small sizes. |
| `GoodAllocAndFree` | Basic allocation and free. | Allocated pointer not null, `good_usable_size` returns at least requested size. |
| `GoodAllocUpdatesSize` | `good_alloc` modifies the passed `n` to the actual usable size. | After call, `*n` equals `good_alloc_size(original)`. |
| `GoodRealloc` | Reallocate to larger/smaller size. | Data preserved (if smaller, truncated) and pointer may change. |
| `AlignedAllocAlignment` | `AlignedMalloc<64>::good_alloc` returns pointer aligned to 64 bytes. | `is_aligned(ptr)` true. |
| `AlignedFreeSizeAligned` | `good_free` with size and alignment matches allocation. | No crash, memory returned to allocator. |
| `AlignedAllocLargeAlignment` | Test with Alignment = 4096, allocate large block. | Pointer aligned, usable size ≥ requested. |
| `AlignedAllocZeroSize` | Allocation with `n = 0` should return nullptr (not required but safe). | `good_alloc(nullptr?)` – our code returns `mi_aligned_alloc(Alignment,0)`? Implementation may return nullptr; test that no crash. |

### 3. Allocator<T> (STL compatible)

| Case | Purpose | Key Assertions |
|------|---------|----------------|
| `AllocatorAllocateDeallocate` | Basic allocate/deallocate with `new`/`delete` analogy. | Allocation returns non‑null; deallocate does not crash. |
| `AllocatorConstructDestroy` | `construct` and `destroy` work for trivial and non‑trivial types. | For a type with counter, constructs increments, destroy decrements. |
| `AllocatorRebind` | `rebind` to another type works (test via `std::vector` using allocator). | `std::vector<T, Allocator<T>>` compiles and operates correctly. |
| `AllocatorMaxSize` | `max_size()` returns reasonable value (not zero). | `max_size()` ≥ 1. |
| `AllocatorEquality` | Allocators of same type are equal (true). | `operator==` returns true, `operator!=` false. |
| `AllocatorWithContainers` | Use `Allocator<int>` with `std::vector` or `std::list`. | Container operations (push, pop, resize) work correctly, memory freed. |
| `AllocatorExceptionSafety` | Allocation of huge size throws `std::bad_alloc`. | `allocate(max_size() + 1)` throws exception. |

### 4. AlignedAllocator<T, Alignment>

| Case | Purpose | Key Assertions |
|------|---------|----------------|
| `AlignedAllocatorAlignmentProperty` | `alignment()` returns the template parameter. | `alloc.alignment() == Alignment`. |
| `IsAligned` | `is_aligned(ptr)` true for allocated pointer. | After `allocate`, `is_aligned(ptr)` true. |
| `IsAlignedSize` | `is_aligned_size(n)` checks if `n` is multiple of alignment in bytes. | For `n=64, Alignment=64` returns true; `n=128` true; `n=32` false. |
| `AllocateAlignedMemory` | Allocated pointer aligns to specified alignment. | `reinterpret_cast<uintptr_t>(ptr) % Alignment == 0`. |
| `DeallocateWithAlign` | Deallocate using correct size and alignment (no crash). | Works with `std::vector` with custom allocator. |
| `AlignedAllocatorRebind` | `rebind` to another type preserves alignment. | `using Other = typename decltype(alloc)::template rebind<char>::other;` – `Other` has same alignment. |
| `AlignedAllocatorWithContainer` | Use `AlignedAllocator<char, 64>` with `std::basic_string`. | String operations work, internal memory aligned to 64 bytes. |
| `AlignedAllocatorMoveAndSwap` | Containers using aligned allocator can be moved/swapped efficiently (since `is_always_equal` is true). | No extra allocations; swapping two vectors transfers ownership. |

### 5. Concurrency and Stress Tests

| Case | Purpose | Key Assertions |
|------|---------|----------------|
| `ObjectPoolHighContention` | Many threads (e.g., 16) allocate/release objects in a loop. | No data races, all objects correctly constructed/destructed, total allocated count equals released count. |
| `MixedAllocatorTypes` | Simultaneous use of `Allocator` and `AlignedAllocator` across threads. | No interference, memory management correct. |
| `MallocThreadSafety` | `Malloc::good_alloc`/`good_free` called concurrently (mimalloc is thread‑safe). | No crashes or memory corruption. |

### 6. Memory Leak Detection (ASAN)

| Case | Purpose | Key Assertions |
|------|---------|----------------|
| `ObjectPoolDestructorCleansUp` | When `ObjectPool` is not used directly but thread‑local caches are destroyed at thread exit, all cached pointers are freed. | Run under ASAN, no leaks. |
| `AllocatorLeakOnContainerDestruction` | `std::vector` with custom allocator destroys its elements and frees memory. | ASAN reports no leaks. |

---

These test cases cover functionality, correctness, alignment, STL compatibility, concurrency, and resource management. If you need the actual Google Test implementation for any of them, please specify and I will provide the code.