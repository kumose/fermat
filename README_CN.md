# fermat – 高性能 C++ 基础库

[English](README.md)

fermat 是一套专注于**高吞吐、低延迟**的 C++ 基础组件库，提供优化的字符串、分块缓冲区、连续容器、有序关联容器、栈、优先队列、内存池、LRU 缓存、基数排序、同步磁盘 IO，以及零拷贝辅助工具（`Peeker`、`Receiver`、`Customer`/`Reader`）。相比标准库，fermat 在多种场景下可取得 **2~10 倍**性能提升。

除微基准上的加速外，fermat 面向 **AI / 向量数据** 等重负载场景还有三项设计取向，潜在收益是**在实际运行中减少大量内存拷贝与格式转换**：

1. **全尺寸对齐分配**：`KString`、`Buffer<T>`、`Vector<T>` 等连续容器支持模板参数指定对齐（配合分级池化分配器），从小对象到大块缓冲均可按所需对齐申请内存，便于与 SIMD、向量化算子或下游张量布局对接，**尽量少做因对齐不足而触发的重排或二次拷贝**。
2. **`seize` / `bestow` 移交所有权**：连续容器提供 `seize()` 取出底层指针（容器置空且**不释放**内存）与 `bestow(ptr, size, capacity)` 接管外部已分配块（要求容器为空；`Alignment != 0` 时校验指针对齐）。`KString`、`Buffer`、`Vector` 均支持，便于与外部分配器、推理框架或 GPU 侧缓冲对接，**避免「拷进容器再拷出」**。
3. **分段共享、少拷贝**：`CordBufferBase` 的 `share()` / `BufferRef`、`borrow()` 可写尾部、`Peeker` 的 `string_view` 遍历，以及 `append_reference` / `Customer` / `Reader` 等，让非连续或跨模块的大块向量数据**传视图、拼引用而非复制 payload**——AI 应用中常见的大块 embedding、batch 缓冲尤其适合。

若业务以「拷贝字节数」计成本，优先用 `size()` 界定逻辑长度、用分段共享传递数据，并在需要时 `reserve` + 对齐容器；详见下文「容器 `capacity` 语义」与零拷贝相关章节。

## 性能差异阈值

fermat 的目标是在**优势场景**下提供显著（通常 >20%）的性能提升，而在**劣势场景**下尽量将损失控制在可接受范围（<10%）。如果某个操作只比 `std` 快不到 10%，建议继续使用标准库，以保持兼容性和成熟度。下表仅列出**差异明显**的操作（优势 >20% 或劣势 >10%）；差异不显著的操作不在表中列出，可认为两者相当。

## 关键性能差异对比

### ✅ 显著优势（推荐使用 fermat）

| 组件 | 优势操作 | 典型提升 |
|------|----------|----------|
| `KString` | 构造、拷贝、移动、追加 | 1.2x ~ 3x (>20%) |
| `Buffer<T>` | 构造（所有规模）、小规模 push_back（≤ 512） | 2x ~ 5x |
| `Deque<T>` | 构造、两端 push、clear、析构 | 2x ~ 10x |
| `List<T>` | 所有操作 | 2x ~ 20x |
| `Vector<T>` | 构造、小规模 push_back（≤ 512）、迭代、clear | 1.5x ~ 10x |
| `PriorityQueue<T>` | 大规模 push/pop（10k+）、从迭代器构造 | 10% ~ 30% |
| `Stack<T>` | 所有操作 | 20% ~ 10x（构造） |
| `Bitset` | 构造、set/reset/flip、count、find 系列 | 1.5x ~ 5x |
| `LruCache`（turbo_map + fermat_list） | 插入、查询、更新、删除、touch | 20% ~ 50% |
| `ObjectPool` / `ResourcePool` | 单/多线程分配释放 | 5x ~ 10x |
| `RadixSort` | 随机整数排序 | 5x ~ 10x |
| `CordBufferBase` | 随机分块追加（10 KiB ~ 100 MiB） | 1.5x ~ 5x |
| `CordBinaryStream` | 二进制序列化写/读/往返 | 20% ~ 100% |
| `CordStringStream` | 文本 ping‑pong、往返 | 20% ~ 50% |

### ⚠️ 显著劣势（建议保留 std）

| 组件 | 劣势操作 | 比 std 慢 | 原因 |
|------|----------|-----------|------|
| `Vector<T>` | 随机访问（小容量差距更大；10k+ 约 10% ~ 15%） | 小容量可达数倍；大容量约 10% ~ 15% | 内部边界检查开销 |
| `KString` | 短串查找 (`find`) | 约 30% | 与 `std::string` 实现路径不同 |
| `Deque<T>` | 随机访问 | 约 10% | 需计算块索引 |
| `Deque<T>` | 中间插入/删除 | 约 2x | 分块结构导致数据移动多 |
| `PriorityQueue<T>` | push/pop 小规模（1000 元素） | 约 2x | 小堆时 std 算法更优 |
| `CordInputStringStream` | token 读取（>> string） | 约 20% ~ 30% | 需跨块查找 |
| `CordInputStringStream` | bulk read（read） | 约 2x | 多次跨块拷贝 |
| `CordFormatter` | 文本格式化 | 约 6x | 当前逐字符输出，未批量写入 |
| `Bitset` | 按位运算（&, \|, ^）、移位 | 约 10% ~ 20% | `std::bitset` 编译器优化更强 |

> **注意**：`FermatList` 所有操作均快于 `std::list`，无劣势。`FermatStack` 所有操作均快于 `std::stack`，无劣势。`KString` 在构造、拷贝、移动、追加等操作上明显更快；短串 `find` 略慢于 `std::string`。

## 场景选择指南

- **只有当 fermat 组件在您的场景中优势明显（>20%）且劣势可接受（<10%）时，才推荐替换 `std`。** 否则优先使用标准库。
- 如果核心瓶颈不在此处，无需刻意替换。

| 场景 | 推荐组件 | 原因与注意事项 |
|------|-----------|----------------|
| 大量字符串构造/拷贝/移动 | `KString` | 比 `std::string` 快 1.3~3x；短串 `find` 略慢 |
| 流式追加大数据（日志、网络收包） | `CordBufferBase` | 随机块追加吞吐量极高（10 KiB~100 MiB 区间最佳） |
| 从磁盘/网络填充分块数据 | `CordBufferBase` + `append_by_*` | 与 `IOReader` 无缝集成，零拷贝 |
| 中小型连续数组（性能敏感） | `Buffer<T>` | 构造、小规模 push_back 比 `std::vector` 快 2~5x；大批量 push_back 与 std 大体持平或略慢 |
| 有序只读 / 批量构建 map/set | `VectorMap` / `VectorSet` | 有序插入快 3.8x，迭代快 24x；随机插入慢，不适合频繁修改 |
| 高频栈操作 | `FermatStack` | 从容器构造快约 11x，push/pop 快约 1.2x，无劣势 |
| 支持优先级修改的优先队列 | `FermatPriorityQueue` | 支持 `change`/`remove`，且大规模 push/pop 比 `std::priority_queue` 快 30%；小规模（<1000）比 std 慢，注意权衡 |
| 小对象池化 | `ObjectPool` | 比 `new`/`delete` 快 5~10 倍 |
| 高并发资源复用（带版本号） | `ResourcePool` | 跨线程安全，16 线程约 394 ns/op |
| 本地 LRU 缓存 | `LruCache` + `turbo::flat_hash_map` + `fermat::List` | 所有操作比 `std::map`+`std::list` 快 20%~50% |
| 大量整数排序（随机数据） | `RadixSort` | 比 `std::sort` 快 5~10x；已排序或逆序数据 `std::sort` 更快 |
| 位图查找下一个置位位 | `fermat::Bitset` | `FindNext` 比 `std::bitset` 快 4x；按位运算略慢，避免高频使用 |
| 同步文件读写 | `sys_*` / `*ReadFile` / `*WriteFile` | 跨平台抽象，基于 `turbo::Result` 错误处理 |
| 零拷贝跨块遍历 CordBuffer | `Peeker` | 返回 `string_view`，不拷贝 |
| 从 CordBuffer 消费数据到容器 | `Customer` / `Reader` | 支持拷贝或移动消费 |
| 动态随机 map/set | `std::map` 或 `turbo::flat_hash_map` | fermat 未提供高性能随机 map |
| 纯读取大量数据（连续内存） | `std::vector` + `std::istringstream` | 连续内存读取更快 |
| 文本格式化（高吞吐） | `CordOutputStringStream` 或 `fmt::format_to` + 自定义 sink | 避免使用 `CordFormatter` |

## 容器 `capacity` 语义

`KString`、`Buffer<T>`、`Vector<T>` 等连续容器的约定如下：

- **逻辑数据边界**用 **`size()`**（或实际写入长度）。
- **当前可用存储上界**可以在当下用 **`capacity()`** 做约束（例如判断还能写入多少、在未扩容前做边界检查），但**不要缓存 `capacity()`**——每次需要时重新读取。

`reserve(n)` 之后，fermat 会尽量让已申请内存都可用，故通常有 **`capacity() >= n`**，小规模下还会按分级内存池（`good_size` / 池化档位）向上取整，**`capacity()` 可能大于 `n`**。这与 `std::vector` / `std::string` 的常见体验类似：实现上也会为减少分配次数而多留一些空间；差别在于 **标准库为语义一致性做了更强约束**（许多场景下 `reserve` 后 `capacity` 更接近请求值），而 **fermat 优先池化与减少分配次数**，不保证 `capacity` 与 `reserve` 参数一一对应。

因此：可以把 `capacity()` 当作**瞬时**的可用空间上界；不要把某次读到的 `capacity` 存进成员变量、协议字段或持久化状态——在 `reserve`、`shrink`、`clear` 或再次扩容后它会变，缓存后容易误判剩余空间。

## 性能数据详情

### 字符串 KString

`fermat::BasicString`（别名 `KString`）在构造、拷贝、移动、追加等操作上优于 `std::string`；短串 `find` 略慢。数据来自 [`benchmark/README.md`](benchmark/README.md)。每行中性能更优者以 **粗体** 标出。

| 操作 | KString | std::string | 加速比 |
|------|---------|-------------|--------|
| 构造 (Long) | **14.4 ns** | 41.3 ns | 2.9x |
| 拷贝 (Long) | **25.8 ns** | 31.6 ns | 1.2x |
| 移动 (Long) | **14.9 ns** | 43.7 ns | 2.9x |
| 追加 (Medium) | **3.75 ns** | 4.38 ns | 1.2x |
| 查找 (Short) | 10.8 ns | **8.13 ns** | 略慢 |
| 查找 (Long) | **92.6 ns** | 107 ns | 1.2x |
| 哈希 (Long) | **229 ns** | 232 ns | 持平 |

策略：保守扩容，累积大数据请预先 `reserve()`。

```cpp
fermat::KString s = "hello";
s.reserve(10 * 1024 * 1024);
for (int i = 0; i < 1000000; ++i) s.append("a");
```

### 分块缓冲区 CordBufferBase

随机分块追加（`mem_iobuf_benchmark`，每块 16 KiB）。fermat 分块容器为 **CordBufferBase**；brpc IOBuf、Abseil Cord、Turbo Cord 为 benchmark **对照项**。数据量档位与 `benchmark/iobuf_benchmark.cc` 一致（1 KiB～100 MiB）。每行中吞吐量最高的实现以 **粗体** 标出。

| 数据量 | **CordBufferBase** | std::string | fermat::Buffer | brpc IOBuf | Abseil Cord | Turbo Cord |
|--------|---------------------|-------------|----------------|------------|-------------|------------|
| 1 KiB | 17.9 GiB/s | **20.7 GiB/s** | 37.3 GiB/s | 19.8 GiB/s | 12.0 GiB/s | 12.6 GiB/s |
| 10 KiB | **52.7 GiB/s** | 21.2 GiB/s | 46.3 GiB/s | 31.5 GiB/s | 13.2 GiB/s | 12.5 GiB/s |
| 100 KiB | **46.8 GiB/s** | 20.6 GiB/s | 24.8 GiB/s | 29.7 GiB/s | 18.1 GiB/s | 16.4 GiB/s |
| 1 MiB | **44.5 GiB/s** | 12.3 GiB/s | 18.1 GiB/s | 28.3 GiB/s | 18.9 GiB/s | 17.7 GiB/s |
| 10 MiB | **31.1 GiB/s** | 8.01 GiB/s | 8.07 GiB/s | 22.1 GiB/s | 16.3 GiB/s | 15.6 GiB/s |
| 20 MiB | **17.6 GiB/s** | 4.06 GiB/s | 6.34 GiB/s | 13.8 GiB/s | 10.9 GiB/s | 10.9 GiB/s |
| 50 MiB | **12.96 GiB/s** | 2.07 GiB/s | 6.16 GiB/s | 11.1 GiB/s | 8.88 GiB/s | 8.40 GiB/s |
| 100 MiB | **12.15 GiB/s** | 1.52 GiB/s | 6.00 GiB/s | 11.2 GiB/s | 8.61 GiB/s | 8.30 GiB/s |

> 说明：CordBufferBase 在 10 KiB～100 MiB 范围内均表现最佳或接近最佳，尤其在 10 KiB～10 MiB 区间吞吐量显著领先。

CordBufferBase 写入即见、无状态机，适合流式追加、日志、网络收包；可通过 `IOReader` 接口与磁盘读取对接（`append_by_read` / `append_by_pread`）。

```cpp
fermat::CordBufferBase<64, 16 * 1024> cb;
cb.append("data", 4);
```

### 连续容器 Buffer\<T\>

接口兼容 `std::vector`，构造、清除更快。每行中性能更优者以 **粗体** 标出。

| 操作 | fermat::Buffer\<int\> | std::vector\<int\> | 加速比 |
|------|----------------------|-------------------|--------|
| 构造 1024 元素 | **22.6 ns** | 50.1 ns | 2.2x |
| append 1000 次 | **471 ns** | 489 ns | 1.0x |
| 迭代 1000 元素 | **246 ns** | 253 ns | 持平 |
| clear + shrink 10000 | **685 ns** | 715 ns | 1.0x |

```cpp
fermat::Buffer<int> v;
v.reserve(1000);
v.push_back(42);
```

### 有序关联容器 VectorMap / VectorSet

基于有序 `vector`，有序插入和批量构造极快，迭代速度远高于 `std::map`。每行中性能更优者以 **粗体** 标出。

| 操作 (1000) | VectorMap | std::map | 加速比 |
|-------------|-----------|----------|--------|
| 有序插入 | **9.2 µs** | 35.3 µs | 3.8x |
| 迭代 | **0.15 ns/元素** | 3.6 ns/元素 | ~24x |
| 随机插入 | 83.9 µs | **26.3 µs** | 更慢 |

随机插入、频繁 erase 请用 `std::map` / `turbo::flat_hash_map`。

```cpp
fermat::VectorMap<int, std::string> m;
m.insert({1, "one"});
auto it = m.find(1);
```

### 栈 FermatStack

`top()` 与从容器构造显著快于 `std::stack`。每行中性能更优者以 **粗体** 标出。

| 操作 | FermatStack | std::stack | 加速比 |
|------|-------------|------------|--------|
| top() | **0.374 ns** | 0.423 ns | 1.1x |
| 从容器构造 (1000) | **20.2 ns** | 220 ns | 10.9x |
| push/pop (1000) | **759 ns** | 919 ns | 1.2x |

```cpp
fermat::stack<int, 0> st;
st.push(1);
int v = st.top();
st.pop();
```

### 优先队列 FermatPriorityQueue

支持 `change` / `remove`（可经 `get_container()` 修改堆内元素）。每行中性能更优者以 **粗体** 标出。

| 操作 | FermatPriorityQueue | std::priority_queue | 加速比 |
|------|---------------------|----------------------|--------|
| push/pop (1000) | 22.6 µs | **10.8 µs** | 约 2x 慢 |
| push/pop (10000) | **602 µs** | 649 µs | 1.1x |
| push/pop (100000) | **7.72 ms** | 8.12 ms | 1.1x |
| 从迭代器构造 (1000) | **1.87 µs** | 2.13 µs | 1.1x |
| change/remove (1000) | 支持 | 不支持 | – |

```cpp
fermat::PriorityQueue<int, 0> pq;
pq.push(10);
size_t idx = 0;
pq.get_container()[idx] = 20;
pq.change(idx);
pq.remove(idx);
```

### 对象池 ObjectPool

单线程分配/释放比 `new`/`delete` 快约 6 倍。每行中性能更优者以 **粗体** 标出。

| 操作 | ObjectPool | new/delete | 加速比 |
|------|------------|------------|--------|
| 单线程 | **2.02 ns** | 11.7 ns | 5.8x |
| 16 线程 | **3.77 ns** | 29.0 ns | 7.7x |

```cpp
MyClass* obj = fermat::ObjectPool<MyClass>::get(/* ctor args */);
fermat::ObjectPool<MyClass>::put(obj);
```

### 资源池 ResourcePool

带版本号 ID 的分片对象池，支持跨线程 `find` / `put`（`mem_resource_bench`）。

| 操作 | 时间 |
|------|------|
| 单线程 alloc/free | **14.9 ns** (~67 M ops/s) |
| 16 线程 get/put | **394 ns** (~40 M ops/s) |

```cpp
using MyPool = fermat::ResourcePool<MyClass, 8, 64, 1024, 64>;
uint64_t rid;
MyClass* obj = MyPool::get(rid, /* ctor args */);
MyClass* found = MyPool::find(rid);
MyPool::put(rid);
```

### LRU 缓存 LruCache

可配置底层 map 与 list 类型。benchmark 对比 `turbo::flat_hash_map` + fermat 侵
入式 list，与 `std::map` + `std::list`（`mem_lru_cache`，容量 100k,
缓存容量 = 测试键数（低命中测试容量为一半）。每个组合执行 6 种操作，
记录处理 1000/10000/100000 个键时的平均耗时（ns 或 µs）:

| 操作 | 键数 | std_map_std_list | std_map_fermat_list | turbo_map_std_list | turbo_map_fermat_list |
|------|------|-------------------|----------------------|---------------------|------------------------|
| **插入不重复** | 1000 | 81.0 µs | 59.2 µs | 43.0 µs | **31.5 µs** |
| | 10000 | 869 µs | 729 µs | 514 µs | **406 µs** |
| | 100000 | 11.57 ms | 8.28 ms | 6.82 ms | **5.77 ms** |
| **查找高命中** | 1000 | 45.0 ns | 31.0 ns | 28.3 ns | **21.4 ns** |
| | 10000 | 51.1 ns | 37.2 ns | 31.8 ns | **25.9 ns** |
| | 100000 | 91.7 ns | 45.7 ns | 40.7 ns | **32.9 ns** |
| **查找低命中** | 1000 | 114 ns | 96.8 ns | 85.8 ns | **72.3 ns** |
| | 10000 | 126 ns | 107 ns | 106 ns | **90.1 ns** |
| | 100000 | 149 ns | 118 ns | 127 ns | **114 ns** |
| **更新(assign)** | 1000 | 33.9 ns | 23.6 ns | 22.6 ns | **16.2 ns** |
| | 10000 | 42.4 ns | 29.4 ns | 25.6 ns | **21.0 ns** |
| | 100000 | 66.5 ns | 38.6 ns | 33.3 ns | **27.8 ns** |
| **删除所有** | 1000 | 73.9 µs | 56.2 µs | 42.8 µs | **29.1 µs** |
| | 10000 | 808 µs | 717 µs | 485 µs | **398 µs** |
| | 100000 | 9.88 ms | 8.22 ms | 6.42 ms | **5.56 ms** |
| **touch** | 1000 | 32.9 ns | 22.6 ns | 21.9 ns | **16.2 ns** |
| | 10000 | 39.5 ns | 29.6 ns | 25.2 ns | **21.3 ns** |
| | 100000 | 55.6 ns | 38.7 ns | 31.4 ns | **27.9 ns** |

> **结论**：`turbo::flat_hash_map` + `fermat::List` 组合在所有操作中均表现最佳，推荐作为高性能 LRU 缓存的首选实现。

完整对照见 [`benchmark/README.md`](benchmark/README.md) 中 cache 一节。

```cpp
fermat::LruCache<int, std::string> cache(100000);
cache.insert_or_assign(1, "one");
if (auto v = cache.at(1)) { /* use *v */ }
cache.touch(1);
cache.erase(1);
```

### 基数排序 RadixSort

对 `uint32_t` 随机数据排序比 `std::sort` 快约 10 倍。

| 数据量 | RadixSort | std::sort | 加速比 |
|--------|-----------|-----------|--------|
| 100 万随机 u32 | **5.34 ms** | 53.7 ms | 10.1x |

```cpp
std::vector<uint32_t> data = {5, 2, 8, 1};
fermat::radix_sort(data.begin(), data.end());
```

### 位图 Bitset

`FindNext` 在 1024 位规模下快于 `std::bitset`（`mem_bitset_bench`）：

| 操作 (1024 bit) | fermat | std::bitset | 加速比 |
|-----------------|--------|-------------|--------|
| find_next | **0.387 ns** | 1.56 ns | 4.0x |

### 同步磁盘 IO

`fermat/io` 提供跨平台同步 IO 栈，基于 `turbo::Result` 错误处理：

| 层次 | 说明 |
|------|------|
| `sys_*` | 对 `open/read/write/pread/pwrite/*v/seek/flush` 等的薄封装 |
| `IOReader` / `IOWriter` | 虚接口，供 `CordBuffer` 等容器消费 |
| `SequenceReadFile` / `RandomReadFile` | 读侧文件类型 |
| `SequenceWriteFile` / `RandomWriteFile` | 写侧文件类型 |
| `read_small_file` / `write_small_file` | 一次性读写小文件 |

```cpp
fermat::RandomReadFile f;
TURBO_RETURN_IF_ERROR(f.open(path));
turbo::span<char> buf = ...;
TURBO_ASSIGN_OR_RETURN(size_t n, f.pread(buf, offset));
```

异步 disk IO 尚未纳入核心库。

### 零拷贝辅助工具

#### Receiver 与容器适配器

`Receiver` 是向容器追加字节的抽象接口。`ContainerAppender` 包装已有容器引用；`ContainerReceiver` 拥有容器实例，可通过 `release()` 取出。

```cpp
std::string target;
fermat::ContainerAppender appender(target);
appender.reserve(1024);
appender.append("hello", 5);
```

#### Peeker：零拷贝跨块迭代

`Peeker` 顺序读取 `CordBufferBase` 或连续字符串，返回 `string_view`，跨块不拷贝。支持 `readn()`、`find_first_offset`、`seek_to` 等。

```cpp
fermat::CordBufferBase<64, 4096> buf;
buf.append("Hello world");
fermat::Peeker peeker(&buf);
while (auto chunk = peeker.readn(1024)) {
    process(*chunk);
}
```

#### Customer / Reader：从 CordBuffer 消费数据

- **Customer**（`FlattenCustomer<true>`）：读取并移除已消费字节。
- **Reader**（`FlattenCustomer<false>`）：只拷贝，不修改源。

```cpp
fermat::CordBufferBase<64, 4096> source;
// ... 填充数据 ...
std::string target;
fermat::ContainerAppender appender(target);
fermat::Customer::custom(source, appender, 4096);
fermat::Reader::custom_until(source, appender, '\n');
```

## 编译与依赖

本项目使用 [kmpkg](https://github.com/kumose/kmcmake) 进行依赖管理与构建集成。

**核心库**（`fermat`）链接：`turbo`、`mimalloc`、`Threads`。测试与 benchmark 的额外依赖见 [`kmpkg.json`](kmpkg.json)。

### 0. 准备环境

- Linux（Ubuntu 20.04+ / CentOS 7+ 推荐）
- CMake >= 3.25
- GCC >= 9.4 / Clang >= 12
- 已安装 `kmpkg`（参见 [安装文档](https://kumo-pub.github.io/docs/category/%E6%8C%81%E7%BB%AD%E9%9B%86%E6%88%90----kmpkg)）

### 1. 配置项目（可选）

- 完整依赖列表见 [`kmpkg.json`](kmpkg.json)
- 更新依赖基线：修改 [`kmpkg-configuration.json`](kmpkg-configuration.json) 中 `default-registry` 的 `baseline`
- 也可自行管理依赖，确保 `find_package` 能找到所需库

### 2. 编译项目

```bash
cmake --preset=default
cmake --build build -j$(nproc)
```

自管理依赖：

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 3. 运行测试（可选）

```bash
ctest --test-dir build
```

### 4. 运行 Benchmark（可选）

需开启 benchmark 构建（`KMCMAKE_BUILD_BENCHMARK`）。可执行文件位于 `build/benchmark/`，例如：

```bash
./build/benchmark/mem_string_bench --benchmark_format=console
./build/benchmark/mem_iobuf_benchmark --benchmark_filter=52428800
```

完整原始数据与历史结果见 [`benchmark/README.md`](benchmark/README.md)。

## 许可证

Apache License 2.0。详见 LICENSE 文件。
