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
| `KString` | 构造、拷贝、移动、追加 | <span style="color: green;">1.2x ~ 3x (>20%)</span> |
| `Buffer<T>` | 构造（所有规模）、小规模 push/迭代/中间插删、清除（Release） | <span style="color: green;">2x ~ 5x</span> |
| `Vector<T>` | 与 `Buffer` 同路径优化；Release 下随机访问与 std 持平或更快 | <span style="color: green;">2x ~ 5x</span> |
| `Deque<T, Alloc, kSubarray>` | 构造、两端 push、迭代、随机访问、中间插删、clear、析构 | <span style="color: green;">2x ~ 10x</span> |
| `List<T>` | 所有操作 | <span style="color: green;">2x ~ 20x</span> |
| `PriorityQueue<T>` | push（50–100k）、push/pop（≤2k）、迭代器构造、有界流 | <span style="color: green;">1.2x ~ 4x</span> |
| `Stack<T>` | 所有操作 | <span style="color: green;">20% ~ 10x（构造）</span> |
| `Bitset` | 构造、set/reset/flip、count、find 系列 | <span style="color: green;">1.5x ~ 5x</span> |
| `LruCache`（turbo_map + fermat_list） | 插入、查询、更新、删除、touch | <span style="color: green;">20% ~ 50%</span> |
| `ObjectPool` / `ResourcePool` | 单/多线程分配释放 | <span style="color: green;">5x ~ 10x</span> |
| `RadixSort` | 随机整数排序 | <span style="color: green;">5x ~ 10x</span> |
| `CordBufferBase` | 随机分块追加（10 KiB ~ 100 MiB） | <span style="color: green;">1.5x ~ 5x</span> |
| `CordBinaryStream` | 二进制序列化写/读/往返 | <span style="color: green;">20% ~ 100%</span> |
| `CordStringStream` | 文本 ping‑pong、往返 | <span style="color: green;">20% ~ 50%</span> |

### ⚠️ 显著劣势（建议保留 std）

| 组件 | 劣势操作 | 比 std 慢 | 原因 |
|------|----------|-----------|------|
| `KString` | 短串查找 (`find`) | 约 30% | 与 `std::string` 实现路径不同 |
| `CordFormatter` | 文本格式化 | 约 6x | 当前逐字符输出，未批量写入 |
| `Bitset` | 按位运算（&, \|, ^）、移位 | 约 10% ~ 20% | `std::bitset` 编译器优化更强 |

> **注意**：
> - `FermatList` 所有操作均快于 `std::list`，无劣势。
> - `FermatStack` 所有操作均快于 `std::stack`，无劣势。
> - `KString` 在构造、拷贝、移动、追加等操作上明显更快；短串 `find` 略慢于 `std::string`。
> - `Buffer<T>`（plain 数据，如 `int`/`float`）与 `Vector<T>`（通用连续容器）在 **Release** 模式下经近期优化后，相对 `std::vector` 已无明显劣势；小尺寸随机访问与 std 大体持平，可放心用于性能敏感路径。
> - `Deque<T>`（`mem_deque_bench`，默认 `kDequeSubarraySize = 256`）在实测各项上均快于或持平 `std::deque`；可通过第三个模板参数（须为 2 的幂）调节块大小，在内存占用与遍历/插删性能之间权衡。
> - `ObjectPool` 跨线程转移（`collect_tsl` / `apply_tls`、`collect_arena` / `apply_arena`）止于用 **`ObjectGuard` 搬动空闲块**——协程/M:N 下的刻意边界；**仅适合资深、严格掌控内存的用法**，否则请用同线程池或 `ResourcePool`。
> - **`CordInputStringStream` 对比连续内存上的 `std::istringstream`**（benchmark 中 TokenRead / BulkRead，见 [`benchmark/README.md`](benchmark/README.md)）是**平坦内存的性能参照上限**，不是公平的「fermat 劣势」；数据已连续用 std，数据在 `CordBufferBase` 分块里用 Cord 流。

## 场景选择指南

- **只有当 fermat 组件在您的场景中优势明显（>20%）且劣势可接受（<10%）时，才推荐替换 `std`。** 否则优先使用标准库。
- 如果核心瓶颈不在此处，无需刻意替换。

| 场景 | 推荐组件 | 原因与注意事项 |
|------|-----------|----------------|
| 大量字符串构造/拷贝/移动 | `KString` | 比 `std::string` 快 <span style="color: green;">1.3~3x</span>；短串 `find` 略慢 |
| 流式追加大数据（日志、网络收包） | `CordBufferBase` | 随机块追加吞吐量极高（10 KiB~100 MiB 区间最佳） |
| 从磁盘/网络填充分块数据 | `CordBufferBase` + `append_by_*` | 与 `IOReader` 无缝集成，零拷贝 |
| 中小型连续数组（性能敏感） | `Buffer<T>` / `Vector<T>` | plain 数据用 `Buffer`；需完整 vector 语义用 `Vector`。Release 下构造、push、迭代、中间插删等多项 ≥20% 更快；大批量 push 与 std 大体持平 |
| 高频双端 push（deque） | `Deque<T, Alloc, kSubarray>` | 第三模板参数 `kSubarray` 为每块元素个数（2 的幂，默认 **256**）；块越大越利于扫描与中间插删，块越小越省极小 deque 的尾部浪费 |
| 有序只读 / 批量构建 map/set | `VectorMap` / `VectorSet` | 有序插入快 <span style="color: green;">3.8x</span>，迭代快 <span style="color: green;">24x</span>；随机插入慢，不适合频繁修改 |
| 高频栈操作 | `FermatStack` | 从容器构造快约 <span style="color: green;">11x</span>，push/pop 快约 <span style="color: green;">1.2x</span>，无劣势 |
| 支持优先级修改的优先队列 | `FermatPriorityQueue` | 支持 `change`/`remove`；≤2k push 约 <span style="color: green;">2×</span>，1k push/pop 约 <span style="color: green;">4×</span>；实测平台 std pop 在 ~600 附近有断崖 |
| 小对象池化（同线程） | `ObjectPool` | 线程本地缓存，比 `new`/`delete` 快 <span style="color: green;">5~10 倍</span> |
| 生产者 / 消费者线程复用池化内存 | `ObjectPool` / `BasicAllocator` + `collect_tsl` / `apply_tls`（或 `collect_arena` / `apply_arena`） | **进阶用法**：须严格掌控内存/线程/生命周期；见 ObjectPool 一节 |
| 高并发资源复用（带版本号） | `ResourcePool` | 跨线程句柄查找；16 线程约 394 ns/op |
| 本地 LRU 缓存 | `LruCache` + `turbo::flat_hash_map` + `fermat::List` | 所有操作比 `std::map`+`std::list` 快 <span style="color: green;">20%~50%</span> |
| 大量整数排序（随机数据） | `RadixSort` | 比 `std::sort` 快 <span style="color: green;">5~10x</span>；已排序或逆序数据 `std::sort` 更快 |
| 位图查找下一个置位位 | `fermat::Bitset` | `FindNext` 比 `std::bitset` 快 <span style="color: green;">4x</span>；按位运算略慢，避免高频使用 |
| 已有内存上的位标志（零额外分配） | `BitmapView<true, uint64_t>` | 非拥有视图，经 `setup()` 绑定；后备存储须 **8 字节对齐** 的 `uint64_t` 数组，见下文 Bitset 一节 |
| 同步文件读写 | `sys_*` / `*ReadFile` / `*WriteFile` | 跨平台抽象，基于 `turbo::Result` 错误处理 |
| 零拷贝跨块遍历 CordBuffer | `Peeker` | 返回 `string_view`，不拷贝 |
| 从 CordBuffer 消费数据到容器 | `Customer` / `Reader` | 支持拷贝或移动消费 |
| 动态随机 map/set | `std::map` 或 `turbo::flat_hash_map` | fermat 未提供高性能随机 map |
| 数据已在连续内存，无需 flatten | `std::vector` / `std::string` + `std::istringstream` | 参照上限，非 fermat 组件劣势 |
| 分块 Cord 数据上解析/token 化（网络、IOBuf、共享引用） | `CordInputStringStream` + `Peeker` | 避免先摊平成单块 buffer |
| 文本格式化（高吞吐） | `CordOutputStringStream` 或 `fmt::format_to` + 自定义 sink | 避免使用 `CordFormatter` |

## 容器 `capacity` 语义

`KString`、`Buffer<T>`、`Vector<T>` 等连续容器的约定如下：

- **逻辑数据边界**用 **`size()`**（或实际写入长度）。
- **当前可用存储上界**可以在当下用 **`capacity()`** 做约束（例如判断还能写入多少、在未扩容前做边界检查），但**不要缓存 `capacity()`**——每次需要时重新读取。

`reserve(n)` 之后，fermat 会尽量让已申请内存都可用，故通常有 **`capacity() >= n`**，小规模下还会按分级内存池（`good_size` / 池化档位）向上取整，**`capacity()` 可能大于 `n`**。这与 `std::vector` / `std::string` 的常见体验类似：实现上也会为减少分配次数而多留一些空间；差别在于 **标准库为语义一致性做了更强约束**（许多场景下 `reserve` 后 `capacity` 更接近请求值），而 **fermat 优先池化与减少分配次数**，不保证 `capacity` 与 `reserve` 参数一一对应。

因此：可以把 `capacity()` 当作**瞬时**的可用空间上界；不要把某次读到的 `capacity` 存进成员变量、协议字段或持久化状态——在 `reserve`、`shrink`、`clear` 或再次扩容后它会变，缓存后容易误判剩余空间。

## 性能数据详情

下文容器类 benchmark 均为 **Release** 编译结果（与 [`benchmark/README.md`](benchmark/README.md) 中 Release 表一致）；同文件中的 DEBUG 表仅作开发期对照，**选型请以 Release 为准**。

### 字符串 KString

`fermat::BasicString`（别名 `KString`）在构造、拷贝、移动、追加等操作上优于 `std::string`；短串 `find` 略慢。数据来自 [`benchmark/README.md`](benchmark/README.md)。每行中性能更优者以 **粗体** 标出（此处以绿色高亮数值）。

| 操作 | KString | std::string | 加速比 |
|------|---------|-------------|--------|
| 构造 (Long) | <span style="color: green;">14.4 ns</span> | 41.3 ns | <span style="color: green;">2.9x</span> |
| 拷贝 (Long) | <span style="color: green;">25.8 ns</span> | 31.6 ns | <span style="color: green;">1.2x</span> |
| 移动 (Long) | <span style="color: green;">14.9 ns</span> | 43.7 ns | <span style="color: green;">2.9x</span> |
| 追加 (Medium) | <span style="color: green;">3.75 ns</span> | 4.38 ns | <span style="color: green;">1.2x</span> |
| 查找 (Short) | 10.8 ns | <span style="color: green;">8.13 ns</span> | 略慢 |
| 查找 (Long) | <span style="color: green;">92.6 ns</span> | 107 ns | <span style="color: green;">1.2x</span> |
| 哈希 (Long) | <span style="color: green;">229 ns</span> | 232 ns | 持平 |

策略：保守扩容，累积大数据请预先 `reserve()`。

```cpp
fermat::KString s = "hello";
s.reserve(10 * 1024 * 1024);
for (int i = 0; i < 1000000; ++i) s.append("a");
```

### 连续容器 Buffer\<T\>

`fermat::Buffer<T>` 是专为 plain 数据（如 `int`、`float`）优化的连续容器，接口兼容 `std::vector`，构造、清除更快。以下为 Release 模式性能数据（Intel 12‑core @ 4.4 GHz，L3 18 MiB）。每行中性能更优者以 **粗体** 标出。

| 操作 | 规模 | std::vector\<int\> (ns)                   | fermat::Buffer\<int\> (ns)                 | 更优 |
|------|------|-------------------------------------------|--------------------------------------------|------|
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

### 双端队列 Deque\<T\>

`fermat::Deque` 接口兼容 `std::deque`。第三个模板参数 **`kDequeSubarraySize`**（须为 **2 的幂**）表示每个内存块可容纳的元素个数，默认 **256**。块更小适合元素很少的 deque、减少尾部浪费；块更大则减少指针表与跨块次数，利于大规模遍历与中间插删。

```cpp
fermat::Deque<int> dq;                                    // 默认 kDequeSubarraySize = 256
fermat::Deque<int, BasicAllocator<int>, 128> dq128;       // 每块 128 个元素
```

下表为 `fermat::Deque<int>` 与 `std::deque<int>` 对照（`mem_deque_bench`，Release）。每行中性能更优者以 **粗体** 标出。

| 基准项 | 规模 | std::deque&lt;int&gt; | fermat::Deque&lt;int&gt; |
|--------|------|----------------------|--------------------------|
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

### 连续容器 Vector\<T\>

`fermat::Vector<T>` 接口兼容 `std::vector`，支持非 trivial 类型；连续存储与分配路径与 `Buffer` 共用优化。以下为 **Release** 模式数据（同机环境见上）。完整对照见 [`benchmark/README.md`](benchmark/README.md) Vector 一节。

| 操作 | 规模 | std::vector\<int\> (ns) | fermat::Vector\<int\> (ns) | 更优 |
|------|------|-------------------------|----------------------------|------|
| **Construct** | 4 | 14.6 | <span style="color: green;">5.31</span> | fermat |
| | 1024 | 43.5 | <span style="color: green;">23.9</span> | fermat |
| **PushBackSmall** | 4 | 14.5 | <span style="color: green;">6.93</span> | fermat |
| | 1024 | 488 | <span style="color: green;">481</span> | fermat |
| **IterationSmall** | 4 | 15.4 | <span style="color: green;">5.70</span> | fermat |
| | 1024 | 301 | <span style="color: green;">273</span> | fermat |
| **RandomAccessSmall** | 4 | 174 | <span style="color: green;">174</span> | 持平 |
| | 1024 | <span style="color: green;">172</span> | 180 | std |
| **EmplaceBackSmall** | 4 | 15.0 | <span style="color: green;">6.66</span> | fermat |
| | 1024 | 489 | <span style="color: green;">489</span> | 持平 |
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

### 有序关联容器 VectorMap / VectorSet

基于有序 `vector`，有序插入和批量构造极快，迭代速度远高于 `std::map`。每行中性能更优者以 **粗体** 标出。

| 操作 (1000) | VectorMap | std::map | 加速比 |
|-------------|-----------|----------|--------|
| 有序插入 | <span style="color: green;">9.2 µs</span> | 35.3 µs | <span style="color: green;">3.8x</span> |
| 迭代 | <span style="color: green;">0.15 ns/元素</span> | 3.6 ns/元素 | <span style="color: green;">~24x</span> |
| 随机插入 | 83.9 µs | <span style="color: green;">26.3 µs</span> | 更慢 |

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
| top() | <span style="color: green;">0.374 ns</span> | 0.423 ns | <span style="color: green;">1.1x</span> |
| 从容器构造 (1000) | <span style="color: green;">20.2 ns</span> | 220 ns | <span style="color: green;">10.9x</span> |
| push/pop (1000) | <span style="color: green;">759 ns</span> | 919 ns | <span style="color: green;">1.2x</span> |

```cpp
fermat::stack<int, 0> st;
st.push(1);
int v = st.top();
st.pop();
```

### 优先队列 FermatPriorityQueue

支持 `change` / `remove`（可经 `get_container()` 修改堆内元素）。Push 走 `fermat::push_heap`；Pop 目前与 std 一样走 `std::pop_heap`。每行中性能更优者以 **粗体** 标出。

| 操作 | 规模 | FermatPriorityQueue | std::priority_queue | 说明 |
|------|------|---------------------|----------------------|------|
| **Push** | 1000 | <span style="color: green;">1853 ns</span> | 2369 ns | <span style="color: green;">~1.3x</span> |
| | 100000 | <span style="color: green;">980 µs</span> | 1253 µs | <span style="color: green;">~1.3x</span> |
| **Pop** | 1000 | <span style="color: green;">12.6 µs</span> | 38.7 µs | std 在 ~600 附近有断崖 |
| | 100000 | <span style="color: green;">7.10 ms</span> | 7.25 ms | 基本持平 |
| **PushPop** | 1000 | <span style="color: green;">9.90 µs</span> | 41.4 µs | <span style="color: green;">~4.2x</span> |
| | 100000 | <span style="color: green;">7.82 ms</span> | 8.19 ms | <span style="color: green;">~1.05x</span> |
| **ConstructFromIterators** | 10000 | <span style="color: green;">20.2 µs</span> | 46.9 µs | <span style="color: green;">~2.3x</span> |
| **BoundedPushPop**（上限 1000） | – | <span style="color: green;">13.9 µs</span> | 33.8 µs | 2000 次流式操作 |
| change/remove (1000) | – | 1.68 µs | 不支持 | – |

全尺寸对照见 [`benchmark/README.md`](benchmark/README.md) Priority queue 一节。

```cpp
fermat::PriorityQueue<int, 0> pq;
pq.push(10);
size_t idx = 0;
pq.get_container()[idx] = 20;
pq.change(idx);
pq.remove(idx);
```

### 对象池 ObjectPool

`fermat::ObjectPool`（[`fermat/memory/object_pool.h`](fermat/memory/object_pool.h)）为每种类型维护**线程本地空闲链表**：同一线程上高频 `get` / `put` 多数不碰全局分配器。benchmark 中单线程分配/释放比 `new`/`delete` 快约 6 倍。每行中性能更优者以 **粗体** 标出。

| 操作 | ObjectPool | new/delete | 加速比 |
|------|------------|------------|--------|
| 单线程 | <span style="color: green;">2.02 ns</span> | 11.7 ns | <span style="color: green;">5.8x</span> |
| 16 线程 | <span style="color: green;">3.77 ns</span> | 29.0 ns | <span style="color: green;">7.7x</span> |

**同线程（常规）：**

```cpp
MyClass* obj = fermat::ObjectPool<MyClass>::get(/* ctor args */);
fermat::ObjectPool<MyClass>::put(obj);
```

**生产者 / 消费者线程 — 在线程间转移池化内存**

**刻意的 API 边界（不是「库只能做到这里」）**

在**协程**与 M:N 调度下，一个 OS 线程上往往跑多个逻辑任务，工作也可能在**另一线程或 executor** 上 resume。若库再封装「标准生产者–消费者池」，就要接管**批次、背压、队列深度以及何时转移内存**——会与你们的运行时、锁和流水线语义重叠。

综合考虑协程场景后，fermat 只提供：

- **`ObjectPool<T>::collect_tsl()` / `apply_tls()`** — 经 **`ObjectGuard`** 搬动本线程 TLS 空闲链表
- **`BasicAllocator::collect_arena()` / `apply_arena()`** — `Buffer` / `Vector` / `KString` 背后**分级池**的同样机制

在**业务已定义的边界**（一批结束、协程切换、线程交接）调用；搬的是**空闲块**，对象生命周期仍由业务约束。

再往上封装会内置**竞争形态与调度策略**，**与业务抢语义**；当前原语层是**有意为之的设计上限**，而非能力不足的半成品。

> **适用对象**  
> **跨线程 `collect` / `apply` 属于进阶路径**，只适合**资深、能严格掌控内存**的用法：须自行保证对象生命周期、哪条线程可 `get` / `put_raw`、存活指针在途与空闲块转移的时序、以及 `ObjectGuard` 交接顺序。若团队达不到这一约束，请用**同线程 `ObjectPool`** 或跨线程 **`ResourcePool` 句柄**，不要把 TLS 转移当作默认捷径。

池缓存是**线程本地**的：不要在消费者线程上对生产者线程 `get` 出的指针做 `put`。典型模式是**生产者分配、消费者回收**：生产者在本线程 TLS `get`；消费者在本线程 `put_raw`；一批处理完后由**消费者 `collect_tsl()`**、**生产者 `apply_tls()`** 把空闲块归还到生产者 TLS：

| API | 作用 |
|-----|------|
| `ObjectPool<T>::collect_tsl()` | **源线程**：将本线程 TLS 空闲链表移入 `ObjectGuard`（RAII；未移交则在析构时释放）。 |
| `ObjectPool<T>::apply_tls(guard)` | **目标线程**：把 `guard` 合并进本线程 TLS，后续分配走本地缓存。 |
| `BasicAllocator<T, Align>::collect_arena()` / `apply_arena()` | 对 `Buffer` / `Vector` / `KString` 使用的**分级池**（`TieredAllocator`）做同样操作；按尺寸档位返回多个 `ObjectGuard`。 |

`ObjectGuard` 的移交由业务同步（互斥锁、队列、代际屏障等）。消费者一批 **`put_raw` 完成后**，在消费者侧 **`collect_tsl()`**、在生产者侧 **`apply_tls()`** 把空闲块还回生产者 TLS，形成**乒乓复用**；临界区只在 guard 交接，而非每个对象一把锁。

```cpp
struct RequestCtx { /* ... */ };

std::mutex mu;
std::optional<fermat::ObjectGuard<0>> freed;  // 消费者 → 生产者

// 生产者（如接入 / 解析）：从本地 TLS 分配；存活对象经业务队列交给消费者
void producer_issue(WorkQueue& q) {
  RequestCtx* ctx = fermat::ObjectPool<RequestCtx>::get_uninitialize();
  // 构造 / 填充 ctx ...
  q.push(ctx);  // 业务队列，不是 ObjectGuard
}

// 生产者：消费者 collect_tsl 之后，把归还的空闲块合并进本地 TLS
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

// 消费者（如 worker）：处理、本地 put_raw，再 collect 交回生产者
void consumer_drain(WorkQueue& q) {
  while (RequestCtx* ctx = q.pop()) {
    // ... 处理 ...
    fermat::ObjectPool<RequestCtx>::put_raw(ctx);
  }
  auto guard = fermat::ObjectPool<RequestCtx>::collect_tsl();
  std::lock_guard lock(mu);
  freed = std::move(guard);
}
```

**分级分配器**（消费者在大量 `Buffer`/`Vector` 释放进池后，把缓存还回生产者）：

```cpp
using Alloc = fermat::BasicAllocator<uint8_t, 64>;
// 消费者线程、释放进池之后：
auto guards = Alloc::collect_arena();
// 将 guards 移到生产者线程后，在生产者上：
Alloc::apply_arena(guards);
```

若要在多线程间传递**仍存活的对象句柄**（而非仅转移空闲块），请用 **`ResourcePool`**（带版本号 ID 的 `find` / `put`），不要跨线程传递裸池指针。

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

可配置底层 map 与 list 类型。benchmark 对比 `turbo::flat_hash_map` + fermat 侵入式 list，与 `std::map` + `std::list`（`mem_lru_cache`，容量 100k；缓存容量 = 测试键数，低命中测试为一半容量）。每个组合执行 6 种操作，记录处理 1000/10000/100000 个键时的平均耗时（ns 或 µs）：

| 操作 | 键数 | std_map_std_list | std_map_fermat_list | turbo_map_std_list | turbo_map_fermat_list |
|------|------|-------------------|----------------------|---------------------|------------------------|
| **插入不重复** | 1000 | 81.0 µs | 59.2 µs | 43.0 µs | <span style="color: green;">31.5 µs</span> |
| | 10000 | 869 µs | 729 µs | 514 µs | <span style="color: green;">406 µs</span> |
| | 100000 | 11.57 ms | 8.28 ms | 6.82 ms | <span style="color: green;">5.77 ms</span> |
| **查找高命中** | 1000 | 45.0 ns | 31.0 ns | 28.3 ns | <span style="color: green;">21.4 ns</span> |
| | 10000 | 51.1 ns | 37.2 ns | 31.8 ns | <span style="color: green;">25.9 ns</span> |
| | 100000 | 91.7 ns | 45.7 ns | 40.7 ns | <span style="color: green;">32.9 ns</span> |
| **查找低命中** | 1000 | 114 ns | 96.8 ns | 85.8 ns | <span style="color: green;">72.3 ns</span> |
| | 10000 | 126 ns | 107 ns | 106 ns | <span style="color: green;">90.1 ns</span> |
| | 100000 | 149 ns | 118 ns | 127 ns | <span style="color: green;">114 ns</span> |
| **更新(assign)** | 1000 | 33.9 ns | 23.6 ns | 22.6 ns | <span style="color: green;">16.2 ns</span> |
| | 10000 | 42.4 ns | 29.4 ns | 25.6 ns | <span style="color: green;">21.0 ns</span> |
| | 100000 | 66.5 ns | 38.6 ns | 33.3 ns | <span style="color: green;">27.8 ns</span> |
| **删除所有** | 1000 | 73.9 µs | 56.2 µs | 42.8 µs | <span style="color: green;">29.1 µs</span> |
| | 10000 | 808 µs | 717 µs | 485 µs | <span style="color: green;">398 µs</span> |
| | 100000 | 9.88 ms | 8.22 ms | 6.42 ms | <span style="color: green;">5.56 ms</span> |
| **touch** | 1000 | 32.9 ns | 22.6 ns | 21.9 ns | <span style="color: green;">16.2 ns</span> |
| | 10000 | 39.5 ns | 29.6 ns | 25.2 ns | <span style="color: green;">21.3 ns</span> |
| | 100000 | 55.6 ns | 38.7 ns | 31.4 ns | <span style="color: green;">27.9 ns</span> |

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
| 100 万随机 u32 | <span style="color: green;">5.34 ms</span> | 53.7 ms | <span style="color: green;">10.1x</span> |

```cpp
std::vector<uint32_t> data = {5, 2, 8, 1};
fermat::radix_sort(data.begin(), data.end());
```

### 位图 Bitset 与 `BitmapView`

**`fermat::Bitset`** — 编译期固定位数的**拥有型**位图，语义接近 `std::bitset`。

**`fermat::BitmapView`** — **不拥有内存的视图**，绑定业务侧已有缓冲区（结构体字段、slab、映射区等），自身不分配/释放存储；通过 `setup(span<uint64_t>, bit_count)` 或接受 `turbo::span<WordType>` 的构造函数挂接。便于在已有位图内存上直接做 find / 按位运算，无需再拷一份。

> **硬性要求：`WordType = uint64_t`，后备区 8 字节对齐**
>
> - 模板参数 **`WordType` 须为 `uint64_t`**（每字 8 字节）方为**受支持**用法；benchmark 与快速路径（`find_*`、按字运算）按 64 位字实现。
> - 后备存储为 **`uint64_t` 数组**，须 **8 字节对齐**（`alignas(8)` 等），长度至少 `ceil(位数 / 64)` 个字。
> - **`WordType` 小于 8 字节**（如 `uint32_t`、`uint16_t`、`uint8_t`）**无法保证**效率与安全性，请勿在生产路径使用。

```cpp
constexpr size_t kBits = 1024;
alignas(8) uint64_t storage[(kBits + 63) / 64]{};

fermat::BitmapView<true, uint64_t> bm;
bm.setup(turbo::span<uint64_t>(storage), kBits);
size_t i = bm.find_first();
```

**`fermat::Bitset`** 在 1024 位上的 `find_next` 快于 `std::bitset`（`mem_bitset_bench`）；对齐的 `uint64_t` 后备区上的 **`BitmapView`** 在 find 类操作上表现接近。三方完整表见 [`benchmark/README.md`](benchmark/README.md) Bitset 一节。

| 操作 (1024 bit) | fermat::Bitset | std::bitset | 加速比 |
|-----------------|--------|-------------|--------|
| find_next | <span style="color: green;">0.387 ns</span> | 1.56 ns | <span style="color: green;">4.0x</span> |

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