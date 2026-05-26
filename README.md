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
| `KString` | construction, copy, move, append | 1.2x – 3x (>20%) |
| `Buffer<T>` | construction (all sizes), small push/iteration/middle insert‑erase, clear (Release) | 2x – 5x |
| `Vector<T>` | same allocation path as `Buffer`; random access on par or faster than `std` in Release | 2x – 5x |
| `Deque<T>` | construction, push_front/push_back, clear, destruct | 2x – 10x |
| `List<T>` | all operations | 2x – 20x |
| `PriorityQueue<T>` | large push/pop (10k+), construction from iterators | 10% – 30% |
| `Stack<T>` | all operations | 20% – 10x (construction) |
| `Bitset` | construction, set/reset/flip, count, find series | 1.5x – 5x |
| `LruCache` (turbo_map + fermat_list) | insert, lookup, update, erase, touch | 20% – 50% |
| `ObjectPool` / `ResourcePool` | single‑/multi‑threaded alloc/free | 5x – 10x |
| `RadixSort` | random integer sorting | 5x – 10x |
| `CordBufferBase` | random chunked append (10 KiB – 100 MiB) | 1.5x – 5x |
| `CordBinaryStream` | binary serialization write/read/round‑trip | 20% – 100% |
| `CordStringStream` | text ping‑pong, round‑trip | 20% – 50% |

### ⚠️ Significant disadvantage (prefer `std`)

| Component | Weak operations | Slower than `std` | Reason |
|-----------|-----------------|--------------------|--------|
| `KString` | short‑string `find` | ~30% | different implementation path from `std::string` |
| `Deque<T>` | random access | ~10% | need to compute block index |
| `Deque<T>` | middle insert/erase | ~2× | block structure causes more data movement |
| `PriorityQueue<T>` | push/pop for small sizes (1000 elements) | ~2× | `std` algorithm better for small heaps |
| `CordInputStringStream` | token read (`>> string`) | ~20–30% | need cross‑block search |
| `CordInputStringStream` | bulk read (`read`) | ~2× | multiple cross‑block copies |
| `CordFormatter` | text formatting | ~6× | currently outputs char‑by‑char, no batch write |
| `Bitset` | bitwise ops (`&`, `|`, `^`), shift | ~10–20% | `std::bitset` better compiler optimization |

> **Note**:
> - `FermatList` is faster than `std::list` in all operations – no disadvantage.
> - `FermatStack` is faster than `std::stack` in all operations – no disadvantage.
> - `KString` is significantly faster in construction, copy, move, append; short‑string `find` is slightly slower than `std::string`.
> - `Buffer<T>` (plain data such as `int`/`float`) and `Vector<T>` (general contiguous container) have no significant disadvantage vs `std::vector` in **Release** mode after recent optimizations; small‑size random access is now roughly on par with `std`.

## When to use which component

- **Only replace `std` with a fermat component when its advantage in your scenario is significant (>20%) and its disadvantage is acceptable (<10%).** Otherwise prefer the standard library.
- If the bottleneck is not here, there is no need to replace.

| Scenario | Recommended component | Notes |
|----------|-----------------------|-------|
| Many string constructions / copies / moves | `KString` | 1.3–3× faster than `std::string`; short‑string `find` slightly slower |
| Streaming large data (logs, network packets) | `CordBufferBase` | extremely high random‑chunk append throughput (best in 10 KiB–100 MiB) |
| Filling chunked data from disk/network | `CordBufferBase` + `append_by_*` | seamless integration with `IOReader`, zero‑copy |
| Performance‑sensitive small/medium contiguous arrays | `Buffer<T>` / `Vector<T>` | use `Buffer` for plain POD; `Vector` for full `std::vector` semantics. Release: many ops ≥20% faster; large push_back roughly on par |
| Ordered read‑only / bulk construction of maps/sets | `VectorMap` / `VectorSet` | ordered insert 3.8× faster, iteration 24× faster; random insert slower, not for frequent modification |
| Frequent stack operations | `FermatStack` | construction from container ~11× faster, push/pop ~1.2× faster, no disadvantage |
| Priority queue with priority change/remove | `FermatPriorityQueue` | supports `change`/`remove`, large push/pop 30% faster; for small (<1000) slower than `std`, trade‑off |
| Small object pooling | `ObjectPool` | 5–10× faster than `new`/`delete` |
| High‑concurrency resource reuse (with versioning) | `ResourcePool` | thread‑safe, 16 threads ~394 ns/op |
| Local LRU cache | `LruCache` + `turbo::flat_hash_map` + `fermat::List` | 20–50% faster than `std::map`+`std::list` in all operations |
| Large integer sorting (random data) | `RadixSort` | 5–10× faster than `std::sort`; for already‑sorted or reverse‑sorted data, `std::sort` is faster |
| Finding next set bit in a bitset | `fermat::Bitset` | `FindNext` 4× faster than `std::bitset`; bitwise ops slightly slower, avoid heavy use |
| Synchronous file I/O | `sys_*` / `*ReadFile` / `*WriteFile` | cross‑platform abstraction, error handling with `turbo::Result` |
| Zero‑copy cross‑block traversal of CordBuffer | `Peeker` | returns `string_view`, no copy |
| Consuming data from CordBuffer into a container | `Customer` / `Reader` | supports copy‑or‑move consumption |
| Dynamic random map / set | `std::map` or `turbo::flat_hash_map` | fermat does not provide a high‑performance random map |
| Pure heavy reading of continuous data | `std::vector` + `std::istringstream` | contiguous memory reads are faster |
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
| Construct (Long) | **14.4 ns** | 41.3 ns | 2.9× |
| Copy (Long) | **25.8 ns** | 31.6 ns | 1.2× |
| Move (Long) | **14.9 ns** | 43.7 ns | 2.9× |
| Append (Medium) | **3.75 ns** | 4.38 ns | 1.2× |
| Find (Short) | 10.8 ns | **8.13 ns** | slightly slower |
| Find (Long) | **92.6 ns** | 107 ns | 1.2× |
| Hash (Long) | **229 ns** | 232 ns | comparable |

Strategy: conservative growth. For large accumulated data, pre‑`reserve()`.

```cpp
fermat::KString s = "hello";
s.reserve(10 * 1024 * 1024);
for (int i = 0; i < 1000000; ++i) s.append("a");
```

### Contiguous container `Buffer<T>`

`fermat::Buffer<T>` is a contiguous container specialized for plain data (e.g., `int`, `float`). Its interface is compatible with `std::vector`, but construction and clearing are faster. Release mode performance data (Intel 12‑core @ 4.4 GHz, L3 18 MiB). Better per row in **bold**.

| Operation | Size | std::vector<int> (ns) | fermat::Buffer<int> (ns) | Winner |
|-----------|------|----------------------|--------------------------|--------|
| **Construct** | 4 | 14.2 | **4.77** | fermat |
| | 8 | 14.1 | **4.71** | fermat |
| | 16 | 13.6 | **4.71** | fermat |
| | 32 | 13.9 | **4.91** | fermat |
| | 64 | 14.0 | **5.86** | fermat |
| | 128 | 14.5 | **6.97** | fermat |
| | 256 | 15.7 | **8.84** | fermat |
| | 512 | 39.1 | **12.7** | fermat |
| | 1024 | 50.0 | **21.4** | fermat |
| **PushBackSmall** | 4 | 14.5 | **5.61** | fermat |
| | 8 | 15.3 | **7.81** | fermat |
| | 16 | 17.4 | **14.0** | fermat |
| | 32 | **20.7** | 23.5 | std |
| | 64 | **39.9** | 39.6 | std |
| | 128 | **65.0** | 69.3 | std |
| | 256 | 143 | **138** | fermat |
| | 512 | 286 | **268** | fermat |
| | 1024 | 571 | **516** | fermat |
| **IterationSmall** | 4 | 15.9 | **5.57** | fermat |
| | 8 | 17.9 | **10.1** | fermat |
| | 16 | 19.5 | **12.9** | fermat |
| | 32 | **26.8** | 34.0 | std |
| | 64 | 34.7 | **31.5** | fermat |
| | 128 | 54.0 | **47.0** | fermat |
| | 256 | 94.9 | **83.8** | fermat |
| | 512 | 191 | **152** | fermat |
| | 1024 | 340 | **305** | fermat |
| **RandomAccessSmall** | 4 | 181 | **183** | std |
| | 8 | 186 | **180** | fermat |
| | 16 | 189 | **178** | fermat |
| | 32 | 179 | **178** | std |
| | 64 | 174 | **177** | std |
| | 128 | 178 | **174** | fermat |
| | 256 | 179 | **183** | std |
| | 512 | 177 | **181** | std |
| | 1024 | 177 | **178** | std |
| **InsertMiddleSmall** | 4 | 73.5 | **42.9** | fermat |
| | 8 | 66.3 | **43.9** | fermat |
| | 16 | 55.1 | **52.7** | fermat |
| | 32 | 59.4 | **57.2** | fermat |
| | 64 | 77.3 | **73.2** | fermat |
| | 128 | 102 | **73.6** | fermat |
| | 256 | 135 | **110** | fermat |
| | 512 | 188 | **148** | fermat |
| | 1024 | 270 | **258** | fermat |
| **EraseMiddleSmall** | 4 | 20.5 | **11.8** | fermat |
| | 8 | 28.4 | **22.7** | fermat |
| | 16 | 46.1 | **41.4** | fermat |
| | 32 | **52.4** | 53.4 | std |
| | 64 | 59.7 | **56.0** | fermat |
| | 128 | 66.2 | **66.4** | std |
| | 256 | 87.2 | **83.6** | fermat |
| | 512 | 135 | **107** | fermat |
| | 1024 | 180 | **165** | fermat |
| **ClearShrinkSmall** | 4 | 14.5 | **10.2** | fermat |
| | 8 | 14.5 | **9.83** | fermat |
| | 16 | 14.3 | **10.1** | fermat |
| | 32 | 14.9 | **10.9** | fermat |
| | 64 | 15.1 | **10.7** | fermat |
| | 128 | 17.4 | **12.8** | fermat |
| | 256 | 18.6 | **14.6** | fermat |
| | 512 | 39.4 | **18.3** | fermat |
| | 1024 | 48.6 | **31.7** | fermat |
| **PushBack** | 1000 | 489 | **469** | fermat |
| | 10000 | **4725** | 4711 | std |
| | 100000 | **54065** | 55949 | std |
| **Iteration** | 1000 | **247** | 248 | std |
| | 10000 | 2389 | **2355** | fermat |
| | 100000 | 23770 | **23646** | fermat |
| **RandomAccess** | 10000 | 1713 | **1710** | fermat |
| | 100000 | **1730** | 1758 | std |
| **InsertMiddle** | 1000 | 1505 | **1503** | fermat |
| | 10000 | 13498 | **13017** | fermat |
| **EraseMiddle** | 1000 | 1379 | **1362** | fermat |
| | 10000 | **462606** | 463187 | std |
| **Sort** | 10000 | 342928 | **342581** | fermat |
| | 100000 | 4533225 | **4500183** | fermat |
| **ClearAndShrink** | 10000 | **686** | 709 | std |
| | 100000 | 6778 | **6694** | fermat |

```cpp
fermat::Buffer<int> v;
v.reserve(1000);
v.push_back(42);
```

### Contiguous container `Vector<T>`

`fermat::Vector<T>` is interface‑compatible with `std::vector` and supports non‑trivial types; it shares the optimized storage/allocation path with `Buffer`. **Release** data below (same machine as above). Full tables: [`benchmark/README.md`](benchmark/README.md) (Vector section).

| Operation | Size | std::vector<int> (ns) | fermat::Vector<int> (ns) | Winner |
|-----------|------|----------------------|--------------------------|--------|
| **Construct** | 4 | 14.6 | **5.31** | fermat |
| | 1024 | 43.5 | **23.9** | fermat |
| **PushBackSmall** | 4 | 14.5 | **6.93** | fermat |
| | 1024 | 488 | **481** | fermat |
| **IterationSmall** | 4 | 15.4 | **5.70** | fermat |
| | 1024 | 301 | **273** | fermat |
| **RandomAccessSmall** | 4 | 174 | **174** | tie |
| | 1024 | **172** | 180 | std |
| **RandomAccess** | 10000 | 1739 | **1698** | fermat |
| | 100000 | 1732 | **1725** | fermat |
| **PushBack** | 1000 | 487 | **478** | fermat |
| **EmplaceBack** | 100000 | 54018 | **53590** | fermat |
| **Iteration** | 100000 | 23576 | **23511** | fermat |
| **InsertMiddle** | 10000 | 13698 | **12917** | fermat |
| **EraseMiddle** | 1000 | 1396 | **1367** | fermat |
| **ClearShrinkSmall** | 1024 | 43.0 | **25.5** | fermat |

```cpp
fermat::Vector<int> v;
v.reserve(1000);
v.emplace_back(42);
```

### Ordered associative containers VectorMap / VectorSet

Based on sorted `vector`, very fast ordered insertion and bulk construction, iteration much faster than `std::map`. Better per row in **bold**.

| Operation (1000) | VectorMap | std::map | Speedup |
|-----------------|-----------|----------|---------|
| Ordered insert | **9.2 µs** | 35.3 µs | 3.8× |
| Iteration | **0.15 ns/element** | 3.6 ns/element | ~24× |
| Random insert | 83.9 µs | **26.3 µs** | slower |

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
| top() | **0.374 ns** | 0.423 ns | 1.1× |
| construct from container (1000) | **20.2 ns** | 220 ns | 10.9× |
| push/pop (1000) | **759 ns** | 919 ns | 1.2× |

```cpp
fermat::stack<int, 0> st;
st.push(1);
int v = st.top();
st.pop();
```

### Priority queue FermatPriorityQueue

Supports `change` / `remove` (can modify elements via `get_container()`). Better per row in **bold**.

| Operation | FermatPriorityQueue | std::priority_queue | Speedup |
|-----------|---------------------|----------------------|---------|
| push/pop (1000) | 22.6 µs | **10.8 µs** | ~2× slower |
| push/pop (10000) | **602 µs** | 649 µs | 1.1× |
| push/pop (100000) | **7.72 ms** | 8.12 ms | 1.1× |
| construct from iterators (1000) | **1.87 µs** | 2.13 µs | 1.1× |
| change/remove (1000) | supported | not supported | – |

```cpp
fermat::PriorityQueue<int, 0> pq;
pq.push(10);
size_t idx = 0;
pq.get_container()[idx] = 20;
pq.change(idx);
pq.remove(idx);
```

### Object pool ObjectPool

Single‑thread allocation/deallocation ~6× faster than `new`/`delete`. Better per row in **bold**.

| Operation | ObjectPool | new/delete | Speedup |
|-----------|------------|------------|---------|
| single thread | **2.02 ns** | 11.7 ns | 5.8× |
| 16 threads | **3.77 ns** | 29.0 ns | 7.7× |

```cpp
MyClass* obj = fermat::ObjectPool<MyClass>::get(/* ctor args */);
fermat::ObjectPool<MyClass>::put(obj);
```

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
| **Insert distinct** | 1000 | 81.0 µs | 59.2 µs | 43.0 µs | **31.5 µs** |
| | 10000 | 869 µs | 729 µs | 514 µs | **406 µs** |
| | 100000 | 11.57 ms | 8.28 ms | 6.82 ms | **5.77 ms** |
| **Lookup high hit** | 1000 | 45.0 ns | 31.0 ns | 28.3 ns | **21.4 ns** |
| | 10000 | 51.1 ns | 37.2 ns | 31.8 ns | **25.9 ns** |
| | 100000 | 91.7 ns | 45.7 ns | 40.7 ns | **32.9 ns** |
| **Lookup low hit** | 1000 | 114 ns | 96.8 ns | 85.8 ns | **72.3 ns** |
| | 10000 | 126 ns | 107 ns | 106 ns | **90.1 ns** |
| | 100000 | 149 ns | 118 ns | 127 ns | **114 ns** |
| **Update (assign)** | 1000 | 33.9 ns | 23.6 ns | 22.6 ns | **16.2 ns** |
| | 10000 | 42.4 ns | 29.4 ns | 25.6 ns | **21.0 ns** |
| | 100000 | 66.5 ns | 38.6 ns | 33.3 ns | **27.8 ns** |
| **Erase all** | 1000 | 73.9 µs | 56.2 µs | 42.8 µs | **29.1 µs** |
| | 10000 | 808 µs | 717 µs | 485 µs | **398 µs** |
| | 100000 | 9.88 ms | 8.22 ms | 6.42 ms | **5.56 ms** |
| **touch** | 1000 | 32.9 ns | 22.6 ns | 21.9 ns | **16.2 ns** |
| | 10000 | 39.5 ns | 29.6 ns | 25.2 ns | **21.3 ns** |
| | 100000 | 55.6 ns | 38.7 ns | 31.4 ns | **27.9 ns** |

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
| 1M random u32 | **5.34 ms** | 53.7 ms | 10.1× |

```cpp
std::vector<uint32_t> data = {5, 2, 8, 1};
fermat::radix_sort(data.begin(), data.end());
```

### Bitset

`FindNext` for 1024 bits is faster than `std::bitset` (`mem_bitset_bench`):

| Operation (1024 bit) | fermat | std::bitset | Speedup |
|----------------------|--------|-------------|---------|
| find_next | **0.387 ns** | 1.56 ns | 4.0× |

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