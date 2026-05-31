# fermat – High‑performance C++ Base Library

[中文](README_CN.md)

fermat is a C++ base component library focused on **high throughput and low latency**, providing optimized strings, chunked buffers, contiguous containers, ordered associative containers, stacks, priority queues, memory pools, LRU caches, radix sort, synchronous disk I/O, and zero‑copy helpers (`Peeker`, `Receiver`, `Customer`/`Reader`). Compared to the standard library, fermat can achieve **2–10×** performance improvement in many scenarios.

Beyond micro‑benchmark speedups, fermat is designed for heavy workloads such as **AI / vector data** with three additional design orientations that can **significantly reduce memory copies and format conversions in real applications**:

1. **Full‑size aligned allocation** – Contiguous containers (`KString`, `Buffer<T>`, `Vector<T>`) support a template alignment parameter (together with tiered pool allocators). From small objects to large buffers, memory can be allocated with required alignment, making it easy to interface with SIMD, vectorization operators or downstream tensor layouts – **avoiding unnecessary rearrangement or secondary copies due to insufficient alignment**.
2. **`seize` / `bestow` ownership transfer** – Contiguous containers provide `seize()` to take out the underlying pointer (container becomes empty **without freeing** the memory) and `bestow(ptr, size, capacity)` to take ownership of an externally allocated block (container must be empty; alignment is checked when `Alignment != 0`). `KString`, `Buffer`, and `Vector` all support this, facilitating integration with external allocators, inference frameworks, or GPU‑side buffers – **avoiding “copy‑in then copy‑out”**.
3. **Chunked sharing, fewer copies** – `CordBufferBase::share()` / `BufferRef`, the writable tail via `borrow()`, `string_view` traversal with `Peeker`, as well as `append_reference` / `Customer` / `Reader` etc., let non‑contiguous or cross‑module large vector data **pass views, assemble references, instead of copying payloads** – especially beneficial for large embeddings or batch buffers in AI workloads.

If you count costs by “bytes copied”, prefer using `size()` to define logical length, pass data via chunked sharing, and when needed call `reserve` + aligned containers; see the sections **Container `capacity` semantics** and zero‑copy helpers below.

## Performance difference threshold

fermat aims to provide **significant** performance gains (>20%) in its sweet spots, while keeping losses in weak areas **acceptable (<10%)** . If an operation is only <10% faster than `std`, it is recommended to stick with the standard library for compatibility and maturity. The tables below only list **noticeable differences** (advantage >20% or disadvantage >10%); operations with negligible differences are omitted and can be considered comparable.

## Key performance differences

### ✅ Significant advantage (fermat recommended)

| Component | Advantageous operations | Typical speedup |
|-----------|-------------------------|------------------|
| `KString` | construction, copy, move, append | <span style="color: green;">1.2x – 3x (>20%)</span> |
| `Buffer<T>` | construction (all sizes), small push/iteration/middle insert‑erase, clear (Release) | <span style="color: green;">2x – 5x</span> |
| `Vector<T>` | same allocation path as `Buffer`; random access on par or faster than `std` in Release | <span style="color: green;">2x – 5x</span> |
| `Deque<T, Alloc, kSubarray>` | construction, push_front/push_back, iteration, random access, middle insert/erase, clear, destruct | <span style="color: green;">2x – 10x</span> |
| `List<T>` | all operations | <span style="color: green;">2x – 20x</span> |
| `PriorityQueue<T>` | push (50–100k), push/pop (≤2k), construct from iterators, bounded stream | <span style="color: green;">1.2x – 4x</span> |
| `Stack<T>` | all operations | <span style="color: green;">20% – 10x (construction)</span> |
| `Bitset` | construction, set/reset/flip, count, find series | <span style="color: green;">1.5x – 5x</span> |
| `LruCache` (turbo_map + fermat_list) | insert, lookup, update, erase, touch | <span style="color: green;">20% – 50%</span> |
| `ObjectPool` / `ResourcePool` | single‑/multi‑threaded alloc/free | <span style="color: green;">5x – 10x</span> |
| `RadixSort` | random integer sorting | <span style="color: green;">5x – 10x</span> |
| `CordBufferBase` | random chunked append (10 KiB – 100 MiB) | <span style="color: green;">1.5x – 5x</span> |
| `CordBinaryStream` | binary serialization write/read/round‑trip | <span style="color: green;">20% – 100%</span> |
| `CordStringStream` | text ping‑pong, round‑trip | <span style="color: green;">20% – 50%</span> |

### ⚠️ Significant disadvantage (prefer `std`)

| Component | Weak operations | Slower than `std` | Reason |
|-----------|-----------------|--------------------|--------|
| `KString` | short‑string `find` | ~30% | different implementation path from `std::string` |
| `CordFormatter` | text formatting | ~6× | currently outputs char‑by‑char, no batch write |
| `Bitset` | bitwise ops (`&`, `|`, `^`), shift | ~10–20% | `std::bitset` better compiler optimization |

> **Note**:
> - `FermatList` is faster than `std::list` in all operations – no disadvantage.
> - `FermatStack` is faster than `std::stack` in all operations – no disadvantage.
> - `KString` is significantly faster in construction, copy, move, append; short‑string `find` is slightly slower than `std::string`.
> - `Buffer<T>` (plain data such as `int`/`float`) and `Vector<T>` (general contiguous container) have no significant disadvantage vs `std::vector` in **Release** mode after recent optimizations; small‑size random access is now roughly on par with `std`.
> - `Deque<T>` (`mem_deque_bench`, default `kDequeSubarraySize = 256`) is faster than or on par with `std::deque` on all measured operations; tune the third template parameter (power of 2) for block size vs memory overhead.
> - `ObjectPool` cross-thread transfer (`collect_tsl` / `apply_tls`, `collect_arena` / `apply_arena`) stops at **moving free blocks via `ObjectGuard`**—an intentional ceiling after **coroutine / M:N** considerations. **Advanced users only** (strict memory/lifetime control); otherwise use same-thread pool or `ResourcePool`.
> - **`CordInputStringStream` vs `std::istringstream` on contiguous data** (TokenRead / BulkRead in [`benchmark/README.md`](benchmark/README.md)) is a **reference ceiling** for flat memory—not a fair “fermat disadvantage”; use std when data is already contiguous, Cord streams when data lives in `CordBufferBase` chunks.

## When to use which component

- **Only replace `std` with a fermat component when its advantage in your scenario is significant (>20%) and its disadvantage is acceptable (<10%).** Otherwise prefer the standard library.
- If the bottleneck is not here, there is no need to replace.

| Scenario | Recommended component | Notes |
|----------|-----------------------|-------|
| Many string constructions / copies / moves | `KString` | <span style="color: green;">1.3–3×</span> faster than `std::string`; short‑string `find` slightly slower |
| Streaming large data (logs, network packets) | `CordBufferBase` | extremely high random‑chunk append throughput (best in 10 KiB–100 MiB) |
| Filling chunked data from disk/network | `CordBufferBase` + `append_by_*` | seamless integration with `IOReader`, zero‑copy |
| Performance‑sensitive small/medium contiguous arrays | `Buffer<T>` / `Vector<T>` | use `Buffer` for plain POD; `Vector` for full `std::vector` semantics. Release: many ops <span style="color: green;">≥20% faster</span>; large push_back roughly on par |
| Frequent push_front / push_back (deque) | `Deque<T, Alloc, kSubarray>` | third template arg `kSubarray` (power of 2, default **256**) sets elements per memory block; larger blocks reduce chunk crossings (better scan/insert); smaller blocks save memory when `size()` is tiny |
| Ordered read‑only / bulk construction of maps/sets | `VectorMap` / `VectorSet` | ordered insert <span style="color: green;">3.8× faster</span>, iteration <span style="color: green;">24× faster</span>; random insert slower, not for frequent modification |
| Frequent stack operations | `FermatStack` | construction from container <span style="color: green;">~11× faster</span>, push/pop <span style="color: green;">~1.2× faster</span>, no disadvantage |
| Priority queue with priority change/remove | `FermatPriorityQueue` | supports `change`/`remove`; push <span style="color: green;">~2×</span> at ≤2k, push/pop <span style="color: green;">~4×</span> at 1k; std pop cliff ~600 on measured platform |
| Small object pooling (same thread) | `ObjectPool` | <span style="color: green;">5–10×</span> faster than `new`/`delete` on thread‑local cache |
| Producer / consumer threads reuse pooled memory | `ObjectPool` / `BasicAllocator` + `collect_tsl` / `apply_tls` (or `collect_arena` / `apply_arena`) | **Advanced**: strict memory/thread/lifetime control required; see ObjectPool section |
| High‑concurrency resource reuse (with versioning) | `ResourcePool` | thread‑safe handle lookup; 16 threads ~394 ns/op |
| Local LRU cache | `LruCache` + `turbo::flat_hash_map` + `fermat::List` | <span style="color: green;">20–50% faster</span> than `std::map`+`std::list` in all operations |
| Large integer sorting (random data) | `RadixSort` | <span style="color: green;">5–10× faster</span> than `std::sort`; for already‑sorted or reverse‑sorted data, `std::sort` is faster |
| Finding next set bit in a bitset | `fermat::Bitset` | <span style="color: green;">`FindNext` 4× faster</span> than `std::bitset`; bitwise ops slightly slower, avoid heavy use |
| Bit flags over **existing** memory (no extra allocation) | `BitmapView<true, uint64_t>` | non-owning view via `setup()` / constructor; backing must be **8-byte-aligned** `uint64_t` words — see Bitset section |
| Synchronous file I/O | `sys_*` / `*ReadFile` / `*WriteFile` | cross‑platform abstraction, error handling with `turbo::Result` |
| Zero‑copy cross‑block traversal of CordBuffer | `Peeker` | returns `string_view`, no copy |
| Consuming data from CordBuffer into a container | `Customer` / `Reader` | supports copy‑or‑move consumption |
| Dynamic random map / set | `std::map` or `turbo::flat_hash_map` | fermat does not provide a high‑performance random map |
| Data already in contiguous memory; parse/read without flattening | `std::vector` / `std::string` + `std::istringstream` | reference upper bound; not a fermat weakness |
| Parse/tokenize chunked Cord data (network, IOBuf, shared refs) | `CordInputStringStream` + `Peeker` | avoid flattening to a single buffer first |
| High‑throughput text formatting | `CordOutputStringStream` or `fmt::format_to` + custom sink | avoid `CordFormatter` |

## Container `capacity` semantics

For contiguous containers such as `KString`, `Buffer<T>`, `Vector<T>`:

- **Logical data boundary** is `size()` (or the actual written length).
- **Current available storage upper bound** is `capacity()`, which can be used to decide how many more bytes can be appended before reallocation. However, **do not cache `capacity()`** – read it when needed.

After a `reserve(n)`, fermat tries to make the allocated memory available, so usually **`capacity() >= n`**. For small sizes, the memory is rounded up according to tiered pool allocation (`good_size` / pool tiers), so **`capacity()` may be larger than `n`**. This is similar to the behavior of `std::vector` / `std::string` (implementations often leave extra space to reduce allocations). The difference is that **the standard library imposes stronger constraints for semantic consistency** (in many cases `capacity()` is close to the requested value after `reserve`), while **fermat prioritizes pooling and reducing allocations**, not guaranteeing that `capacity()` exactly matches the `reserve` parameter.

Therefore: treat `capacity()` as a **momentary** upper bound on available space; never store a read `capacity()` into a member variable, a protocol field, or a persistent state – it changes after `reserve`, `shrink`, `clear`, or further growth, and caching it can cause incorrect space judgments.

## Detailed performance data

### String KString

`fermat::BasicString` (alias `KString`) outperforms `std::string` in construction, copy, move, append etc.; short‑string `find` is slightly slower. Data from [`benchmark/README.md`](benchmark/README.md). Better performance per row is **bolded**.

| Operation | KString | std::string | Speedup |
|-----------|---------|-------------|---------|
| Construct (Long) | <span style="color: green;">14.4 ns</span> | 41.3 ns | <span style="color: green;">2.9×</span> |
| Copy (Long) | <span style="color: green;">25.8 ns</span> | 31.6 ns | <span style="color: green;">1.2×</span> |
| Move (Long) | <span style="color: green;">14.9 ns</span> | 43.7 ns | <span style="color: green;">2.9×</span> |
| Append (Medium) | <span style="color: green;">3.75 ns</span> | 4.38 ns | <span style="color: green;">1.2×</span> |
| Find (Short) | 10.8 ns | <span style="color: green;">8.13 ns</span> | slightly slower |
| Find (Long) | <span style="color: green;">92.6 ns</span> | 107 ns | <span style="color: green;">1.2×</span> |
| Hash (Long) | <span style="color: green;">229 ns</span> | 232 ns | comparable |

Strategy: conservative growth. For large accumulated data, pre‑`reserve()`.

```cpp
fermat::KString s = "hello";
s.reserve(10 * 1024 * 1024);
for (int i = 0; i < 1000000; ++i) s.append("a");
```

### Contiguous container `Buffer<T>`

`fermat::Buffer<T>` is a contiguous container specialized for plain data (e.g., `int`, `float`). Its interface is compatible with `std::vector`, but construction and clearing are faster. Release mode performance data (Intel 12‑core @ 4.4 GHz, L3 18 MiB). Better per row in **bold**.

| Operation | Size | std::vector<int> (ns)                     | fermat::Buffer<int> (ns)                   | Winner |
|-----------|------|-------------------------------------------|--------------------------------------------|--------|
| **Construct** | 4 | 14.2                                      | <span style="color: green;">4.77</span>    | fermat |
| | 8 | 14.1                                      | <span style="color: green;">4.71</span>    | fermat |
| | 16 | 13.6                                      | <span style="color: green;">4.71</span>    | fermat |
| | 32 | 13.9                                      | <span style="color: green;">4.91</span>    | fermat |
| | 64 | 14.0                                      | <span style="color: green;">5.86</span>    | fermat |
| | 128 | 14.5                                      | <span style="color: green;">6.97</span>    | fermat |
| | 256 | 15.7                                      | <span style="color: green;">8.84</span>    | fermat |
| | 512 | 39.1                                      | <span style="color: green;">12.7</span>    | fermat |
| | 1024 | 50.0                                      | <span style="color: green;">21.4</span>    | fermat |
| **PushBackSmall** | 4 | 14.5                                      | <span style="color: green;">5.61</span>    | fermat |
| | 8 | 15.3                                      | <span style="color: green;">7.81</span>    | fermat |
| | 16 | 17.4                                      | <span style="color: green;">14.0</span>    | fermat |
| | 32 | <span style="color: green;">20.7</span>   | 23.5                                       | std |
| | 64 | <span style="color: green;">39.9</span>   | 39.6                                       | std |
| | 128 | <span style="color: green;">65.0</span>   | 69.3                                       | std |
| | 256 | 143                                       | <span style="color: green;">138</span>     | fermat |
| | 512 | 286                                       | <span style="color: green;">268</span>     | fermat |
| | 1024 | 571                                       | <span style="color: green;">516</span>     | fermat |
| **IterationSmall** | 4 | 15.9                                      | <span style="color: green;">5.57</span>    | fermat |
| | 8 | 17.9                                      | <span style="color: green;">10.1</span>    | fermat |
| | 16 | 19.5                                      | <span style="color: green;">12.9</span>    | fermat |
| | 32 | <span style="color: green;">26.8</span>   | 34.0                                       | std |
| | 64 | 34.7                                      | <span style="color: green;">31.5</span>    | fermat |
| | 128 | 54.0                                      | <span style="color: green;">47.0</span>    | fermat |
| | 256 | 94.9                                      | <span style="color: green;">83.8</span>    | fermat |
| | 512 | 191                                       | <span style="color: green;">152</span>     | fermat |
| | 1024 | 340                                       | <span style="color: green;">305</span>     | fermat |
| **RandomAccessSmall** | 4 | 181                                       | <span style="color: green;">183</span>     | std |
| | 8 | 186                                       | <span style="color: green;">180</span>     | fermat |
| | 16 | 189                                       | <span style="color: green;">178</span>     | fermat |
| | 32 | 179                                       | <span style="color: green;">178</span>     | std |
| | 64 | 174                                       | <span style="color: green;">177</span>     | std |
| | 128 | 178                                       | <span style="color: green;">174</span>     | fermat |
| | 256 | 179                                       | <span style="color: green;">183</span>     | std |
| | 512 | 177                                       | <span style="color: green;">181</span>     | std |
| | 1024 | 177                                       | <span style="color: green;">178</span>     | std |
| **InsertMiddleSmall** | 4 | 73.5                                      | <span style="color: green;">42.9</span>    | fermat |
| | 8 | 66.3                                      | <span style="color: green;">43.9</span>    | fermat |
| | 16 | 55.1                                      | <span style="color: green;">52.7</span>    | fermat |
| | 32 | 59.4                                      | <span style="color: green;">57.2</span>    | fermat |
| | 64 | 77.3                                      | <span style="color: green;">73.2</span>    | fermat |
| | 128 | 102                                       | <span style="color: green;">73.6</span>    | fermat |
| | 256 | 135                                       | <span style="color: green;">110</span>     | fermat |
| | 512 | 188                                       | <span style="color: green;">148</span>     | fermat |
| | 1024 | 270                                       | <span style="color: green;">258</span>     | fermat |
| **EraseMiddleSmall** | 4 | 20.5                                      | <span style="color: green;">11.8</span>    | fermat |
| | 8 | 28.4                                      | <span style="color: green;">22.7</span>    | fermat |
| | 16 | 46.1                                      | <span style="color: green;">41.4</span>    | fermat |
| | 32 | <span style="color: green;">52.4</span>   | 53.4                                       | std |
| | 64 | 59.7                                      | <span style="color: green;">56.0</span>    | fermat |
| | 128 | 66.2                                      | <span style="color: green;">66.4</span>    | std |
| | 256 | 87.2                                      | <span style="color: green;">83.6</span>    | fermat |
| | 512 | 135                                       | <span style="color: green;">107</span>     | fermat |
| | 1024 | 180                                       | <span style="color: green;">165</span>     | fermat |
| **ClearShrinkSmall** | 4 | 14.5                                      | <span style="color: green;">10.2</span>    | fermat |
| | 8 | 14.5                                      | <span style="color: green;">9.83</span>    | fermat |
| | 16 | 14.3                                      | <span style="color: green;">10.1</span>    | fermat |
| | 32 | 14.9                                      | <span style="color: green;">10.9</span>    | fermat |
| | 64 | 15.1                                      | <span style="color: green;">10.7</span>    | fermat |
| | 128 | 17.4                                      | <span style="color: green;">12.8</span>    | fermat |
| | 256 | 18.6                                      | <span style="color: green;">14.6</span>    | fermat |
| | 512 | 39.4                                      | <span style="color: green;">18.3</span>    | fermat |
| | 1024 | 48.6                                      | <span style="color: green;">31.7</span>    | fermat |
| **PushBack** | 1000 | 489                                       | <span style="color: green;">469</span>     | fermat |
| | 10000 | 4725                                      | <span style="color: green;">4711</span>    | std |
| | 100000 | <span style="color: green;">54065</span>  | 55949                                      | std |
| **Iteration** | 1000 | <span style="color: green;">247</span>    | 248                                        | std |
| | 10000 | 2389                                      | <span style="color: green;">2355</span>    | fermat |
| | 100000 | 23770                                     | <span style="color: green;">23646</span>   | fermat |
| **RandomAccess** | 10000 | 1713                                      | <span style="color: green;">1710</span>    | fermat |
| | 100000 | <span style="color: green;">1730</span>   | 1758                                       | std |
| **InsertMiddle** | 1000 | 1505                                      | <span style="color: green;">1503</span>    | fermat |
| | 10000 | 13498                                     | <span style="color: green;">13017</span>   | fermat |
| **EraseMiddle** | 1000 | 1379                                      | <span style="color: green;">1362</span>    | fermat |
| | 10000 | <span style="color: green;">462606</span> | 463187                                     | std |
| **Sort** | 10000 | 342928                                    | <span style="color: green;">342581</span>  | fermat |
| | 100000 | 4533225                                   | <span style="color: green;">4500183</span> | fermat |
| **ClearAndShrink** | 10000 | <span style="color: green;">686</span>    | 709                                        | std |
| | 100000 | 6778                                      | <span style="color: green;">6694</span>    | fermat |

```cpp
fermat::Buffer<int> v;
v.reserve(1000);
v.push_back(42);
```

### Double-ended queue `Deque<T>`

`fermat::Deque` is interface-compatible with `std::deque`. The third template parameter **`kDequeSubarraySize`** (must be a **power of 2**) is the number of elements per internal memory block; default is **256**. Smaller blocks reduce wasted capacity for very small deques; larger blocks reduce pointer-array and chunk-crossing overhead (better for scan and middle insert/erase on large deques).

```cpp
fermat::Deque<int> dq;                         // default kDequeSubarraySize = 256
fermat::Deque<int, BasicAllocator<int>, 128> dq128;  // 128 elements per block
```

Benchmark below: `fermat::Deque<int>` vs `std::deque<int>` (`mem_deque_bench`, Release). Better per row in **bold**.

| Benchmark | Size | std::deque&lt;int&gt; | fermat::Deque&lt;int&gt; |
|-----------|------|----------------------|--------------------------|
| **ConstructEmpty** | – | 26.1 ns | <span style="color: green;">7.25 ns</span> |
| **ConstructFill** | 8 | 29.6 ns | <span style="color: green;">9.29 ns</span> |
| | 64 | 30.8 ns | <span style="color: green;">10.8 ns</span> |
| | 512 | 79.3 ns | <span style="color: green;">32.5 ns</span> |
| | 4096 | 826 ns | <span style="color: green;">203 ns</span> |
| | 32768 | 10617 ns | <span style="color: green;">1974 ns</span> |
| | 100000 | 69040 ns | <span style="color: green;">6476 ns</span> |
| **PushBack** | 8 | 28.1 ns | <span style="color: green;">11.5 ns</span> |
| | 64 | 63.4 ns | <span style="color: green;">42.3 ns</span> |
| | 512 | 535 ns | <span style="color: green;">249 ns</span> |
| | 4096 | 3899 ns | <span style="color: green;">1945 ns</span> |
| | 32768 | 32319 ns | <span style="color: green;">17317 ns</span> |
| | 100000 | 167878 ns | <span style="color: green;">54955 ns</span> |
| **PushFront** | 8 | 42.5 ns | <span style="color: green;">12.1 ns</span> |
| | 64 | 57.4 ns | <span style="color: green;">40.5 ns</span> |
| | 512 | 303 ns | <span style="color: green;">268 ns</span> |
| | 4096 | 2464 ns | <span style="color: green;">2002 ns</span> |
| | 32768 | 22512 ns | <span style="color: green;">18141 ns</span> |
| | 100000 | 67356 ns | <span style="color: green;">56549 ns</span> |
| **RandomAccess** | – | 1.63 ns | <span style="color: green;">1.62 ns</span> |
| **Iteration** | – | 30287 ns | <span style="color: green;">29205 ns</span> |
| **InsertMiddle** | – | 1488 ns | <span style="color: green;">882 ns</span> |
| **EraseMiddle** | – | 1767 ns | <span style="color: green;">767 ns</span> |
| **Clear** | – | 222 ns | <span style="color: green;">62.2 ns</span> |
| **Destruct** | – | 128 ns | <span style="color: green;">44.5 ns</span> |

### Contiguous container `Vector<T>`

`fermat::Vector<T>` is interface‑compatible with `std::vector` and supports non‑trivial types; it shares the optimized storage/allocation path with `Buffer`. **Release** data below (same machine as above). Full tables: [`benchmark/README.md`](benchmark/README.md) (Vector section).

| Operation | Size | std::vector<int> (ns) | fermat::Vector<int> (ns) | Winner |
|-----------|------|----------------------|--------------------------|--------|
| **Construct** | 4 | 14.6 | <span style="color: green;">5.31</span> | fermat |
| | 1024 | 43.5 | <span style="color: green;">23.9</span> | fermat |
| **PushBackSmall** | 4 | 14.5 | <span style="color: green;">6.93</span> | fermat |
| | 1024 | 488 | <span style="color: green;">481</span> | fermat |
| **IterationSmall** | 4 | 15.4 | <span style="color: green;">5.70</span> | fermat |
| | 1024 | 301 | <span style="color: green;">273</span> | fermat |
| **RandomAccessSmall** | 4 | 174 | <span style="color: green;">174</span> | tie |
| | 1024 | <span style="color: green;">172</span> | 180 | std |
| **RandomAccess** | 10000 | 1739 | <span style="color: green;">1698</span> | fermat |
| | 100000 | 1732 | <span style="color: green;">1725</span> | fermat |
| **PushBack** | 1000 | 487 | <span style="color: green;">478</span> | fermat |
| **EmplaceBack** | 100000 | 54018 | <span style="color: green;">53590</span> | fermat |
| **Iteration** | 100000 | 23576 | <span style="color: green;">23511</span> | fermat |
| **InsertMiddle** | 10000 | 13698 | <span style="color: green;">12917</span> | fermat |
| **EraseMiddle** | 1000 | 1396 | <span style="color: green;">1367</span> | fermat |
| **ClearShrinkSmall** | 1024 | 43.0 | <span style="color: green;">25.5</span> | fermat |

```cpp
fermat::Vector<int> v;
v.reserve(1000);
v.emplace_back(42);
```

### Ordered associative containers VectorMap / VectorSet

Based on sorted `vector`, very fast ordered insertion and bulk construction, iteration much faster than `std::map`. Better per row in **bold**.

| Operation (1000) | VectorMap | std::map | Speedup |
|-----------------|-----------|----------|---------|
| Ordered insert | <span style="color: green;">9.2 µs</span> | 35.3 µs | <span style="color: green;">3.8×</span> |
| Iteration | <span style="color: green;">0.15 ns/element</span> | 3.6 ns/element | <span style="color: green;">~24×</span> |
| Random insert | 83.9 µs | <span style="color: green;">26.3 µs</span> | slower |

For random insert and frequent erase, use `std::map` / `turbo::flat_hash_map`.

```cpp
fermat::VectorMap<int, std::string> m;
m.insert({1, "one"});
auto it = m.find(1);
```

### Stack FermatStack

`top()` and construction from container significantly faster than `std::stack`. Better per row in **bold**.

| Operation | FermatStack | std::stack | Speedup |
|-----------|-------------|------------|---------|
| top() | <span style="color: green;">0.374 ns</span> | 0.423 ns | <span style="color: green;">1.1×</span> |
| construct from container (1000) | <span style="color: green;">20.2 ns</span> | 220 ns | <span style="color: green;">10.9×</span> |
| push/pop (1000) | <span style="color: green;">759 ns</span> | 919 ns | <span style="color: green;">1.2×</span> |

```cpp
fermat::stack<int, 0> st;
st.push(1);
int v = st.top();
st.pop();
```

### Priority queue FermatPriorityQueue

Supports `change` / `remove` (can modify elements via `get_container()`). Push uses `fermat::push_heap`; pop uses `std::pop_heap` (same as std adapter today). Better per row in **bold**.

| Operation | Size | FermatPriorityQueue | std::priority_queue | Notes |
|-----------|------|---------------------|----------------------|-------|
| **Push** | 1000 | <span style="color: green;">1853 ns</span> | 2369 ns | <span style="color: green;">~1.3×</span> |
| | 100000 | <span style="color: green;">980 µs</span> | 1253 µs | <span style="color: green;">~1.3×</span> |
| **Pop** | 1000 | <span style="color: green;">12.6 µs</span> | 38.7 µs | std cliff ~600 |
| | 100000 | <span style="color: green;">7.10 ms</span> | 7.25 ms | ~equal |
| **PushPop** | 1000 | <span style="color: green;">9.90 µs</span> | 41.4 µs | <span style="color: green;">~4.2×</span> |
| | 100000 | <span style="color: green;">7.82 ms</span> | 8.19 ms | <span style="color: green;">~1.05×</span> |
| **ConstructFromIterators** | 10000 | <span style="color: green;">20.2 µs</span> | 46.9 µs | <span style="color: green;">~2.3×</span> |
| **BoundedPushPop** (limit 1000) | – | <span style="color: green;">13.9 µs</span> | 33.8 µs | 2000 stream ops |
| change/remove (1000) | – | 1.68 µs | not supported | – |

Full size sweep: [`benchmark/README.md`](benchmark/README.md) (Priority queue section).

```cpp
fermat::PriorityQueue<int, 0> pq;
pq.push(10);
size_t idx = 0;
pq.get_container()[idx] = 20;
pq.change(idx);
pq.remove(idx);
```

### Object pool ObjectPool

`fermat::ObjectPool` (`fermat/memory/object_pool.h`) keeps a **thread‑local free list** per type: hot `get` / `put` on one thread avoid the global allocator. Single‑thread allocation/deallocation is ~6× faster than `new`/`delete` in benchmarks. Better per row in **bold**.

| Operation | ObjectPool | new/delete | Speedup |
|-----------|------------|------------|---------|
| single thread | <span style="color: green;">2.02 ns</span> | 11.7 ns | <span style="color: green;">5.8×</span> |
| 16 threads | <span style="color: green;">3.77 ns</span> | 29.0 ns | <span style="color: green;">7.7×</span> |

**Same thread (typical):**

```cpp
MyClass* obj = fermat::ObjectPool<MyClass>::get(/* ctor args */);
fermat::ObjectPool<MyClass>::put(obj);
```

**Producer / consumer threads — transfer pooled memory between threads**

**Intentional API boundary (not “we could only go this far”)**

With **coroutines** and M:N scheduling, one OS thread often runs many logical tasks, and work may **resume on another thread or executor**. If the library wrapped a “standard producer–consumer pool,” it would have to own **batching, backpressure, queue depth, and when to transfer memory**—overlapping your runtime, locks, and pipeline semantics.

After explicitly accounting for coroutine scenarios, fermat exposes only:

- **`ObjectPool<T>::collect_tsl()` / `apply_tls()`** — move a thread’s TLS free list through an **`ObjectGuard`**
- **`BasicAllocator::collect_arena()` / `apply_arena()`** — the same for **tiered** pools behind `Buffer` / `Vector` / `KString`

You call these at **boundaries you already define** (end of batch, coroutine handoff, thread switch). **Free blocks** move; live object lifetime rules stay yours.

A higher layer would embed **contention and scheduling policy** and **compete with business code**. The current primitives are a **deliberate design ceiling**, not an unfinished wrapper.

> **Audience**  
> **`collect` / `apply` cross-thread transfer is an advanced path.** You must strictly own object lifetime, which thread may `get` / `put_raw`, when live pointers are in flight vs when only free blocks move, and guard handoff ordering. If that is not your team’s comfort zone, stay on **same-thread `ObjectPool`** or use **`ResourcePool`** for cross-thread handles—do not use TLS transfer as a default shortcut.

`ObjectPool` caches are **thread‑local**: do not `put` on thread B a pointer allocated on thread A. Typical **producer allocates, consumer recycles**: the producer `get`s from its TLS; the consumer `put_raw`s into its TLS; when a batch is done, the **consumer `collect_tsl()`** and the **producer `apply_tls()`** move the free list back to the producer’s cache:

| API | Role |
|-----|------|
| `ObjectPool<T>::collect_tsl()` | On the **source** thread: move all blocks in the current TLS free list into an `ObjectGuard` (RAII; frees on destroy if not moved). |
| `ObjectPool<T>::apply_tls(guard)` | On the **destination** thread: merge `guard` into that thread’s TLS cache; subsequent `get_uninitialize` / `put_raw` reuse those blocks locally. |
| `BasicAllocator<T, Align>::collect_arena()` / `apply_arena()` | Same idea for **tiered** pool slots used by `Buffer` / `Vector` / `KString` (`TieredAllocator` behind `BasicAllocator`). Returns one `ObjectGuard` per size class. |

Hand off the guard under your own sync (mutex, queue, epoch barrier). After the consumer **`put_raw`s a batch**, **`collect_tsl()` on the consumer** and **`apply_tls()` on the producer** return freed blocks to the producer’s TLS—**ping‑pong reuse** with only a short critical section on the guard, not per object.

```cpp
struct RequestCtx { /* ... */ };

std::mutex mu;
std::optional<fermat::ObjectGuard<0>> freed;  // consumer → producer

// Producer (e.g. accept / parse): alloc from local TLS; send live objects via your queue
void producer_issue(WorkQueue& q) {
  RequestCtx* ctx = fermat::ObjectPool<RequestCtx>::get_uninitialize();
  // construct / fill ctx ...
  q.push(ctx);  // business queue — not ObjectGuard
}

// Producer: after consumer collect_tsl — merge returned free blocks into local TLS
void producer_reclaim_free_list() {
  fermat::ObjectGuard<0> guard;
  {
    std::lock_guard lock(mu);
    if (freed) { guard = std::move(*freed); freed.reset(); }
  }
  if (!guard.ptrs.empty()) {
    fermat::ObjectPool<RequestCtx>::apply_tls(guard);
  }
}

// Consumer (e.g. worker): process, put_raw locally, then collect and hand back to producer
void consumer_drain(WorkQueue& q) {
  while (RequestCtx* ctx = q.pop()) {
    // ... process ...
    fermat::ObjectPool<RequestCtx>::put_raw(ctx);
  }
  auto guard = fermat::ObjectPool<RequestCtx>::collect_tsl();
  std::lock_guard lock(mu);
  freed = std::move(guard);
}
```

**Tiered allocator** (consumer releases many `Buffer`/`Vector` blocks, then returns caches to the producer):

```cpp
using Alloc = fermat::BasicAllocator<uint8_t, 64>;
// on consumer thread after deallocations into the pool:
auto guards = Alloc::collect_arena();
// move `guards` to producer thread, then on producer:
Alloc::apply_arena(guards);
```

For **cross‑thread live object handles** (not just moving empty blocks), use **`ResourcePool`** (versioned ID + `find` / `put`) instead of passing raw pool pointers.

### Resource pool ResourcePool

Sharded object pool with versioned IDs, supports cross‑thread `find` / `put` (`mem_resource_bench`).

| Operation | Time |
|-----------|------|
| single thread alloc/free | **14.9 ns** (~67 M ops/s) |
| 16 threads get/put | **394 ns** (~40 M ops/s) |

```cpp
using MyPool = fermat::ResourcePool<MyClass, 8, 64, 1024, 64>;
uint64_t rid;
MyClass* obj = MyPool::get(rid, /* ctor args */);
MyClass* found = MyPool::find(rid);
MyPool::put(rid);
```

### LRU cache LruCache

Configurable underlying map and list types. Benchmark compares `turbo::flat_hash_map` + fermat intrusive list vs `std::map` + `std::list` (`mem_lru_cache`, capacity 100k, cache capacity = test key count; low‑hit test uses half capacity). Each combination runs 6 operations, average time (ns or µs) for 1000/10000/100000 keys:

| Operation | Keys | std_map_std_list | std_map_fermat_list | turbo_map_std_list | turbo_map_fermat_list |
|-----------|------|-------------------|----------------------|---------------------|------------------------|
| **Insert distinct** | 1000 | 81.0 µs | 59.2 µs | 43.0 µs | <span style="color: green;">31.5 µs</span> |
| | 10000 | 869 µs | 729 µs | 514 µs | <span style="color: green;">406 µs</span> |
| | 100000 | 11.57 ms | 8.28 ms | 6.82 ms | <span style="color: green;">5.77 ms</span> |
| **Lookup high hit** | 1000 | 45.0 ns | 31.0 ns | 28.3 ns | <span style="color: green;">21.4 ns</span> |
| | 10000 | 51.1 ns | 37.2 ns | 31.8 ns | <span style="color: green;">25.9 ns</span> |
| | 100000 | 91.7 ns | 45.7 ns | 40.7 ns | <span style="color: green;">32.9 ns</span> |
| **Lookup low hit** | 1000 | 114 ns | 96.8 ns | 85.8 ns | <span style="color: green;">72.3 ns</span> |
| | 10000 | 126 ns | 107 ns | 106 ns | <span style="color: green;">90.1 ns</span> |
| | 100000 | 149 ns | 118 ns | 127 ns | <span style="color: green;">114 ns</span> |
| **Update (assign)** | 1000 | 33.9 ns | 23.6 ns | 22.6 ns | <span style="color: green;">16.2 ns</span> |
| | 10000 | 42.4 ns | 29.4 ns | 25.6 ns | <span style="color: green;">21.0 ns</span> |
| | 100000 | 66.5 ns | 38.6 ns | 33.3 ns | <span style="color: green;">27.8 ns</span> |
| **Erase all** | 1000 | 73.9 µs | 56.2 µs | 42.8 µs | <span style="color: green;">29.1 µs</span> |
| | 10000 | 808 µs | 717 µs | 485 µs | <span style="color: green;">398 µs</span> |
| | 100000 | 9.88 ms | 8.22 ms | 6.42 ms | <span style="color: green;">5.56 ms</span> |
| **touch** | 1000 | 32.9 ns | 22.6 ns | 21.9 ns | <span style="color: green;">16.2 ns</span> |
| | 10000 | 39.5 ns | 29.6 ns | 25.2 ns | <span style="color: green;">21.3 ns</span> |
| | 100000 | 55.6 ns | 38.7 ns | 31.4 ns | <span style="color: green;">27.9 ns</span> |

> **Conclusion**: `turbo::flat_hash_map` + `fermat::List` performs best in all operations and is recommended as the high‑performance LRU cache implementation.

See [`benchmark/README.md`](benchmark/README.md) for full raw data.

```cpp
fermat::LruCache<int, std::string> cache(100000);
cache.insert_or_assign(1, "one");
if (auto v = cache.at(1)) { /* use *v */ }
cache.touch(1);
cache.erase(1);
```

### Radix sort RadixSort

Sorting random `uint32_t` data is ~10× faster than `std::sort`.

| Data size | RadixSort | std::sort | Speedup |
|-----------|-----------|-----------|---------|
| 1M random u32 | <span style="color: green;">5.34 ms</span> | 53.7 ms | <span style="color: green;">10.1×</span> |

```cpp
std::vector<uint32_t> data = {5, 2, 8, 1};
fermat::radix_sort(data.begin(), data.end());
```

### Bitset and `BitmapView`

**`fermat::Bitset`** — fixed-size, owning bitset (compile-time bit count), comparable to `std::bitset`.

**`fermat::BitmapView`** — **non-owning view** over memory you already have (e.g. a field in a struct, a slab, mmap). It does not allocate or free storage; bind with `setup(span<uint64_t>, bit_count)` or the constructor taking `turbo::span<WordType>`. This is intended so business code can operate on existing bitmap buffers without copying.

> **Required: `WordType = uint64_t` and 8-byte-aligned backing**
>
> - The third template parameter **`WordType`** must be **`uint64_t`** (8 bytes per machine word) for **supported** use. Benchmarks and fast paths (`find_*`, word-wise ops) assume 64-bit words.
> - The backing buffer must be an array of `uint64_t` with **8-byte alignment** (`alignas(8)` or equivalent), sized to at least `ceil(bit_count / 64)` words.
> - Instantiating `BitmapView` with **`WordType` smaller than 8 bytes** (e.g. `uint32_t`, `uint16_t`, `uint8_t`) is **not** supported for performance or safety guarantees: correctness and SIMD/word tricks are only validated for **`uint64_t`**.

```cpp
constexpr size_t kBits = 1024;
alignas(8) uint64_t storage[(kBits + 63) / 64]{};

fermat::BitmapView<true, uint64_t> bm;
bm.setup(turbo::span<uint64_t>(storage), kBits);
size_t i = bm.find_first();
```

`FindNext` on **`fermat::Bitset`** for 1024 bits is faster than `std::bitset` (`mem_bitset_bench`); `BitmapView<uint64_t>` is similar on find paths when bound to aligned `uint64_t` storage. Full three-way tables: [`benchmark/README.md`](benchmark/README.md) (Bitset section).

| Operation (1024 bit) | fermat::Bitset | std::bitset | Speedup |
|----------------------|----------------|-------------|---------|
| find_next | <span style="color: green;">0.387 ns</span> | 1.56 ns | <span style="color: green;">4.0×</span> |

### Synchronous disk I/O

`fermat/io` provides a cross‑platform synchronous I/O stack based on `turbo::Result` error handling:

| Layer | Description |
|-------|-------------|
| `sys_*` | thin wrappers for `open/read/write/pread/pwrite/*v/seek/flush` |
| `IOReader` / `IOWriter` | virtual interfaces for consumption by `CordBuffer` etc. |
| `SequenceReadFile` / `RandomReadFile` | read‑side file types |
| `SequenceWriteFile` / `RandomWriteFile` | write‑side file types |
| `read_small_file` / `write_small_file` | one‑shot read/write for small files |

```cpp
fermat::RandomReadFile f;
TURBO_RETURN_IF_ERROR(f.open(path));
turbo::span<char> buf = ...;
TURBO_ASSIGN_OR_RETURN(size_t n, f.pread(buf, offset));
```

Asynchronous disk I/O is not yet in the core library.

### Zero‑copy helpers

#### Receiver and container adapters

`Receiver` is an abstract interface to append bytes to a container. `ContainerAppender` wraps an existing container reference; `ContainerReceiver` owns a container instance and can `release()` it.

```cpp
std::string target;
fermat::ContainerAppender appender(target);
appender.reserve(1024);
appender.append("hello", 5);
```

#### Peeker: zero‑copy cross‑block iteration

`Peeker` sequentially reads from a `CordBufferBase` or a contiguous string, returning `string_view` chunks without copying across blocks. Supports `readn()`, `find_first_offset`, `seek_to`, etc.

```cpp
fermat::CordBufferBase<64, 4096> buf;
buf.append("Hello world");
fermat::Peeker peeker(&buf);
while (auto chunk = peeker.readn(1024)) {
    process(*chunk);
}
```

#### Customer / Reader: consume data from CordBuffer

- **Customer** (`FlattenCustomer<true>`): reads and removes consumed bytes.
- **Reader** (`FlattenCustomer<false>`): copies only, does not modify source.

```cpp
fermat::CordBufferBase<64, 4096> source;
// ... fill data ...
std::string target;
fermat::ContainerAppender appender(target);
fermat::Customer::custom(source, appender, 4096);
fermat::Reader::custom_until(source, appender, '\n');
```

## Building and dependencies

This project uses [kmpkg](https://github.com/kumose/kmcmake) for dependency management and build integration.

**Core library** (`fermat`) links: `turbo`, `mimalloc`, `Threads`. Additional dependencies for tests and benchmarks are in [`kmpkg.json`](kmpkg.json).

### 0. Environment

- Linux (Ubuntu 20.04+ / CentOS 7+ recommended)
- CMake >= 3.25
- GCC >= 9.4 / Clang >= 12
- `kmpkg` installed (see [installation docs](https://kumo-pub.github.io/docs/category/%E6%8C%81%E7%BB%AD%E9%9B%86%E6%88%90----kmpkg))

### 1. Configure (optional)

- Full dependency list in [`kmpkg.json`](kmpkg.json)
- To update dependency baseline, modify `baseline` of `default-registry` in [`kmpkg-configuration.json`](kmpkg-configuration.json)
- You can also manage dependencies manually, ensuring `find_package` can locate required libraries.

### 2. Build

```bash
cmake --preset=default
cmake --build build -j$(nproc)
```

Manual dependency management:

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. Run tests (optional)

```bash
ctest --test-dir build
```

### 4. Run benchmarks (optional)

Requires `KMCMAKE_BUILD_BENCHMARK` enabled. Executables are under `build/benchmark/`, e.g.:

```bash
./build/benchmark/mem_string_bench --benchmark_format=console
./build/benchmark/mem_iobuf_benchmark --benchmark_filter=52428800
```

Full raw data and historical results can be found in [`benchmark/README.md`](benchmark/README.md).

## License

Apache License 2.0. See the LICENSE file.