# fermat – High-Performance C++ Foundation Library

[中文版](README_CN.md)

fermat is a modern C++ library focused on **high throughput and low latency**, providing optimized implementations for strings, chunked buffers, contiguous containers, ordered associative containers, stacks, priority queues, memory pools, caches, radix sort, and zero-copy I/O helpers (`Peeker`, `Receiver`, `Customer`/`Reader`). Compared to the standard library, fermat achieves **2–10x** performance improvements in many scenarios.

## Modules and Performance Data

The following data is collected on Intel Xeon platforms; actual performance may vary slightly with environment.

### String `KString`

`fermat::BasicString` (alias `KString`) outperforms `std::string` in all common operations:

| Operation          | KString      | std::string  | Speedup |
|--------------------|--------------|--------------|---------|
| Construct (Long)   | 15.3 ns      | 44.1 ns      | 2.9x    |
| Copy (Long)        | 27.0 ns      | 34.8 ns      | 1.3x    |
| Move (Long)        | 19.1 ns      | 49.4 ns      | 2.6x    |
| Append (Medium)    | 3.79 ns      | 4.48 ns      | 1.2x    |
| Find (Long)        | 93.3 ns      | 106 ns       | 1.1x    |
| Hash (Long)        | 227 ns       | 231 ns       | –       |

**Strategy**: conservative growth. For accumulating large data (tens of MB), call `reserve()` in advance.

```cpp
fermat::KString s = "hello";
s.reserve(10 * 1024 * 1024);
for (int i = 0; i < 1000000; ++i) s.append("a");
```

### Chunked Buffers `CordBufferBase` / `IOBuf`

Random chunked append (16KB block size), throughput at 50MB:

| Container        | Throughput     |
|------------------|----------------|
| `CordBufferBase`     | 13.87 GiB/s    |
| `IOBuf`          | 9.56 GiB/s     |
| `std::string`    | 2.08 GiB/s     |

- **CordBufferBase**: writes are immediately visible, no state machine. Suitable for streaming append, logging, network receiving.
- **IOBuf**: supports `borrow()` / `commit()` for atomic commits; uncommitted data is not visible to reads. Suitable for `readv` and transactional writes.

```cpp
fermat::CordBufferBase<64, 16*1024> cb;
cb.append("data", 4);

fermat::IOBuf<64, 16*1024> ib;
auto lease = ib.borrow(4096).value();
lease->write("hello", 5);
ib.commit(lease);
```

### Contiguous Container `Buffer<T>`

Interface compatible with `std::vector`, faster construction and clearing:

| Operation                           | fermat::Buffer<int> | std::vector<int> | Speedup |
|-------------------------------------|---------------------|------------------|---------|
| Construct 1024 elements             | 22.1 ns             | 55.5 ns          | 2.5x    |
| `push_back` 1000 times              | 467 ns              | 486 ns           | ~       |
| Iterate 1000 elements               | 245 ns              | 244 ns           | ~       |
| Random access 10000                 | 1718 ns             | 1702 ns          | ~       |

```cpp
fermat::Buffer<int> v;
v.reserve(1000);
v.push_back(42);
```

### Ordered Associative Containers `VectorMap` / `VectorSet`

Based on sorted `std::vector`, extremely fast ordered insertion and batch construction; iteration is 20× faster than `std::map`.

| Operation (1000 elements) | VectorMap   | std::map    | Speedup |
|---------------------------|-------------|-------------|---------|
| Ordered insert            | 9.2 µs      | 35.3 µs     | 3.8x    |
| Iteration                 | 0.15 ns/elem| 3.6 ns/elem | 24x     |
| Random insert             | 83.9 µs     | 26.3 µs     | slower  |

```cpp
fermat::VectorMap<int, std::string> m;
m.insert({1, "one"});
auto it = m.find(1);
```

### Stack `FermatStack`

Extremely fast `top()`; constructing from a container is 10× faster than `std::stack`.

| Operation                         | FermatStack | std::stack | Speedup |
|-----------------------------------|-------------|------------|---------|
| `top()`                           | 0.287 ns    | 0.404 ns   | 1.4x    |
| Construct from container (1000)   | 20.7 ns     | 223 ns     | 10.8x   |
| `push`/`pop` (1000)               | 828 ns      | 926 ns     | 1.1x    |

```cpp
fermat::Stack<int> st;
st.push(1);
int v = st.top();
st.pop();
```

### Priority Queue `FermatPriorityQueue`

Supports `increase_key` / `decrease_key` / `erase`.

| Operation                         | FermatPriorityQueue | std::priority_queue | Speedup |
|-----------------------------------|---------------------|----------------------|---------|
| `push`/`pop` (1000)               | 19 µs               | 37.4 µs              | 1.97x   |
| Construct from iterators (1000)   | 1.67 µs             | 2.09 µs              | 1.25x   |
| `change`/`remove` (1000)          | 1.83 µs             | not supported        | –       |

```cpp
fermat::PriorityQueue<int> pq;
pq.push(10);
pq.increase_key(10, 20);
pq.erase(10);
```

### Object Pool `ObjectPool`

Single-threaded allocation/deallocation is 6× faster than `new`/`delete`.

| Operation          | ObjectPool | new/delete | Speedup |
|--------------------|------------|------------|---------|
| Single‑threaded    | 2.02 ns    | 11.8 ns    | 5.8x    |
| 16 threads         | 3.91 ns    | 30.9 ns    | 7.9x    |

```cpp
fermat::ObjectPool<MyClass> pool;
auto* obj = pool.allocate();
pool.deallocate(obj);
```

### Sharded Resource Pool `ShardedPool`

High‑concurrency sharded pool.

| Operation                     | Time                       |
|-------------------------------|----------------------------|
| Single‑thread `alloc`/`free`  | 14.9 ns (66.95 M ops/s)   |
| 16‑thread `get`/`put`         | 438 ns (36.56 M ops/s)    |

```cpp
ShardedPool<MyClass, 64> pool;
auto* p = pool.get();
pool.put(p);
```

### Cache `TurboCache`

High‑throughput local LRU cache; 30–50% faster than a standard cache based on `unordered_map`.

| Operation (100k)          | TurboCache | StdCache | Speedup |
|---------------------------|------------|----------|---------|
| Insert distinct           | 6.82 ms    | 9.68 ms  | 1.42x   |
| Lookup high‑hit           | 37 ns      | 66.3 ns  | 1.79x   |
| Update                    | 35.1 ns    | 67.3 ns  | 1.92x   |
| Touch (iterate)           | 31.1 ns    | 56.7 ns  | 1.82x   |

```cpp
fermat::TurboCache<int, std::string> cache(1000);
cache.insert(1, "one");
auto val = cache.find(1);
cache.touch(1);
cache.erase(1);
```

### Radix Sort `RadixSort`

Sorting `uint32_t` is 5–10× faster than `std::sort`.

| Dataset (random u32) | RadixSort | std::sort | Speedup |
|----------------------|-----------|-----------|---------|
| 1 million            | 5.22 ms   | 59.1 ms   | 11.3x   |

```cpp
std::vector<uint32_t> data = {5, 2, 8, 1};
fermat::radix_sort(data.begin(), data.end());
```

### I/O Helpers

#### `Receiver` and Container Adapters

`Receiver` is an abstract interface for appending data to a container with capacity reservation. Two adapters are provided:

- `ContainerAppender<Container>`: wraps an existing container reference.
- `ContainerReceiver<Container>`: owns a container instance; can be retrieved via `release()`.

```cpp
std::string target;
fermat::ContainerAppender appender(target);
appender.reserve(1024);
appender.append("hello", 5);
```

#### `Peeker`: Zero‑Copy Cross‑Block Iterator

`Peeker` can sequentially read data from an `IOBuf` or a contiguous string, returning `string_view` without copying across blocks. It supports `readn()`, `find_first_offset`, `seek_to`, etc.

```cpp
fermat::IOBuf<64, 4096> buf;
buf.append("Hello world");
fermat::Peeker peeker(&buf);
while (auto chunk = peeker.readn(1024)) {
    process(*chunk);
}
```

#### `Customer` / `Reader`: Consuming Data from `IOBuf`

- **`Customer`** (`FlattenCustomer<true>`): reads and physically removes bytes from the source (like `pop_front`).
- **`Reader`** (`FlattenCustomer<false>`): copies data without modifying the source.

```cpp
fermat::IOBuf<> source;
// ... fill source ...
std::string target;
fermat::ContainerAppender appender(target);
fermat::Customer::custom(source, appender, 4096);        // consume 4096 bytes
fermat::Reader::custom_until(source, appender, '\n');    // read up to newline
```

## Quick Selection Guide

| Requirement                                           | Recommended Component          |
|-------------------------------------------------------|--------------------------------|
| General strings (reserve for large accumulation)     | `KString`                      |
| Streaming large data (writes visible immediately)    | `CordBufferBase`                   |
| Atomic commit large writes (e.g., `readv`)            | `IOBuf`                        |
| Zero‑copy cross‑block iteration over `IOBuf`          | `Peeker<IOBuf>`                |
| Consume data from `IOBuf` into a container            | `Customer` / `Reader`          |
| Small/medium contiguous array (performance‑sensitive) | `Buffer<T>`                    |
| Ordered read‑only / batch‑built map/set               | `VectorMap` / `VectorSet`      |
| Dynamic random map/set                                | `std::map` / `std::set`        |
| High‑frequency stack operations                       | `FermatStack`                  |
| Priority queue with key modification                  | `FermatPriorityQueue`          |
| Small object pooling                                  | `ObjectPool`                   |
| High‑concurrency resource reuse                       | `ShardedPool`                  |
| High‑throughput local cache                           | `TurboCache`                   |
| Large integer sorting                                 | `RadixSort`                    |

## Build and Dependencies

This project uses [kmpkg](https://github.com/kumose/kmcmake) for dependency management and build integration. kmpkg automatically handles third‑party library downloads, dependency resolution, and compiler flags, avoiding manual CMake configuration hassles.

### 0. Environment Setup

- Linux (Ubuntu 20.04+ / CentOS 7+ recommended)
- CMake >= 3.25
- GCC >= 9.4 / Clang >= 12
- `kmpkg` installed (see [installation guide](https://kumo-pub.github.io/docs/category/%E6%8C%81%E7%BB%AD%E9%9B%86%E6%88%90----kmpkg))

### 1. Project Configuration (Optional)

- Full dependencies are listed in [`kmpkg.json`](kmpkg.json)
- To update the dependency baseline, modify `default-registry.baseline` in [`kmpkg-configuration.json`](kmpkg-configuration.json)
- The baseline can be obtained from the latest commit in the dependency registry via `git log`
- Alternatively, users can manage dependencies manually by ensuring `find_package` works (set `CMAKE_PREFIX_PATH` as needed)

### 2. Build the Project

From the project root:

```bash
cmake --preset=default
cmake --build build -j$(nproc)
```

Manual dependency management:

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

Note: `--preset=default` requires a corresponding CMake preset defined in the project root.

### 3. Run Tests (Optional)

```bash
ctest --test-dir build
```

## License

Apache License 2.0. See the LICENSE file for details.