# 去中心化存储节点 v3.0 (本地版)

完全本地化的去中心化存储节点实现,使用JSON文件进行数据持久化,无需区块链依赖。

## ✨ 主要特性

- ✅ **完全本地化**: 所有数据存储在本地文件系统
- ✅ **JSON持久化**: 使用JSON文件管理配置和索引
- ✅ **交互式控制台**: 友好的命令行界面
- ✅ **密码学支持**: 基于PBC库的配对密码学
- ✅ **无区块链依赖**: 独立运行,不需要以太坊或其他区块链

## 📋 功能列表

1. **文件插入** - 插入加密文件及其索引
2. **关键词搜索** - 基于索引的关键词搜索
3. **文件检索** - 根据文件ID检索加密文件
4. **文件删除** - 删除文件及其索引
5. **完整性证明** - 生成文件完整性证明
6. **状态查看** - 查看节点运行状态
7. **文件列表** - 列出所有存储的文件
8. **元数据导出** - 导出文件元数据到JSON

## 🛠️ 依赖库

```bash
# 必需的库
- OpenSSL (libssl-dev)
- PBC (libpbc-dev)
- GMP (libgmp-dev)
- JsonCpp (libjsoncpp-dev)
```

## 📦 安装依赖

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    libssl-dev \
    libgmp-dev \
    libpbc-dev \
    libjsoncpp-dev
```

### macOS
```bash
brew install openssl gmp pbc jsoncpp
```

## 🔨 编译

```bash
# 编译所有文件
g++ -o storage_node main.cpp storage_node.cpp \
    -lpbc -lgmp -lssl -lcrypto -ljsoncpp -std=c++11

# 或使用优化编译
g++ -O2 -o storage_node main.cpp storage_node.cpp \
    -lpbc -lgmp -lssl -lcrypto -ljsoncpp -std=c++11
```

## 🚀 使用方法

### 启动程序

```bash
# 使用默认配置 (数据目录: ./data, 端口: 9000)
./storage_node

# 指定数据目录
./storage_node /path/to/data

# 指定数据目录和端口
./storage_node /path/to/data 8080
```

### 首次运行

程序首次运行时会自动创建必要的目录和配置文件:

```
data/
├── config.json          # 节点配置
├── node_info.json       # 节点信息
├── index_db.json        # 索引数据库
├── files/               # 加密文件存储
└── metadata/            # 文件元数据
```

## 📝 使用示例

### 1. 插入文件

准备两个文件:
- `insert_params.json` - 参数文件
- `encrypted_file.enc` - 加密文件

**insert_params.json 格式:**
```json
{
    "file_id": "file_12345",
    "Ts": ["state_token_1", "state_token_2", "state_token_3"],
    "keywords": ["keyword_1", "keyword_2", "keyword_3"],
    "pointer": "encrypted_pointer_abc123",
    "file_auth_tag": "file_authentication_tag",
    "metadata": {
        "filename": "document.pdf",
        "size": 102400,
        "upload_time": "2025-11-09T10:30:00Z"
    }
}
```

在控制台中:
```
请选择操作 [0-9]: 1
请输入参数JSON文件路径: ./insert_params.json
请输入加密文件路径: ./encrypted_file.enc
```

### 2. 搜索关键词

```
请选择操作 [0-9]: 2
请输入搜索令牌 (Ts): state_token_1
请输入最新状态 (可选): 
请输入种子 (可选): 
```

### 3. 检索文件

```
请选择操作 [0-9]: 3
请输入文件ID: file_12345
是否保存密文到文件? (y/n): y
输出文件路径: ./retrieved_file.enc
```

### 4. 删除文件

```
请选择操作 [0-9]: 4
请输入文件ID: file_12345
请输入删除证明 (可选): 
确认删除? (y/n): y
```

## 📊 JSON文件格式

### config.json
```json
{
    "version": "3.0",
    "node": {
        "node_id": "storage_node_001",
        "created_at": "2025-11-09T10:00:00Z"
    },
    "paths": {
        "data_dir": "./data",
        "files_dir": "./data/files",
        "metadata_dir": "./data/metadata",
        "index_db": "./data/index_db.json"
    },
    "server": {
        "port": 9000
    },
    "storage": {
        "max_file_size_mb": 100,
        "max_total_storage_gb": 10
    }
}
```

### node_info.json
```json
{
    "node_id": "storage_node_001",
    "status": "active",
    "last_updated": "2025-11-09T15:30:00Z",
    "statistics": {
        "total_files": 5,
        "total_index_entries": 15
    }
}
```

### index_db.json
```json
{
    "version": "1.0",
    "last_updated": "2025-11-09T15:30:00Z",
    "total_entries": 3,
    "indices": {
        "state_token_1": [
            {
                "Ts": "state_token_1",
                "keyword": "keyword_1",
                "pointer": "encrypted_pointer",
                "file_identifier": "file_12345",
                "valid": true
            }
        ]
    }
}
```

## 🔒 安全说明

1. **密钥管理**: 确保加密密钥安全存储
2. **访问控制**: 限制数据目录的访问权限
3. **备份**: 定期备份 `data/` 目录
4. **传输安全**: 加密文件在传输时使用安全通道

## 🐛 故障排查

### 编译错误

**问题**: 找不到PBC库
```
solution: 确保已安装 libpbc-dev
sudo apt-get install libpbc-dev
```

**问题**: 找不到jsoncpp
```
solution: 确保已安装 libjsoncpp-dev
sudo apt-get install libjsoncpp-dev
```

### 运行时错误

**问题**: 无法创建目录
```
solution: 检查文件系统权限
chmod 755 data/
```

**问题**: JSON解析失败
```
solution: 检查JSON文件格式是否正确
使用在线JSON验证器: https://jsonlint.com/
```

## 📖 API参考

### StorageNode 类

#### 构造函数
```cpp
StorageNode(const std::string& data_directory = "./data", int port = 9000);
```

#### 主要方法

```cpp
// 初始化
bool setup_cryptography();
bool initialize_directories();
bool load_config();

// 文件操作
bool insert_file(const std::string& param_json_path, const std::string& enc_file_path);
bool delete_file(const std::string& file_id, const std::string& del_proof);
SearchResult search_keyword(const std::string& search_token, ...);
Json::Value retrieve_file(const std::string& file_id);

// 工具方法
std::vector<std::string> list_all_files();
Json::Value get_file_metadata(const std::string& file_id);
bool export_file_metadata(const std::string& file_id, const std::string& output_path);
```

## 🔄 版本历史

### v3.0 (当前版本)
- ✅ 完全本地化,移除所有区块链依赖
- ✅ JSON文件持久化
- ✅ 交互式控制台界面
- ✅ 简化的文件插入流程

### v2.0
- 增强的系统健康检查
- 交易前强制验证
- 详细的错误诊断

### v1.0
- 基础存储功能
- 区块链集成
- 密码学支持

## 📄 许可证

MIT License

## 👥 贡献

欢迎提交问题和拉取请求!

## 📧 联系方式

如有问题,请通过GitHub Issues联系。

---

**注意**: 此版本为本地存储版本,不包含区块链功能。如需区块链集成,请参考v2.0版本。