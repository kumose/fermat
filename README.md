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
| `Buffer<T>` | construction (all sizes), small push_back (≤512) | 2x – 5x |
| `Deque<T>` | construction, push_front/push_back, clear, destruct | 2x – 10x |
| `List<T>` | all operations | 2x – 20x |
| `Vector<T>` | construction, small push_back (≤512), iteration, clear | 1.5x – 10x |
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
| `Vector<T>` | random access (small capacity gap larger; at 10k+ ~10–15%) | up to several times for tiny sizes; ~10–15% for large | internal bounds checking overhead |
| `KString` | short‑string `find` | ~30% | different implementation path from `std::string` |
| `Deque<T>` | random access | ~10% | need to compute block index |
| `Deque<T>` | middle insert/erase | ~2× | block structure causes more data movement |
| `PriorityQueue<T>` | push/pop for small sizes (1000 elements) | ~2× | `std` algorithm better for small heaps |
| `CordInputStringStream` | token read (`>> string`) | ~20–30% | need cross‑block search |
| `CordInputStringStream` | bulk read (`read`) | ~2× | multiple cross‑block copies |
| `CordFormatter` | text formatting | ~6× | currently outputs char‑by‑char, no batch write |
| `Bitset` | bitwise ops (`&`, `|`, `^`), shift | ~10–20% | `std::bitset` better compiler optimization |

> **Note**: `FermatList` is faster than `std::list` in all operations – no disadvantage. `FermatStack` is faster than `std::stack` in all operations – no disadvantage. `KString` is significantly faster in construction, copy, move, append; short‑string `find` is slightly slower than `std::string`.

## When to use which component

- **Only replace `std` with a fermat component when its advantage in your scenario is significant (>20%) and its disadvantage is acceptable (<10%).** Otherwise prefer the standard library.
- If the bottleneck is not here, there is no need to replace.

| Scenario | Recommended component | Notes |
|----------|-----------------------|-------|
| Many string constructions / copies / moves | `KString` | 1.3–3× faster than `std::string`; short‑string `find` slightly slower |
| Streaming large data (logs, network packets) | `CordBufferBase` | extremely high random‑chunk append throughput (best in 10 KiB–100 MiB) |
| Filling chunked data from disk/network | `CordBufferBase` + `append_by_*` | seamless integration with `IOReader`, zero‑copy |
| Performance‑sensitive small/medium contiguous arrays | `Buffer<T>` | construction, small push_back 2–5× faster; large push_back roughly on par or slightly slower |
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

### Chunked buffer CordBufferBase

Random chunked append (`mem_iobuf_benchmark`, each chunk 16 KiB). fermat’s chunked container is **CordBufferBase**; brpc IOBuf, Abseil Cord, Turbo Cord are **reference implementations**. Data sizes are 1 KiB – 100 MiB (same as `benchmark/iobuf_benchmark.cc`). The highest throughput per row is **bolded**.

| Size | **CordBufferBase** | std::string | fermat::Buffer | brpc IOBuf | Abseil Cord | Turbo Cord |
|------|---------------------|-------------|----------------|------------|-------------|------------|
| 1 KiB | 17.9 GiB/s | **20.7 GiB/s** | 37.3 GiB/s | 19.8 GiB/s | 12.0 GiB/s | 12.6 GiB/s |
| 10 KiB | **52.7 GiB/s** | 21.2 GiB/s | 46.3 GiB/s | 31.5 GiB/s | 13.2 GiB/s | 12.5 GiB/s |
| 100 KiB | **46.8 GiB/s** | 20.6 GiB/s | 24.8 GiB/s | 29.7 GiB/s | 18.1 GiB/s | 16.4 GiB/s |
| 1 MiB | **44.5 GiB/s** | 12.3 GiB/s | 18.1 GiB/s | 28.3 GiB/s | 18.9 GiB/s | 17.7 GiB/s |
| 10 MiB | **31.1 GiB/s** | 8.01 GiB/s | 8.07 GiB/s | 22.1 GiB/s | 16.3 GiB/s | 15.6 GiB/s |
| 20 MiB | **17.6 GiB/s** | 4.06 GiB/s | 6.34 GiB/s | 13.8 GiB/s | 10.9 GiB/s | 10.9 GiB/s |
| 50 MiB | **12.96 GiB/s** | 2.07 GiB/s | 6.16 GiB/s | 11.1 GiB/s | 8.88 GiB/s | 8.40 GiB/s |
| 100 MiB | **12.15 GiB/s** | 1.52 GiB/s | 6.00 GiB/s | 11.2 GiB/s | 8.61 GiB/s | 8.30 GiB/s |

> CordBufferBase performs best or near‑best in the 10 KiB–100 MiB range, especially from 10 KiB to 10 MiB.

CordBufferBase writes are immediately visible, no state machine, suitable for streaming append, logging, network packet assembly; can integrate with disk reads via `IOReader` (`append_by_read` / `append_by_pread`).

```cpp
fermat::CordBufferBase<64, 16 * 1024> cb;
cb.append("data", 4);
```

### Contiguous container Buffer\<T\>

Interface compatible with `std::vector`, faster construction and clearing. Better per row in **bold**.

| Operation | fermat::Buffer\<int\> | std::vector\<int\> | Speedup |
|-----------|----------------------|-------------------|---------|
| Construct 1024 elements | **22.6 ns** | 50.1 ns | 2.2× |
| append 1000 times | **471 ns** | 489 ns | 1.0× |
| iterate 1000 elements | **246 ns** | 253 ns | comparable |
| clear + shrink 10000 | **685 ns** | 715 ns | 1.0× |

```cpp
fermat::Buffer<int> v;
v.reserve(1000);
v.push_back(42);
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