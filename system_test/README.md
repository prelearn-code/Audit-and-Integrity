# VDS 性能测试框架

完整的端到端性能测试框架，用于测试 VDS (Verifiable Dynamic Searchable) 系统的插入、搜索和验证性能。

## 📁 项目结构

```
system_test/
├── insert_files/              # 插入性能测试
│   ├── config/
│   │   └── insert_test_config.json   # 插入测试配置
│   ├── data/
│   │   └── database1_keywords.json   # 测试数据（文件->关键词映射）
│   ├── results/               # 测试结果输出目录（自动创建）
│   ├── insert_test.h          # 插入测试类定义
│   ├── insert_test.cpp        # 插入测试类实现（包含main函数）
│   └── Makefile               # 编译配置
│
├── search_files/              # 搜索性能测试
│   ├── config/
│   │   └── search_test_config.json   # 搜索测试配置
│   ├── data/
│   │   └── search_keywords.json      # 搜索关键词列表
│   ├── results/               # 测试结果输出目录（自动创建）
│   ├── search_test.h          # 搜索测试类定义
│   ├── search_test.cpp        # 搜索测试类实现（包含main函数）
│   └── Makefile               # 编译配置
│
├── verify_files/              # 证明验证性能测试
│   ├── config/
│   │   └── verify_test_config.json   # 验证测试配置
│   ├── data/                  # （从搜索测试读取证明文件）
│   ├── results/               # 测试结果输出目录（自动创建）
│   ├── verify_test.h          # 验证测试类定义
│   ├── verify_test.cpp        # 验证测试类实现（包含main函数）
│   └── Makefile               # 编译配置
│
├── install_dependencies.sh    # 依赖库自动安装脚本
├── verify_config.sh           # 配置文件路径验证脚本
├── run_end_to_end_test.sh     # 端到端测试自动化脚本
└── README.md                  # 本文档
```

## 🚀 快速开始

### 步骤 0：验证配置（可选但推荐）

在运行测试前，验证所有配置文件路径是否正确：

```bash
cd system_test
./verify_config.sh
```

如果验证失败，请检查缺失的文件和目录。

### 方式一：使用自动化脚本（推荐）

端到端测试脚本会自动运行插入和搜索测试，并生成综合报告。

```bash
cd system_test

# 快速测试（使用少量数据）
./run_end_to_end_test.sh quick

# 标准测试
./run_end_to_end_test.sh standard

# 完整测试（使用所有数据）
./run_end_to_end_test.sh full
```

### 方式二：单独运行测试

#### 1. 插入性能测试

```bash
cd system_test/insert_files

# 编译
make

# 运行（使用默认配置）
make run

# 使用自定义配置
make run-config CONFIG=my_config.json

# 查看结果
make show-results

# 清理
make clean
```

#### 2. 搜索性能测试

```bash
cd system_test/search_files

# 编译
make

# 运行（使用默认配置）
make run

# 使用自定义配置
make run-config CONFIG=my_config.json

# 查看结果
make show-results

# 清理
make clean
```

#### 3. 证明验证性能测试

**注意**：验证测试需要先运行搜索测试生成证明文件。

```bash
cd system_test/verify_files

# 编译
make

# 运行（使用默认配置）
make run

# 使用自定义配置
make run-config CONFIG=my_config.json

# 查看结果
make show-results

# 清理
make clean
```

#### 4. 完整测试流程（推荐）

按顺序运行插入、搜索、验证测试：

```bash
# 1. 插入测试
cd system_test/insert_files && make run

# 2. 搜索测试（生成证明文件）
cd ../search_files && make run

# 3. 验证测试（验证证明文件）
cd ../verify_files && make run
```

### 方式三：使用 VSCode 任务

在 VSCode 中按 `Ctrl+Shift+P`，选择 "Tasks: Run Task"，然后选择：

- `build:insert_perf_test` - 编译插入测试
- `run:insert_perf_test` - 运行插入测试
- `build:search_perf_test` - 编译搜索测试
- `run:search_perf_test` - 运行搜索测试
- `build:verify_perf_test` - 编译验证测试
- `run:verify_perf_test` - 运行验证测试
- `run:end_to_end_test (quick)` - 运行快速端到端测试
- `run:end_to_end_test (standard)` - 运行标准端到端测试
- `run:end_to_end_test (full)` - 运行完整端到端测试
- `clean:all_tests` - 清理所有编译文件

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

统计数据包括：平均值、最小值、最大值、标准差

### 搜索性能测试指标

| 指标 | 说明 |
|------|------|
| **t_client_token_gen_ms** | 客户端Token生成时间（毫秒） |
| **t_server_proof_calc_ms** | 服务端纯证明计算时间（毫秒，不含加载） |
| **token_size_bytes** | Token文件大小（字节） |
| **proof_size_bytes** | 证明文件大小（字节） |
| **result_count** | 命中文件数 |
| **success** | 是否成功 |

统计数据包括：
- 客户端：总时间、平均值、最小值、最大值、标准差、QPS
- 服务端：总时间、平均值、最小值、最大值、标准差、QPS

### 证明验证性能测试指标

| 指标 | 说明 |
|------|------|
| **t_verify_ms** | 纯证明验证时间（毫秒，不含加载） |
| **proof_size_bytes** | 证明文件大小（字节） |
| **result_count** | 证明中的文件数 |
| **success** | 是否验证成功 |

统计数据包括：总验证时间、平均值、最小值、最大值、标准差、验证吞吐量（验证/秒）

## 📝 配置文件说明

### 插入测试配置 (insert_test_config.json)

```json
{
  "test_name": "database1 insert performance",
  "paths": {
    "keywords_file": "system_test/insert_files/data/database1_keywords.json",
    "dataset_root": "make_data/database1",
    "public_params": "vds-client/data/public_params.json",
    "private_key": "vds-client/data/private_key.dat",
    "client": {
      "data_dir": "vds-client/data",
      "insert_dir": "vds-client/data/Insert",
      "enc_dir": "vds-client/data/EncFiles",
      "metadata_dir": "vds-client/data/MetaFiles",
      "search_dir": "vds-client/data/Search",
      "deles_dir": "vds-client/data/Deles",
      "keyword_states_file": "vds-client/data/keyword_states.json"
    },
    "server": {
      "data_dir": "Storage-node/data",
      "insert_dir": "vds-client/data/Insert",
      "enc_dir": "vds-client/data/EncFiles",
      "port": 9000
    }
  },
  "options": {
    "max_files": 0,           // 0 = 测试所有文件
    "verbose": true,          // 显示详细日志
    "save_intermediate": true // 保存中间文件
  }
}
```

### 搜索测试配置 (search_test_config.json)

```json
{
  "test_name": "database1 search performance",
  "paths": {
    "keywords_file": "system_test/search_files/data/search_keywords.json",
    "public_params": "vds-client/data/public_params.json",
    "private_key": "vds-client/data/private_key.dat",
    "client": {
      "data_dir": "vds-client/data",
      "keyword_states_file": "vds-client/data/keyword_states.json"
    },
    "server": {
      "data_dir": "Storage-node/data",
      "search_proof_dir": "Storage-node/data/SearchProof",
      "port": 9000
    }
  },
  "options": {
    "max_keywords": 0,         // 0 = 测试所有关键词
    "verbose": true,
    "save_intermediate": true,
    "use_keyword_states": true,  // 使用插入测试生成的 keyword_states
    "verify_proof": true         // 验证搜索证明
  }
}
```

### 验证测试配置 (verify_test_config.json)

```json
{
  "test_name": "database1 verify performance",
  "paths": {
    "proof_dir": "../../Storage-node/data/SearchProof",
    "public_params": "../../vds-client/data/public_params.json",
    "private_key": "../../vds-client/data/private_key.dat",
    "client": {
      "data_dir": "../../vds-client/data"
    },
    "server": {
      "data_dir": "../../Storage-node/data",
      "port": 9000
    }
  },
  "options": {
    "max_proofs": 0,           // 0 = 验证所有证明
    "verbose": true,
    "save_intermediate": true
  }
}
```

## 📂 输出结果

### 插入测试结果

- **insert_detailed.csv** - 每个文件的详细性能数据（CSV格式）
- **insert_summary.json** - 统计摘要（JSON格式）

### 搜索测试结果

- **search_detailed.csv** - 每个关键词的详细性能数据（CSV格式）
- **search_summary.json** - 统计摘要（JSON格式）

### 验证测试结果

- **verify_detailed.csv** - 每个证明的详细验证性能数据（CSV格式）
- **verify_summary.json** - 统计摘要（JSON格式）

### 端到端测试结果

运行端到端测试后，结果保存在 `end_to_end_results_<timestamp>/` 目录：

```
end_to_end_results_20250130_123456/
├── insert/
│   ├── insert_detailed.csv
│   └── insert_summary.json
├── search/
│   ├── search_detailed.csv
│   └── search_summary.json
└── summary_report.md          # Markdown 格式的综合报告
```

## 🔄 数据流说明

```
1. 插入测试
   ├─> 读取 database1_keywords.json（文件路径->关键词映射）
   ├─> 客户端加密文件（生成 EncFiles/*.enc）
   ├─> 生成插入 JSON（生成 Insert/*.json）
   ├─> 服务端插入文件
   └─> 生成 keyword_states.json（记录所有已插入关键词）

2. 搜索测试
   ├─> 选项 A：使用 keyword_states.json（插入测试生成的）
   │   └─> 测试所有已插入的关键词
   │
   └─> 选项 B：使用 search_keywords.json
       └─> 测试指定的关键词列表

   性能测量：
   ├─> 客户端：只测量 Token 生成时间
   └─> 服务端：只测量纯证明计算时间（数据库已预加载）
   └─> 生成证明文件（SearchProof/proof_*.json）

3. 验证测试
   ├─> 读取搜索测试生成的证明文件（SearchProof/proof_*.json）
   ├─> 预加载数据库和公共参数（不计入性能测试）
   └─> 只测量纯证明验证时间

4. 端到端测试
   └─> 自动运行插入测试 → 搜索测试 → 生成综合报告
```

## ⚙️ 系统要求

### 必需的依赖库

**重要**: 编译前必须安装以下开发包（包含头文件和库文件）：

- **C++ 编译器**: g++ (支持 C++17)
- **PBC 库**: Pairing-Based Cryptography library - **libpbc-dev**
- **GMP 库**: GNU Multiple Precision library - **libgmp-dev**
- **OpenSSL**: libcrypto - **libssl-dev**
- **JsonCpp**: JSON 解析库 - **libjsoncpp-dev**

### 一键安装脚本（推荐）

```bash
# 运行自动安装脚本
cd system_test
sudo ./install_dependencies.sh
```

脚本会自动：
- 检测系统类型
- 安装所有必需的开发包
- 验证安装结果
- 如果 PBC 库不在标准源，会自动编译安装

### 手动安装（Ubuntu/Debian）

```bash
# 安装所有必需的开发包
sudo apt-get update
sudo apt-get install -y build-essential libpbc-dev libgmp-dev libssl-dev libjsoncpp-dev

# 验证安装
dpkg -l | grep -E "libpbc-dev|libgmp-dev|libssl-dev|libjsoncpp-dev"
```

**注意**: 如果 `libpbc-dev` 不可用，需要手动编译 PBC 库：

```bash
# 下载 PBC 库
wget https://crypto.stanford.edu/pbc/files/pbc-0.5.14.tar.gz
tar -xzf pbc-0.5.14.tar.gz
cd pbc-0.5.14

# 编译安装
./configure
make
sudo make install
sudo ldconfig
```

### 安装依赖（macOS）

```bash
brew install pbc gmp openssl jsoncpp
```

### 验证环境

```bash
# 检查编译器
g++ --version

# 检查头文件
ls /usr/include/pbc/
ls /usr/include/gmp.h
ls /usr/include/json/
```

## 🔧 故障排除

### 编译错误：找不到 pbc/pbc.h

**问题**: PBC 库未安装或路径不正确

**解决方案**:
```bash
# 检查 PBC 库是否安装
ls /usr/local/include/pbc/
ls /usr/include/pbc/

# 如果未安装，安装 PBC 库
sudo apt-get install libpbc-dev
```

### 运行时错误：找不到配置文件

**问题**: 配置文件路径不正确

**解决方案**:
```bash
# 确保在正确的目录运行
cd system_test/insert_files
./insert_perf_test

# 或使用绝对路径
./insert_perf_test /path/to/config.json
```

### 测试数据文件不存在

**问题**: database1_keywords.json 或测试数据文件不存在

**解决方案**:
```bash
# 检查测试数据是否存在
ls system_test/insert_files/data/database1_keywords.json
ls make_data/database1/

# 如果数据不存在，需要先生成测试数据
# （根据您的数据生成流程）
```

### 性能回调报错

**问题**: 性能监控回调函数报错

**解决方案**:
- 确保 StorageClient 和 StorageNode 支持性能回调接口
- 检查 `PerformanceCallback_c` 和 `PerformanceCallback_s` 结构定义

## 📖 使用示例

### 示例 1：快速测试前 10 个文件

修改配置文件 `insert_test_config.json`:

```json
{
  "options": {
    "max_files": 10,
    "verbose": true
  }
}
```

运行测试：
```bash
cd system_test/insert_files
make run
```

### 示例 2：测试特定关键词

创建自定义关键词文件 `my_keywords.json`:

```json
{
  "keywords": ["software", "meeting", "project"]
}
```

修改 `search_test_config.json`:

```json
{
  "paths": {
    "keywords_file": "system_test/search_files/data/my_keywords.json"
  },
  "options": {
    "use_keyword_states": false
  }
}
```

运行测试：
```bash
cd system_test/search_files
make run
```

### 示例 3：端到端测试并分析结果

```bash
# 运行完整测试
cd system_test
./run_end_to_end_test.sh full

# 查看综合报告
cat end_to_end_results_*/summary_report.md

# 使用 jq 分析 JSON 结果
jq '.statistics.t1_avg' end_to_end_results_*/insert/insert_summary.json
jq '.statistics.t_server_avg' end_to_end_results_*/search/search_summary.json
```

## 📚 扩展阅读

- **性能测试类文档**: 查看 `insert_test.h` 和 `search_test.h` 中的详细注释
- **VDS 系统文档**: 参考项目根目录的文档
- **性能优化指南**: TBD

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

（根据项目许可证填写）

---

**最后更新**: 2025-01-30
**版本**: 1.0
