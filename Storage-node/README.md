# 去中心化存储节点 v3.0 (本地版)

# 去中心化存储节点 v3.1 使用说明

## 📋 目录

1. [简介](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#%E7%AE%80%E4%BB%8B)
2. [版本更新](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#%E7%89%88%E6%9C%AC%E6%9B%B4%E6%96%B0)
3. [编译与运行](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#%E7%BC%96%E8%AF%91%E4%B8%8E%E8%BF%90%E8%A1%8C)
4. [功能概述](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#%E5%8A%9F%E8%83%BD%E6%A6%82%E8%BF%B0)
5. [详细使用指南](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#%E8%AF%A6%E7%BB%86%E4%BD%BF%E7%94%A8%E6%8C%87%E5%8D%97)
   * [1. 插入文件](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#1-%E6%8F%92%E5%85%A5%E6%96%87%E4%BB%B6)
   * [2. 搜索关键词](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#2-%E6%90%9C%E7%B4%A2%E5%85%B3%E9%94%AE%E8%AF%8D)
   * [3. 检索文件](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#3-%E6%A3%80%E7%B4%A2%E6%96%87%E4%BB%B6)
   * [4. 删除文件](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#4-%E5%88%A0%E9%99%A4%E6%96%87%E4%BB%B6)
   * [5. 生成完整性证明](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#5-%E7%94%9F%E6%88%90%E5%AE%8C%E6%95%B4%E6%80%A7%E8%AF%81%E6%98%8E)
   * [6. 查看状态](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#6-%E6%9F%A5%E7%9C%8B%E7%8A%B6%E6%80%81)
6. [JSON格式说明](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#json%E6%A0%BC%E5%BC%8F%E8%AF%B4%E6%98%8E)
7. [文件结构](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#%E6%96%87%E4%BB%B6%E7%BB%93%E6%9E%84)
8. [常见问题](https://15659118.4omini.xyz/chat/5b39992c-53a8-46d9-953d-7380290ca4ae#%E5%B8%B8%E8%A7%81%E9%97%AE%E9%A2%98)

---

## 简介

**去中心化存储节点 v3.1** 是一个支持加密文件存储、关键词搜索和身份验证的本地存储系统。

### 核心特性

* ✅ **完全本地化存储** - 无需区块链或网络连接
* ✅ **JSON持久化** - 数据以JSON格式存储，易于管理
* ✅ **交互式控制台** - 友好的命令行界面
* ✅ **PK身份验证** - 基于客户端公钥的权限控制 **(v3.1新增)**
* ✅ **文件状态管理** - 支持 valid/invalid 状态 **(v3.1新增)**
* ✅ **密码学安全** - 使用PBC库和OpenSSL进行加密操作

---

## 版本更新

### v3.1 主要更新 (当前版本)

1. **PK身份验证系统**
   * 所有文件和索引都关联客户端公钥 (PK)
   * 删除和搜索操作需要PK验证
   * 只有文件所有者才能删除文件
2. **文件状态管理**
   * 状态字段从 bool 改为 string ("valid"/"invalid")
   * 删除操作标记文件为 "invalid" 而非物理删除
   * 搜索只返回 "valid" 状态的文件
3. **JSON格式更新**
   * 新的参数格式: `PK`, `ID_F`, `ptr`, `TS_F`, `state`
   * 关键词格式: `T_i`, `kt_i`
   * 更清晰的参数命名

---

## 编译与运行

### 依赖库

```bash
# Ubuntu/Debian
sudo apt-get install libpbc-dev libgmp-dev libssl-dev libjsoncpp-dev

# 或手动编译 PBC 和 GMP
```

### 编译

```bash
# 基本编译
g++ -o storage_node main.cpp storage_node.cpp \
    -lpbc -lgmp -lcrypto -ljsoncpp -std=c++11

# 启用调试信息
g++ -g -o storage_node main.cpp storage_node.cpp \
    -lpbc -lgmp -lcrypto -ljsoncpp -std=c++11
```

### 运行

```bash
# 使用默认参数 (数据目录: ./data, 端口: 9000)
./storage_node

# 指定数据目录
./storage_node /path/to/data

# 指定数据目录和端口
./storage_node /path/to/data 9001
```

---

## 功能概述


| 功能       | 菜单选项 | 需要PK        | 说明                   |
| ---------- | -------- | ------------- | ---------------------- |
| 插入文件   | 1        | ✅ (在JSON中) | 上传加密文件到存储节点 |
| 搜索关键词 | 2        | ✅            | 根据关键词搜索文件     |
| 检索文件   | 3        | ❌            | 获取文件内容           |
| 删除文件   | 4        | ✅            | 标记文件为无效         |
| 完整性证明 | 5        | ❌            | 生成文件完整性证明     |
| 查看状态   | 6        | ❌            | 显示节点基本状态       |
| 列出文件   | 7        | ❌            | 显示所有文件列表       |
| 导出元数据 | 8        | ❌            | 导出文件元数据         |
| 详细状态   | 9        | ❌            | 显示详细统计信息       |

---

## 详细使用指南

### 1. 插入文件

**功能**: 将加密文件和索引信息插入存储节点

#### 输入参数

**1.1 参数JSON文件** (必需)

文件格式: `insert.json`

```json
{
    "PK": "客户端公钥(hex字符串)",
    "ID_F": "文件唯一标识符",
    "ptr": "文件指针",
    "TS_F": "文件认证标签",
    "state": "valid",
    "keywords": [
        {
            "T_i": "关键词状态令牌_1",
            "kt_i": "关键词_1"
        },
        {
            "T_i": "关键词状态令牌_2",
            "kt_i": "关键词_2"
        }
    ],
    "metadata": {
        "description": "可选的元数据",
        "tags": ["标签1", "标签2"]
    }
}
```

**参数说明**:


| 参数               | 类型   | 必需 | 说明                               |
| ------------------ | ------ | ---- | ---------------------------------- |
| `PK`               | string | ✅   | 客户端公钥，用于身份验证 (hex格式) |
| `ID_F`             | string | ✅   | 文件的唯一标识符                   |
| `ptr`              | string | ✅   | 加密的文件指针                     |
| `TS_F`             | string | ✅   | 文件认证标签/时间戳                |
| `state`            | string | ✅   | 文件状态: "valid" 或 "invalid"     |
| `keywords`         | array  | ✅   | 关键词数组                         |
| `keywords[i].T_i`  | string | ✅   | 第i个关键词的状态令牌 (用于搜索)   |
| `keywords[i].kt_i` | string | ✅   | 第i个关键词                        |
| `metadata`         | object | ❌   | 可选的元数据信息                   |

**1.2 加密文件** (必需)

* 文件格式: 任意二进制文件
* 文件路径: 本地文件系统路径
* 示例: `./encrypted_file.enc`

#### 操作步骤

1. 准备参数JSON文件 (`insert.json`)
2. 准备加密文件 (`encrypted_file.enc`)
3. 在主菜单选择 `1`
4. 输入参数JSON文件路径
5. 输入加密文件路径
6. 等待插入完成

#### 输出结果

**成功输出**:

```
✅ 文件插入成功!
   索引条目: 3
   总文件数: 1
   总索引数: 3
```

**存储位置**:

* 加密文件: `./data/files/file_001.enc`
* 元数据: `./data/metadata/file_001.json`
* 索引: `./data/index_db.json`

#### 完整示例

```bash
# 1. 创建测试文件
echo "This is encrypted content" > test.enc

# 2. 创建 insert.json
cat > insert.json << 'EOF'
{
    "PK": "1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef",
    "ID_F": "test_file_001",
    "ptr": "encrypted_ptr_xyz",
    "TS_F": "auth_tag_abc",
    "state": "valid",
    "keywords": [
        {
            "T_i": "token_search_medical",
            "kt_i": "medical"
        },
        {
            "T_i": "token_search_report",
            "kt_i": "report"
        }
    ],
    "metadata": {
        "description": "医疗报告",
        "tags": ["医疗", "2024"]
    }
}
EOF

# 3. 运行程序并插入
./storage_node
# 选择 1 (插入文件)
# 输入: insert.json
# 输入: test.enc
```

---

### 2. 搜索关键词

**功能**: 根据关键词搜索属于特定客户端的文件

#### 输入参数


| 参数           | 类型   | 必需 | 说明                                  |
| -------------- | ------ | ---- | ------------------------------------- |
| `PK`           | string | ✅   | 客户端公钥 (必须与文件插入时的PK匹配) |
| `T_i`          | string | ✅   | 搜索令牌 (对应关键词的 T\_i 值)       |
| `latest_state` | string | ❌   | 最新状态 (可选参数)                   |
| `seed`         | string | ❌   | 随机种子 (可选参数)                   |

#### 操作步骤

1. 在主菜单选择 `2`
2. 输入客户端公钥 (PK)
3. 输入搜索令牌 (T\_i)
4. 可选: 输入最新状态和种子

#### 输出结果

**找到文件**:

```
📊 搜索结果:
   找到 2 个匹配文件

📄 文件列表:
   [1] test_file_001 (关键词: medical)
   [2] test_file_002 (关键词: medical)
```

**未找到文件**:

```
📊 搜索结果:
   找到 0 个匹配文件

⚠️  未找到匹配的文件
   请检查:
   1. PK是否正确
   2. 搜索令牌是否正确
   3. 文件状态是否为 'valid'
```

#### 搜索逻辑

搜索时会进行以下过滤:

1. **PK匹配**: 只返回该PK所属的文件
2. **T\_i匹配**: 搜索令牌必须匹配
3. **状态过滤**: 只返回 state="valid" 的文件

#### 完整示例

```bash
# 在控制台中
选择操作: 2

请输入客户端公钥 (PK): 1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef
请输入搜索令牌 (T_i): token_search_medical
请输入最新状态 (可选): [回车跳过]
请输入种子 (可选): [回车跳过]

# 输出: 找到所有包含该关键词的文件
```

---

### 3. 检索文件

**功能**: 根据文件ID检索完整的文件信息

#### 输入参数


| 参数      | 类型   | 必需 | 说明                     |
| --------- | ------ | ---- | ------------------------ |
| `file_id` | string | ✅   | 文件的唯一标识符 (ID\_F) |

#### 操作步骤

1. 在主菜单选择 `3`
2. 输入文件ID
3. 查看文件信息
4. 可选: 保存密文到文件

#### 输出结果

```
✅ 文件检索成功!
   文件ID:       test_file_001
   客户端PK:     1234567890abcdef...
   密文大小:     1024 字节
   指针:         encrypted_ptr_xyz...
   认证标签:     auth_tag_abc...
   状态:         valid

是否保存密文到文件? (y/n): 
```

#### JSON响应格式

```json
{
    "success": true,
    "PK": "1234567890abcdef...",
    "file_id": "test_file_001",
    "ciphertext": "加密内容...",
    "pointer": "encrypted_ptr_xyz",
    "file_auth_tag": "auth_tag_abc",
    "state": "valid"
}
```

---

### 4. 删除文件

**功能**: 标记文件为无效状态 (需要PK身份验证)

#### 输入参数


| 参数        | 类型   | 必需 | 说明                              |
| ----------- | ------ | ---- | --------------------------------- |
| `PK`        | string | ✅   | 客户端公钥 (必须与文件所有者匹配) |
| `file_id`   | string | ✅   | 要删除的文件ID                    |
| `del_proof` | string | ❌   | 删除证明 (可选)                   |

#### 操作步骤

1. 在主菜单选择 `4`
2. 输入客户端公钥 (PK)
3. 输入文件ID
4. 可选: 输入删除证明
5. 确认删除操作

#### 权限验证

删除操作会验证:

1. **PK格式**: 必须是有效的hex字符串
2. **文件存在**: 文件必须存在
3. **所有权**: PK必须与文件所有者匹配

#### 输出结果

**成功删除**:

```
🗑️  删除文件: test_file_001
   请求者PK: 1234567890abcdef...
   ✅ 身份验证通过
   标记 3 条索引为无效
✅ 文件删除成功 (已标记为无效)
```

**权限不足**:

```
❌ 权限不足: 您不是此文件的所有者
   文件所有者PK: abcdef1234567890...
```

#### 删除行为说明

* **软删除**: 文件被标记为 `state="invalid"`，但不会物理删除
* **索引更新**: 所有相关索引条目的 state 都会设为 "invalid"
* **搜索影响**: 被标记为 invalid 的文件不会出现在搜索结果中

---

### 5. 生成完整性证明

**功能**: 为指定文件生成完整性证明

#### 输入参数


| 参数      | 类型   | 必需 | 说明            |
| --------- | ------ | ---- | --------------- |
| `file_id` | string | ✅   | 文件ID          |
| `seed`    | string | ❌   | 随机种子 (可选) |

#### 操作步骤

1. 在主菜单选择 `5`
2. 输入文件ID
3. 可选: 输入种子
4. 查看生成的证明
5. 可选: 保存证明到文件

#### 输出结果

```
✅ 完整性证明生成成功!
   证明: a1b2c3d4e5f6789012345678901234567890abcdef...

是否保存证明到文件? (y/n):
```

#### 证明算法

```
proof = H1(file_id || file_auth_tag || seed)
```

其中 H1 是 SHA256 哈希函数。

---

### 6. 查看状态

**功能**: 显示节点的基本状态信息

#### 输出内容

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📊 存储节点状态
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
节点 ID:      node_1699123456
数据目录:     ./data
端口:         9000
文件数:       5
索引数:       15
密码学:       已初始化
版本:         v3.1 (支持PK身份验证)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

## JSON格式说明

### 插入文件参数格式 (insert.json)

```json
{
    "PK": "hex格式的客户端公钥",
    "ID_F": "文件唯一标识符",
    "ptr": "文件指针",
    "TS_F": "文件认证标签",
    "state": "valid",
    "keywords": [
        {
            "T_i": "搜索令牌",
            "kt_i": "关键词"
        }
    ],
    "metadata": {
        "任意键": "任意值"
    }
}
```

### 索引数据库格式 (index\_db.json)

```json
{
    "version": "3.1",
    "last_updated": "2024-11-09T10:30:00Z",
    "indices": {
        "search_token_1": [
            {
                "PK": "客户端公钥",
                "Ts": "状态令牌",
                "keyword": "关键词",
                "pointer": "文件指针",
                "file_identifier": "文件ID",
                "state": "valid"
            }
        ]
    },
    "total_entries": 15
}
```

### 文件元数据格式 (metadata/ {file\_id}.json)

```json
{
    "PK": "客户端公钥",
    "file_id": "test_file_001",
    "pointer": "encrypted_ptr_xyz",
    "file_auth_tag": "auth_tag_abc",
    "state": "valid",
    "insert_time": "2024-11-09T10:30:00Z",
    "keyword_count": 3,
    "file_size": 1024,
    "original": {
        "description": "原始元数据",
        "tags": ["标签"]
    }
}
```

---

## 文件结构

### 目录结构

```
./data/                          # 数据根目录
├── config.json                  # 节点配置文件
├── index_db.json               # 索引数据库
├── node_info.json              # 节点信息
├── files/                      # 加密文件存储
│   ├── file_001.enc
│   ├── file_002.enc
│   └── ...
└── metadata/                   # 文件元数据
    ├── file_001.json
    ├── file_002.json
    └── ...
```

### 配置文件 (config.json)

```json
{
    "version": "3.1",
    "node": {
        "node_id": "node_1699123456",
        "created_at": "2024-11-09T10:00:00Z",
        "description": "去中心化存储节点 (支持PK身份验证)"
    },
    "paths": {
        "data_dir": "./data",
        "files_dir": "./data/files",
        "metadata_dir": "./data/metadata",
        "index_db": "./data/index_db.json"
    },
    "server": {
        "port": 9000,
        "enable_server": false
    },
    "storage": {
        "max_file_size_mb": 100,
        "max_total_storage_gb": 10
    },
    "logging": {
        "enable_logging": true,
        "log_level": "INFO"
    },
    "security": {
        "enable_pk_verification": true
    }
}
```

---

## 常见问题

### Q1: PK格式要求是什么?

**A**: PK必须是十六进制(hex)字符串，只包含0-9和a-f字符。建议长度至少64字符。

示例:

```
✅ 正确: "1234567890abcdef1234567890abcdef"
❌ 错误: "123xyz" (包含非hex字符)
❌ 错误: "12 34" (包含空格)
```

### Q2: 删除文件后还能恢复吗?

**A**: 可以。v3.1采用软删除机制，文件被标记为 `state="invalid"` 但不会物理删除。如需恢复，可以手动修改索引数据库和元数据中的 state 字段。

### Q3: 为什么搜索不到我的文件?

**A**: 请检查以下几点:

1. **PK是否正确**: 搜索时输入的PK必须与插入时的PK完全匹配
2. **T\_i是否正确**: 搜索令牌必须是插入时使用的 T\_i 值
3. **文件状态**: 只有 state="valid" 的文件会出现在搜索结果中
4. **关键词是否存在**: 确认该文件确实包含该关键词

### Q4: 多个客户端可以插入同一个file\_id吗?

**A**: 技术上可以，但不建议。如果多个PK插入相同的file\_id，后插入的会覆盖前面的文件，但索引数据库会保留所有PK的索引条目。

### Q5: 如何批量导入文件?

**A**: 可以编写脚本循环调用插入接口。示例:

```bash
#!/bin/bash
for i in {1..10}; do
    # 准备 insert_$i.json 和 file_$i.enc
    echo "1" | ./storage_node  # 选择插入
    # ... 自动化输入
done
```

### Q6: 索引数据库损坏怎么办?

**A**: 备份策略:

1. 定期备份 `./data` 目录
2. 如果 index\_db.json 损坏，删除它并重启程序，会自动创建新的空数据库
3. 从备份恢复: `cp backup/index_db.json ./data/`

### Q7: state字段可以自定义吗?

**A**: 目前只支持 "valid" 和 "invalid" 两种状态。如需扩展，需要修改源代码中的验证逻辑。

### Q8: 如何迁移到新版本?

**A**: 从v3.0升级到v3.1:

1. 备份数据: `cp -r ./data ./data_backup`
2. 清空或删除旧的 index\_db.json (v3.0格式不兼容)
3. 重新插入所有文件，使用新的JSON格式

---

## 技术支持

如有问题，请检查:

1. 依赖库是否正确安装
2. 编译时是否有警告或错误
3. 数据目录是否有写权限
4. JSON文件格式是否正确

---

## 版权信息

去中心化存储节点 v3.1
© 2024 All Rights Reserved

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
