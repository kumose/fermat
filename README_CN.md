# fermat – 高性能 C++ 基础库

fermat 是一套专注于高吞吐、低延迟的 C++ 基础组件库，提供优化的字符串、分块缓冲区、连续容器、有序关联容器、栈、优先队列、内存池、缓存、基数排序以及零拷贝 IO 辅助工具。相比标准库，fermat 在多种场景下取得 2~10 倍性能提升。

## 模块与性能数据

以下数据基于 Intel Xeon 平台，单次操作为平均值。

### 字符串 KString

fermat::BasicString（别名 KString）常规操作优于 std::string：

| 操作 | KString | std::string | 加速比 |
|------|---------|-------------|--------|
| 构造 (Long) | 15.3 ns | 44.1 ns | 2.9x |
| 拷贝 (Long) | 27.0 ns | 34.8 ns | 1.3x |
| 移动 (Long) | 19.1 ns | 49.4 ns | 2.6x |
| 追加 (Medium) | 3.79 ns | 4.48 ns | 1.2x |
| 查找 (Long) | 93.3 ns | 106 ns | 1.1x |
| 哈希 (Long) | 227 ns | 231 ns | 持平 |

策略：保守扩容，累积大数据请预先 reserve()。

```cpp
fermat::KString s = "hello";
s.reserve(10 * 1024 * 1024);
for (int i = 0; i < 1000000; ++i) s.append("a");
```

### 分块缓冲区 CordBufferBase / IOBuf

随机分块追加（16KB 块）50MB 吞吐量：

| 容器 | 吞吐量 |
|------|--------|
| CordBufferBase | 13.87 GiB/s |
| IOBuf | 9.56 GiB/s |
| std::string | 2.08 GiB/s |

CordBufferBase 写入即见，适合流式追加、日志、网络收包。IOBuf 支持 borrow/commit 原子提交，适合 readv 等事务写入。

```cpp
fermat::CordBufferBase<64, 16*1024> cb;
cb.append("data", 4);

fermat::IOBuf<64, 16*1024> ib;
auto lease = ib.borrow(4096).value();
lease->write("hello", 5);
ib.commit(lease);
```

### 连续容器 Buffer\<T\>

接口兼容 std::vector，构造、清除更快：

| 操作 | fermat::Buffer<int> | std::vector<int> | 加速比 |
|------|---------------------|------------------|--------|
| 构造 1024 元素 | 22.1 ns | 55.5 ns | 2.5x |
| push_back 1000 次 | 467 ns | 486 ns | 持平 |
| 迭代 1000 元素 | 245 ns | 244 ns | 持平 |
| 随机访问 10000 | 1718 ns | 1702 ns | 持平 |

```cpp
fermat::Buffer<int> v;
v.reserve(1000);
v.push_back(42);
```

### 有序关联容器 VectorMap / VectorSet

基于有序 vector，有序插入和批量构造极快，迭代速度是 std::map 的 20 倍以上。

| 操作 (1000) | VectorMap | std::map | 加速比 |
|-------------|-----------|----------|--------|
| 有序插入 | 9.2 µs | 35.3 µs | 3.8x |
| 迭代 | 0.15 ns/元素 | 3.6 ns/元素 | 24x |
| 随机插入 | 83.9 µs | 26.3 µs | 慢 |

```cpp
fermat::VectorMap<int, std::string> m;
m.insert({1, "one"});
auto it = m.find(1);
```

### 栈 FermatStack

top() 极快，从容器构造快 10 倍。

| 操作 | FermatStack | std::stack | 加速比 |
|------|-------------|------------|--------|
| top() | 0.287 ns | 0.404 ns | 1.4x |
| 从容器构造 (1000) | 20.7 ns | 223 ns | 10.8x |
| push/pop (1000) | 828 ns | 926 ns | 1.1x |

```cpp
fermat::Stack<int> st;
st.push(1);
int v = st.top();
st.pop();
```

### 优先队列 FermatPriorityQueue

支持 increase_key / decrease_key / erase。

| 操作 | FermatPriorityQueue | std::priority_queue | 加速比 |
|------|---------------------|----------------------|--------|
| push/pop (1000) | 19 µs | 37.4 µs | 1.97x |
| 从迭代器构造 (1000) | 1.67 µs | 2.09 µs | 1.25x |
| change/remove (1000) | 1.83 µs | 不支持 | – |

```cpp
fermat::PriorityQueue<int> pq;
pq.push(10);
pq.increase_key(10, 20);
pq.erase(10);
```

### 对象池 ObjectPool

单线程分配/释放比 new/delete 快 6 倍。

| 操作 | ObjectPool | new/delete | 加速比 |
|------|------------|------------|--------|
| 单线程 | 2.02 ns | 11.8 ns | 5.8x |
| 16 线程 | 3.91 ns | 30.9 ns | 7.9x |

```cpp
fermat::ObjectPool<MyClass> pool;
auto* obj = pool.allocate();
pool.deallocate(obj);
```

### 资源池 ShardedPool

高并发分片池。

| 操作 | 时间 |
|------|------|
| 单线程 alloc/free | 14.9 ns (66.95 M ops/s) |
| 16 线程 get/put | 438 ns (36.56 M ops/s) |

```cpp
ShardedPool<MyClass, 64> pool;
auto* p = pool.get();
pool.put(p);
```

### 缓存 TurboCache

高吞吐本地 LRU 缓存，比基于 unordered_map 的标准缓存快 30~50%。

| 操作 (100k) | TurboCache | StdCache | 加速比 |
|-------------|------------|----------|--------|
| 插入 distinct | 6.82 ms | 9.68 ms | 1.42x |
| 查找高命中 | 37 ns | 66.3 ns | 1.79x |
| 更新 | 35.1 ns | 67.3 ns | 1.92x |
| 遍历 touch | 31.1 ns | 56.7 ns | 1.82x |

```cpp
fermat::TurboCache<int, std::string> cache(1000);
cache.insert(1, "one");
auto val = cache.find(1);
cache.touch(1);
cache.erase(1);
```

### 基数排序 RadixSort

对 uint32_t 排序比 std::sort 快 5~10 倍。

| 数据量 | RadixSort | std::sort | 加速比 |
|--------|-----------|-----------|--------|
| 100 万随机 | 5.22 ms | 59.1 ms | 11.3x |

```cpp
std::vector<uint32_t> data = {5, 2, 8, 1};
fermat::radix_sort(data.begin(), data.end());
```

### IO 辅助工具

#### Receiver 与容器适配器

Receiver 是抽象接口，用于向容器追加数据并预留容量。提供 ContainerAppender（包装引用）和 ContainerReceiver（拥有容器）。

```cpp
std::string target;
fermat::ContainerAppender appender(target);
appender.reserve(1024);
appender.append("hello", 5);
```

#### Peeker：零拷贝跨块迭代器

Peeker 可以顺序读取 IOBuf 或连续字符串的数据，返回 string_view，不跨块拷贝。支持 readn()、find_first_offset、seek_to 等。

```cpp
fermat::IOBuf<64, 4096> buf;
buf.append("Hello world");
fermat::Peeker peeker(&buf);
while (auto chunk = peeker.readn(1024)) {
    process(*chunk);
}
```

#### Customer / Reader：从 IOBuf 消费数据

Customer (FlattenCustomer<true>)：读取并物理移除已读字节。Reader (FlattenCustomer<false>)：只拷贝，不修改源。

```cpp
fermat::IOBuf<> source;
// ... 填充数据 ...
std::string target;
fermat::ContainerAppender appender(target);
fermat::Customer::custom(source, appender, 4096);   // 消费 4096 字节
fermat::Reader::custom_until(source, appender, '\n'); // 读至换行符
```

## 快速选择指南

| 需求 | 推荐组件 |
|------|-----------|
| 常规字符串（累积大数据需 reserve） | KString |
| 流式追加大数据（写入即见） | CordBufferBase |
| 原子提交的大数据写入（如 readv） | IOBuf |
| 零拷贝跨块遍历 IOBuf | Peeker<IOBuf> |
| 从 IOBuf 消费数据到容器 | Customer / Reader |
| 中小连续数组（性能敏感） | Buffer<T> |
| 有序只读/批量构建映射/集合 | VectorMap / VectorSet |
| 动态随机映射/集合 | std::map / std::set |
| 高频栈操作 | FermatStack |
| 支持修改优先级的优先队列 | FermatPriorityQueue |
| 小对象池化 | ObjectPool |
| 高并发资源复用 | ShardedPool |
| 高吞吐本地缓存 | TurboCache |
| 大量整数排序 | RadixSort |

## 编译与依赖

本项目使用 [kmpkg](https://github.com/kumose/kmcmake) 进行依赖管理与构建集成。kmpkg 会自动处理第三方库下载、依赖查找、编译标志配置等，避免手工维护复杂的 CMake 配置。

### 0. 准备环境

- Linux (Ubuntu 20.04+ / CentOS 7+ 推荐)
- CMake >= 3.25
- GCC >= 9.4 / Clang >= 12
- 已安装 `kmpkg`（参见 [安装文档](https://kumo-pub.github.io/docs/category/%E6%8C%81%E7%BB%AD%E9%9B%86%E6%88%90----kmpkg)）

### 1. 配置项目（可选）

- 完整的依赖请参见 [`kmpkg.json`](kmpkg.json)
- 更新依赖基线请参见 [`kmpkg-configuration.json`](kmpkg-configuration.json) 修改 `default-registry` 的 `baseline`
- `baseline` 可通过 `git log` 获取最新提交
- 可选：用户可以自行管理依赖，确保 CMake 的 find_package 能正确找到所需库

  - 例如在系统路径或自定义路径安装依赖，并通过 CMAKE_PREFIX_PATH 指定
  - 或在 kmpkg 中声明外部依赖路径，避免重复下载

### 2. 编译项目

在项目根目录执行：

```bash
cmake --preset=default
cmake --build build -j$(nproc)
```

自管理依赖：

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

注意：`--preset=default` 需确保已在项目根目录下定义相应 CMake Preset。

### 3. 运行测试（可选）

```bash
ctest --test-dir build
```

## 许可证

Apache License 2.0。详见 LICENSE 文件。
