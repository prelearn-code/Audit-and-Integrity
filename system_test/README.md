# VDS 性能测试框架

完整的性能测试框架，用于测试 VDS (Verifiable Dynamic Searchable) 系统的插入、搜索和验证性能。

## ✨ 三个独立测试（推荐使用）

本测试框架提供**三个完全独立的测试脚本**，每个测试专注于一个功能模块：

| 测试 | 脚本 | 功能 | 清理范围 | 依赖 |
|------|------|------|----------|------|
| 1️⃣ **插入测试** | `run_insert_test.sh` | 测试文件加密和插入性能 | 清理所有数据库和文件 | 无 |
| 2️⃣ **搜索测试** | `run_search_test.sh` | 测试关键词搜索性能 | 只清理搜索token和proof | 需要插入测试的数据库 |
| 3️⃣ **验证测试** | `run_verify_test.sh` | 测试证明验证性能 | 不清理（只读） | 需要搜索测试的证明 |

### 🎯 为什么使用独立测试脚本？

✅ **独立运行** - 每个测试可以单独执行，不会自动串联
✅ **智能检查** - 自动检查依赖，提示缺少的数据
✅ **精确清理** - 每个测试只清理自己产生的文件
✅ **可重复运行** - 不影响其他测试的数据
✅ **简单易用** - 一个命令完成一个测试

---

## 🚀 快速开始

### 方式一：三个独立测试（推荐）

#### 1️⃣ 插入性能测试

测试文件加密和插入到数据库的性能。

```bash
cd system_test
./run_insert_test.sh
```

**特点**:
- 🔥 **完全独立**，不依赖任何其他测试
- 🔥 **完全重置**，清理所有数据库后重新插入
- 🔥 **自动清理**: 删除所有数据库、加密文件、元数据

**清理的文件**:
- ✅ `index_db.json` - 索引数据库
- ✅ `search_db.json` - 搜索数据库
- ✅ `EncFiles/*` - 所有加密文件
- ✅ `MetaFiles/*` - 所有元数据
- ✅ `Insert/*` - 所有插入JSON
- ✅ `keyword_states.json` - 关键词状态

**生成的结果**:
- 📊 `insert_results_YYYYMMDD_HHMMSS/`
  - `insert_detailed.csv` - 每个文件的详细性能数据
  - `insert_summary.json` - 统计摘要

---

#### 2️⃣ 搜索性能测试

测试关键词搜索和证明生成的性能。

```bash
cd system_test
./run_search_test.sh
```

**前提条件**:
- ⚠️ 必须先运行过 `run_insert_test.sh`
- ⚠️ 需要存在数据库文件

**特点**:
- 🔥 **独立运行**，但依赖插入测试的数据库
- 🔥 **可重复运行**，不影响数据库
- 🔥 **智能检查**: 自动检查数据库是否存在

**清理的文件**:
- ✅ `Search/*.json` - 所有搜索token文件
- ✅ `SearchProof/*.json` - 所有搜索证明文件

**保留的文件**（依赖这些数据）:
- ⚠️ `index_db.json` - 搜索需要索引数据库
- ⚠️ `search_db.json` - 搜索需要搜索数据库
- ⚠️ `EncFiles/*` - 搜索需要加密文件

**生成的结果**:
- 📊 `search_results_YYYYMMDD_HHMMSS/`
  - `search_detailed.csv` - 每个关键词的详细性能数据
  - `search_summary.json` - 统计摘要

---

#### 3️⃣ 验证性能测试

测试证明验证的性能。

```bash
cd system_test
./run_verify_test.sh
```

**前提条件**:
- ⚠️ 必须先运行过 `run_insert_test.sh`（需要数据库）
- ⚠️ 必须先运行过 `run_search_test.sh`（需要证明文件）

**特点**:
- 🔥 **完全只读**，不修改任何数据
- 🔥 **可重复运行**，无副作用
- 🔥 **智能检查**: 自动检查数据库和证明文件

**读取的文件**:
- 📖 `SearchProof/*.json` - 搜索证明文件
- 📖 `index_db.json` - 索引数据库
- 📖 `search_db.json` - 搜索数据库

**生成的结果**:
- 📊 `verify_results_YYYYMMDD_HHMMSS/`
  - `verify_detailed.csv` - 每个证明的详细验证性能数据
  - `verify_summary.json` - 统计摘要

---

### 完整测试流程示例

如果您想运行完整的测试流程，按顺序执行三个脚本：

```bash
cd system_test

# 第一步：插入测试（清理所有数据，重新插入）
./run_insert_test.sh

# 等待完成后，运行第二步

# 第二步：搜索测试（清理搜索文件，保留数据库）
./run_search_test.sh

# 等待完成后，运行第三步

# 第三步：验证测试（只读取证明文件）
./run_verify_test.sh
```

每个测试完成后，脚本会提示下一步运行哪个测试。

---

### 智能依赖检查示例

脚本会自动检查依赖并给出清晰的提示：

**示例1**: 运行搜索测试，但数据库不存在

```bash
$ ./run_search_test.sh

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
检查依赖
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

❌ 未找到索引数据库: ../Storage-node/data/index_db.json

搜索测试需要插入测试产生的数据库文件。
请先运行插入测试：
  ./run_insert_test.sh
```

**示例2**: 运行验证测试，但证明文件不存在

```bash
$ ./run_verify_test.sh

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
检查依赖
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

❌ 未找到证明文件

验证测试需要搜索测试产生的证明文件。
请先运行搜索测试：
  ./run_search_test.sh
```

---

### 方式二：使用 make 命令（备选）

如果您习惯使用 make，也可以直接进入各个测试目录运行：

#### 插入测试

```bash
cd system_test/insert_files

# 编译并运行
make run

# 查看结果
make show-results

# 清理编译文件
make clean
```

#### 搜索测试

```bash
cd system_test/search_files

# 编译并运行
make run

# 查看结果
make show-results
```

#### 验证测试

```bash
cd system_test/verify_files

# 编译并运行
make run

# 查看结果
make show-results
```

---

### 方式三：端到端自动化测试（可选）

如果您想自动连续运行所有测试，可以使用端到端脚本：

```bash
cd system_test

# 完整测试（使用所有数据）
./run_end_to_end_test.sh full

# 快速测试（使用少量数据）
./run_end_to_end_test.sh quick

# 标准测试
./run_end_to_end_test.sh standard
```

⚠️ **注意**: 端到端脚本会自动连续运行插入和搜索测试，不适合单独重复测试某个模块。

---

## 📁 项目结构

```
system_test/
├── 🚀 独立测试脚本（推荐）
│   ├── run_insert_test.sh       # 插入测试脚本
│   ├── run_search_test.sh       # 搜索测试脚本
│   └── run_verify_test.sh       # 验证测试脚本
│
├── 📚 文档
│   ├── README.md                # 本文档（快速开始指南）
│   ├── README_INDEPENDENT_TESTS.md  # 独立测试详细指南
│   └── DATA_CLEANUP_GUIDE.md    # 数据清理详细说明
│
├── 🛠️ 工具脚本
│   ├── test_cleanup_demo.sh     # 交互式测试演示
│   ├── run_end_to_end_test.sh   # 端到端自动化测试
│   ├── verify_config.sh         # 配置文件验证
│   └── install_dependencies.sh  # 依赖库安装
│
├── insert_files/                # 插入性能测试
│   ├── config/
│   │   └── insert_test_config.json
│   ├── data/
│   │   └── database1_keywords.json
│   ├── results/                 # 结果输出目录
│   ├── insert_test.h
│   ├── insert_test.cpp
│   └── Makefile
│
├── search_files/                # 搜索性能测试
│   ├── config/
│   │   └── search_test_config.json
│   ├── data/
│   │   └── search_keywords.json
│   ├── results/                 # 结果输出目录
│   ├── search_test.h
│   ├── search_test.cpp
│   └── Makefile
│
└── verify_files/                # 验证性能测试
    ├── config/
    │   └── verify_test_config.json
    ├── results/                 # 结果输出目录
    ├── verify_test.h
    ├── verify_test.cpp
    └── Makefile
```

---

## 📊 性能指标说明

### 插入性能测试指标

| 指标 | 说明 |
|------|------|
| **t1_ms** | 客户端加密时间（毫秒） |
| **t3_ms** | 服务端插入时间（毫秒） |
| **s1_bytes** | 明文文件大小（字节） |
| **s2_bytes** | 密文文件大小（字节） |
| **s3_bytes** | Insert JSON 大小（字节） |
| **encrypt_ratio** | 加密膨胀率 = s2 / s1 |
| **metadata_ratio** | 元数据占比 = s3 / s2 |
| **total_overhead** | 总开销 = (s2 + s3) / s1 - 1 |
| **client_throughput_mbps** | 客户端吞吐量（MB/s） |
| **server_throughput_mbps** | 服务端吞吐量（MB/s） |

统计数据：平均值、最小值、最大值、标准差

### 搜索性能测试指标

| 指标 | 说明 |
|------|------|
| **t_client_token_gen_ms** | 客户端Token生成时间（毫秒） |
| **t_server_proof_calc_ms** | 服务端纯证明计算时间（毫秒） |
| **token_size_bytes** | Token文件大小（字节） |
| **proof_size_bytes** | 证明文件大小（字节） |
| **result_count** | 命中文件数 |

统计数据：总时间、平均值、最小值、最大值、标准差、QPS

### 验证性能测试指标

| 指标 | 说明 |
|------|------|
| **t_verify_ms** | 纯证明验证时间（毫秒） |
| **proof_size_bytes** | 证明文件大小（字节） |
| **result_count** | 证明中的文件数 |
| **success** | 是否验证成功 |

统计数据：总验证时间、平均值、最小值、最大值、标准差、吞吐量

---

## 🔄 数据清理说明

### 每个测试的清理范围

**插入测试** - 清理所有数据库和文件（完全重置）
```
✅ 清理客户端：
   - EncFiles/*          (所有加密文件)
   - MetaFiles/*         (所有元数据文件)
   - Insert/*            (所有插入JSON)
   - keyword_states.json (关键词状态)

✅ 清理服务端：
   - index_db.json       (索引数据库)
   - search_db.json      (搜索数据库)
   - metadata/*          (元数据目录)
   - EncFiles/*          (加密文件目录)
```

**搜索测试** - 只清理搜索文件，保留数据库
```
✅ 清理客户端：
   - Search/*.json       (搜索token文件)

✅ 清理服务端：
   - SearchProof/*.json  (搜索证明文件)

⚠️ 保留（依赖这些数据）：
   - index_db.json       (搜索需要)
   - search_db.json      (搜索需要)
   - EncFiles/*          (搜索需要)
```

**验证测试** - 不清理（只读测试）
```
📖 只读取：
   - SearchProof/*.json  (搜索证明文件)
   - index_db.json       (索引数据库)
   - search_db.json      (搜索数据库)

✅ 不修改任何文件
```

详细的数据清理说明请参考：`DATA_CLEANUP_GUIDE.md`

---

## 📖 使用场景示例

### 场景1: 首次运行完整测试

```bash
cd system_test

# 第一次运行，从零开始
./run_insert_test.sh    # 插入所有文件
./run_search_test.sh    # 搜索所有关键词
./run_verify_test.sh    # 验证所有证明
```

### 场景2: 只想重新测试搜索性能

```bash
cd system_test

# 不需要重新插入文件，直接测试搜索
./run_search_test.sh    # 只清理搜索文件，保留数据库
```

✅ **好处**: 不需要重新插入，节省大量时间
⚠️ **注意**: 会清理之前的证明文件，需要重新运行验证测试

### 场景3: 反复测试验证性能

```bash
cd system_test

# 验证测试可以无限次运行
./run_verify_test.sh    # 第一次
./run_verify_test.sh    # 第二次
./run_verify_test.sh    # 第三次
# ... 不会修改任何数据
```

✅ **好处**: 完全无副作用，可以反复运行

### 场景4: 重新从零开始

```bash
cd system_test

# 重新插入所有文件（会清理所有数据库）
./run_insert_test.sh

# 然后依次运行搜索和验证
./run_search_test.sh
./run_verify_test.sh
```

⚠️ **注意**: 插入测试会删除所有现有数据

---

## ⚙️ 系统要求

### 必需的依赖库

**重要**: 编译前必须安装以下开发包：

- **C++ 编译器**: g++ (支持 C++17)
- **PBC 库**: libpbc-dev
- **GMP 库**: libgmp-dev
- **OpenSSL**: libssl-dev
- **JsonCpp**: libjsoncpp-dev

### 一键安装（推荐）

```bash
cd system_test
sudo ./install_dependencies.sh
```

### 手动安装（Ubuntu/Debian）

```bash
sudo apt-get update
sudo apt-get install -y build-essential libpbc-dev libgmp-dev libssl-dev libjsoncpp-dev
```

### 安装（macOS）

```bash
brew install pbc gmp openssl jsoncpp
```

---

## 🔧 故障排除

### Q1: 运行搜索测试时提示"未找到数据库"

**A**: 您需要先运行插入测试：
```bash
./run_insert_test.sh
```

---

### Q2: 运行验证测试时提示"未找到证明文件"

**A**: 您需要先运行搜索测试：
```bash
./run_search_test.sh
```

---

### Q3: 我想重新测试搜索性能，但不想重新插入文件

**A**: 直接运行搜索测试即可：
```bash
./run_search_test.sh  # 只清理搜索文件，不删除数据库
```

---

### Q4: 编译错误：找不到 pbc/pbc.h

**A**: PBC 库未安装，运行：
```bash
sudo ./install_dependencies.sh
```

---

### Q5: 如何查看当前数据状态？

**A**: 使用交互式演示脚本：
```bash
./test_cleanup_demo.sh
```

---

## 📚 详细文档

- **`README_INDEPENDENT_TESTS.md`** - 三个独立测试的完整使用指南
  - 详细的使用示例
  - 依赖关系说明
  - 常见问题解答

- **`DATA_CLEANUP_GUIDE.md`** - 数据清理详细说明
  - 每个测试的清理范围
  - 手动清理命令
  - 故障排查指南

- **`test_cleanup_demo.sh`** - 交互式演示脚本
  - 可视化数据状态
  - 智能依赖检查
  - 一键运行测试

---

## 🎯 推荐工作流程

### 日常开发测试

1. **首次测试**: 运行完整流程
   ```bash
   ./run_insert_test.sh
   ./run_search_test.sh
   ./run_verify_test.sh
   ```

2. **修改搜索算法后**: 只重新测试搜索
   ```bash
   ./run_search_test.sh  # 保留数据库，只测搜索
   ```

3. **修改验证算法后**: 只重新测试验证
   ```bash
   ./run_verify_test.sh  # 完全只读，可反复测试
   ```

4. **需要新数据时**: 重新运行插入测试
   ```bash
   ./run_insert_test.sh  # 清理所有数据，重新插入
   ```

---

## 🤝 获取帮助

每个脚本都支持 `--help` 选项：

```bash
./run_insert_test.sh --help
./run_search_test.sh --help
./run_verify_test.sh --help
```

---

## 📝 总结

✅ **推荐使用**: 三个独立测试脚本
- `run_insert_test.sh` - 插入测试
- `run_search_test.sh` - 搜索测试
- `run_verify_test.sh` - 验证测试

✅ **每个测试独立运行**
- 有自己的清理逻辑
- 有自己的结果目录
- 可以单独重复运行

✅ **智能依赖检查**
- 自动检查前置条件
- 明确提示缺少的数据
- 引导用户运行正确的测试

✅ **灵活使用**
- 可以完整流程运行
- 可以单独运行某个测试
- 可以重复运行测试

---

**最后更新**: 2025-12-31
**版本**: 2.0
