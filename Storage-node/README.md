# 去中心化存储节点系统 - 技术文档

## 📋 项目概述

**项目名称**: 去中心化存储节点系统 (Decentralized Storage Node System)  
**版本**: v3.4-fixed-final  
**语言**: C++11  
**主要功能**: 基于密码学的安全文件存储、可搜索加密索引、完整性验证

## 🏗️ 系统架构

```
┌─────────────────────────────────────────────┐
│           StorageNode Class                 │
├─────────────────────────────────────────────┤
│  密码学模块  │  索引管理  │  文件存储     │
├─────────────────────────────────────────────┤
│  - 配对加密  │  - 搜索    │  - 插入       │
│  - 哈希函数  │  - 索引    │  - 删除       │
│  - PRF函数   │  - 验证    │  - 检索       │
└─────────────────────────────────────────────┘
```

## 📊 核心数据结构

### 1. IndexKeywords 结构体

关键词索引信息，存储每个关键词的相关数据。

```cpp
struct IndexKeywords {
    std::string ptr_i;    // 指针，指向文件或下一个索引位置
    std::string kt_wi;    // 关键词标签（Keyword Tag）
    std::string Ti_bar;   // 状态令牌（State Token），用于搜索
};
```

**字段说明**：
- `ptr_i`: 指针字段，可以是文件ID或链式索引指针
- `kt_wi`: 关键词的加密标签，用于验证
- `Ti_bar`: 搜索令牌，作为索引数据库的键值

### 2. IndexEntry 结构体

文件索引条目，包含文件的完整元数据和关键词信息。

```cpp
struct IndexEntry {
    std::string ID_F;                      // 文件唯一标识符
    std::string PK;                        // 客户端公钥
    std::vector<std::string> TS_F;         // 文件认证标签数组
    std::string state;                     // 文件状态："valid" 或 "invalid"
    std::string file_path;                 // 文件在服务器上的存储路径
    std::vector<IndexKeywords> keywords;   // 关联的关键词列表
};
```

**字段说明**：
- `ID_F`: 文件的全局唯一标识符（通常为大整数）
- `PK`: 上传文件的客户端公钥，用于权限验证
- `TS_F`: 文件的多个认证标签，用于完整性验证
- `state`: 文件状态标志，支持软删除
- `file_path`: 加密文件的本地存储路径
- `keywords`: 文件的所有关键词索引信息

### 3. SearchResult 结构体

搜索结果，包含匹配的文件ID和验证信息。

```cpp
struct SearchResult {
    std::vector<std::string> ID_F;           // 匹配的文件ID列表
    std::vector<std::string> keyword_proofs; // 关键词证明列表
    std::string aggregated_proof;            // 聚合证明
};
```

**字段说明**：
- `ID_F`: 搜索到的所有匹配文件的ID
- `keyword_proofs`: 每个匹配文件的关键词证明
- `aggregated_proof`: 可选的聚合证明信息

---

## 🔐 密码学模块

### 1. setup_cryptography()

#### 函数签名
```cpp
bool setup_cryptography(int security_param, 
                       const std::string& public_params_path = "");
```

#### 功能描述
初始化密码学系统，生成公共参数 PP = {N, g, μ}。

#### 输入参数
- `security_param` (int): 安全参数 K，单位为比特（推荐 512）
- `public_params_path` (string): 公共参数保存路径（可选）

#### 输出
- **返回值** (bool): 成功返回 `true`，失败返回 `false`
- **副作用**: 
  - 初始化配对 `pairing`
  - 生成群元素 `g`, `μ`
  - 计算大整数 `N = p × q`
  - 如果提供路径，保存公共参数到文件

#### 计算公式

1. **配对初始化**：使用 Type A 配对
   ```
   pairing_init_set_buf(pairing, param_str, strlen(param_str))
   ```

2. **生成群元素**：
   ```
   g ← random(G₁)
   μ ← random(G₁)
   ```
   其中 G₁ 是配对中的第一个群

3. **计算 N**：
   ```
   p ← 群的阶
   q ← 群的阶
   N = p × q
   ```
   对于 Type A 配对，p = q = 群的阶

#### 使用示例
```cpp
StorageNode node("./data", 9000);
if (node.setup_cryptography(512, "./data/public_params.json")) {
    std::cout << "密码学系统初始化成功" << std::endl;
}
```

---

### 2. save_public_params()

#### 函数签名
```cpp
bool save_public_params(const std::string& filepath);
```

#### 功能描述
将公共参数序列化并保存到JSON文件。

#### 输入参数
- `filepath` (string): 保存公共参数的文件路径

#### 输出
- **返回值** (bool): 保存成功返回 `true`，失败返回 `false`
- **文件输出**: JSON格式的公共参数文件

#### 序列化格式

**JSON结构**：
```json
{
  "version": "2.0",
  "created_at": "2024-11-12T10:30:00Z",
  "description": "Public Parameters (N, g, μ)",
  "serialization_method": "element_to_bytes",
  "public_params": {
    "N": "大整数十进制字符串",
    "g": "群元素的hex编码",
    "g_length": 字节长度,
    "mu": "群元素的hex编码",
    "mu_length": 字节长度
  }
}
```

#### 序列化算法

1. **N 的序列化**：
   ```
   N_str = mpz_get_str(NULL, 10, N)
   ```
   将大整数转换为十进制字符串

2. **g 的序列化**：
   ```
   g_bytes = element_to_bytes(g)
   g_hex = bytes_to_hex(g_bytes)
   ```
   使用 PBC 库的 `element_to_bytes` 函数

3. **μ 的序列化**：
   ```
   mu_bytes = element_to_bytes(μ)
   mu_hex = bytes_to_hex(mu_bytes)
   ```

#### 使用示例
```cpp
if (node.save_public_params("./data/public_params.json")) {
    std::cout << "公共参数已保存" << std::endl;
}
```

---

### 3. load_public_params()

#### 函数签名
```cpp
bool load_public_params(const std::string& filepath);
```

#### 功能描述
从JSON文件加载公共参数并恢复密码学系统状态。

#### 输入参数
- `filepath` (string): 公共参数文件路径

#### 输出
- **返回值** (bool): 加载成功返回 `true`，失败返回 `false`
- **副作用**: 
  - 初始化配对系统
  - 恢复 N, g, μ 到内存
  - 设置 `crypto_initialized = true`

#### 反序列化算法

1. **读取JSON文件**：
   ```
   root = load_json_from_file(filepath)
   ```

2. **恢复 N**：
   ```
   N_str = root["public_params"]["N"]
   mpz_set_str(N, N_str.c_str(), 10)
   ```

3. **恢复 g**：
   ```
   g_hex = root["public_params"]["g"]
   g_bytes = hex_to_bytes(g_hex)
   element_from_bytes(g, g_bytes.data())
   ```

4. **恢复 μ**：
   ```
   mu_hex = root["public_params"]["mu"]
   mu_bytes = hex_to_bytes(mu_hex)
   element_from_bytes(μ, mu_bytes.data())
   ```

#### 使用示例
```cpp
if (node.load_public_params("./data/public_params.json")) {
    std::cout << "密码学系统已从文件恢复" << std::endl;
}
```

---

### 4. compute_hash_H1()

#### 函数签名
```cpp
std::string compute_hash_H1(const std::string& input);
```

#### 功能描述
计算SHA-256哈希并返回完整的十六进制字符串。

#### 输入参数
- `input` (string): 待哈希的输入数据

#### 输出
- **返回值** (string): 64字符的十六进制哈希值

#### 计算公式
```
H₁: {0,1}* → {0,1}²⁵⁶
H₁(input) = SHA256(input)
output = hex(SHA256(input))
```

#### 实现细节
```cpp
unsigned char hash[SHA256_DIGEST_LENGTH];  // 32 bytes
SHA256((unsigned char*)input.c_str(), input.length(), hash);
return bytes_to_hex(hash, SHA256_DIGEST_LENGTH);  // 64 hex chars
```

#### 使用示例
```cpp
std::string hash = compute_hash_H1("Hello World");
// hash = "a591a6d40bf420404a011733cfb7b190d62c65bf0bcda32b57b277d9ad9f146e"
```

---

### 5. compute_hash_H2()

#### 函数签名
```cpp
void compute_hash_H2(element_t result, const std::string& input);
```

#### 功能描述
将字符串哈希映射到群元素，用于密码学运算。

#### 输入参数
- `result` (element_t): 输出参数，存储结果群元素
- `input` (string): 输入字符串

#### 输出
- **副作用**: 修改 `result`，将其设置为哈希后的群元素

#### 计算公式
```
H₂: {0,1}* → G₁
H₂(input) = hash_to_point(SHA256(input))
```

#### 实现细节
```cpp
unsigned char hash[SHA256_DIGEST_LENGTH];
SHA256((unsigned char*)input.c_str(), input.length(), hash);
element_from_hash(result, hash, SHA256_DIGEST_LENGTH);
```

`element_from_hash` 将哈希值确定性地映射到群 G₁ 中的一个点。

#### 使用示例
```cpp
element_t h;
element_init_G1(h, pairing);
compute_hash_H2(h, "identifier");
// h 现在是 G₁ 中的一个元素
```

---

### 6. compute_hash_H3()

#### 函数签名
```cpp
std::string compute_hash_H3(const std::string& input);
```

#### 功能描述
计算SHA-256哈希并返回前16字节的十六进制表示。

#### 输入参数
- `input` (string): 待哈希的输入数据

#### 输出
- **返回值** (string): 32字符的十六进制字符串（16字节）

#### 计算公式
```
H₃: {0,1}* → {0,1}¹²⁸
H₃(input) = truncate(SHA256(input), 16)
output = hex(first_16_bytes(SHA256(input)))
```

#### 实现细节
```cpp
unsigned char hash[SHA256_DIGEST_LENGTH];  // 32 bytes
SHA256((unsigned char*)input.c_str(), input.length(), hash);
return bytes_to_hex(hash, 16);  // 只返回前16字节 = 32 hex chars
```

#### 使用示例
```cpp
std::string short_hash = compute_hash_H3("data");
// short_hash = "3a6eb0790f39ac87c94f3856b2dd2c5d" (32 chars)
```

---

### 7. compute_prf()

#### 函数签名
```cpp
void compute_prf(mpz_t result, const std::string& seed, 
                const std::string& input);
```

#### 功能描述
伪随机函数（PRF），基于种子和输入生成伪随机数。

#### 输入参数
- `result` (mpz_t): 输出参数，存储结果大整数
- `seed` (string): PRF的种子
- `input` (string): PRF的输入

#### 输出
- **副作用**: 修改 `result`，设置为 PRF 计算结果

#### 计算公式
```
PRF: {0,1}* × {0,1}* → ℤₙ
PRF(seed, input) = H₁(seed || input) mod N
```

其中：
- `||` 表示字符串连接
- `mod N` 将结果限制在 [0, N-1] 范围内

#### 实现细节
```cpp
std::string combined = seed + input;          // 连接
std::string hash_hex = compute_hash_H1(combined);  // 哈希
mpz_set_str(result, hash_hex.c_str(), 16);   // 转为大整数
mpz_mod(result, result, N);                  // 模 N
```

#### 使用示例
```cpp
mpz_t prf_value;
mpz_init(prf_value);
compute_prf(prf_value, "my_seed", "counter_1");
// prf_value 现在包含伪随机值
```

---

### 8. verify_pk_format()

#### 函数签名
```cpp
bool verify_pk_format(const std::string& pk);
```

#### 功能描述
验证公钥格式是否有效（十六进制字符串）。

#### 输入参数
- `pk` (string): 待验证的公钥字符串

#### 输出
- **返回值** (bool): 格式有效返回 `true`，否则 `false`

#### 验证规则
```
1. PK 非空
2. PK 只包含十六进制字符 (0-9, a-f, A-F)
```

#### 实现细节
```cpp
if (pk.empty()) return false;
for (char c : pk) {
    if (!isxdigit(c)) return false;  // 检查是否为十六进制字符
}
return true;
```

#### 使用示例
```cpp
if (verify_pk_format("abc123")) {
    std::cout << "PK格式有效" << std::endl;
}
```

---

## 📁 文件操作模块

### 1. insert_file()

#### 函数签名
```cpp
bool insert_file(const std::string& param_json_path, 
                const std::string& enc_file_path);
```

#### 功能描述
插入加密文件及其索引信息到存储系统。

#### 输入参数
- `param_json_path` (string): 包含文件元数据的JSON文件路径
- `enc_file_path` (string): 加密文件的路径

#### 输出
- **返回值** (bool): 插入成功返回 `true`，失败返回 `false`
- **副作用**:
  - 在 `index_database` 中创建索引条目
  - 在 `file_storage` 中存储文件信息
  - 复制加密文件到 `files_dir`
  - 创建元数据文件

#### JSON输入格式
```json
{
  "ID_F": "文件唯一标识符",
  "PK": "客户端公钥（hex）",
  "TS_F": ["标签1", "标签2", ...],
  "state": "valid",
  "keywords": [
    {
      "Ti_bar": "状态令牌（hex）",
      "kt_wi": "关键词标签（hex）",
      "ptr_i": "指针（可选）"
    }
  ]
}
```

#### 处理流程

```
1. 验证输入
   ├─ 检查密码学系统是否初始化
   ├─ 验证JSON文件是否存在
   ├─ 验证必需字段
   └─ 验证PK格式

2. 加载数据
   ├─ 读取JSON参数
   ├─ 读取加密文件内容
   └─ 解析关键词数组

3. 创建索引
   ├─ 创建 IndexEntry 对象
   ├─ 填充基本信息（ID_F, PK, state）
   ├─ 添加认证标签（TS_F）
   └─ 处理每个关键词
       ├─ 提取 Ti_bar, kt_wi, ptr_i
       ├─ 创建 IndexKeywords 对象
       └─ 添加到 index_database[Ti_bar]

4. 存储文件
   ├─ 保存到 file_storage[ID_F]
   ├─ 复制加密文件到 files_dir
   ├─ 创建元数据文件
   └─ 持久化索引数据库
```

#### 关键算法

**1. 指针处理**：
```cpp
// 如果关键词提供了 ptr_i，使用它；否则使用 ID_F
std::string ptr_i = ID_F;
if (kw.isMember("ptr_i")) {
    ptr_i = kw["ptr_i"].asString();
}
```

**2. 索引创建**：
```cpp
for (each keyword kw in keywords) {
    IndexKeywords idx_kw;
    idx_kw.ptr_i = ptr_i;
    idx_kw.kt_wi = kw["kt_wi"];
    idx_kw.Ti_bar = kw["Ti_bar"];
    
    entry.keywords.push_back(idx_kw);
    index_database[Ti_bar].push_back(entry);  // 按 Ti_bar 索引
}
```

#### 使用示例
```cpp
bool success = node.insert_file(
    "./params/file_001.json",
    "./encrypted/file_001.enc"
);
if (success) {
    std::cout << "文件插入成功" << std::endl;
}
```

---

### 2. delete_file()

#### 函数签名
```cpp
bool delete_file(const std::string& PK, 
                const std::string& file_id, 
                const std::string& del_proof);
```

#### 功能描述
软删除文件，将文件状态标记为 "invalid"，但保留数据。

#### 输入参数
- `PK` (string): 请求删除的客户端公钥
- `file_id` (string): 要删除的文件ID
- `del_proof` (string): 删除证明（当前版本未使用）

#### 输出
- **返回值** (bool): 删除成功返回 `true`，失败返回 `false`
- **副作用**:
  - 将文件状态设为 "invalid"
  - 更新所有相关索引条目的状态

#### 处理流程

```
1. 身份验证
   ├─ 验证PK格式
   ├─ 检查文件是否存在
   └─ 验证请求者是否是文件所有者
       └─ if (file_storage[file_id].PK != PK) → 拒绝

2. 标记删除
   ├─ 在 index_database 中查找所有相关条目
   ├─ 将匹配的条目状态设为 "invalid"
   └─ 更新 file_storage[file_id].state = "invalid"

3. 持久化
   └─ 保存更新后的索引数据库
```

#### 权限验证算法
```cpp
// 只有文件所有者才能删除
if (file_storage[file_id].PK != PK) {
    return false;  // 权限不足
}
```

#### 软删除实现
```cpp
// 遍历索引数据库
for (auto& [token, entries] : index_database) {
    for (auto& entry : entries) {
        if (entry.ID_F == file_id && entry.PK == PK) {
            entry.state = "invalid";  // 标记为无效
        }
    }
}
```

#### 使用示例
```cpp
bool deleted = node.delete_file(
    "0dd8e9f10350afcf...",  // 客户端PK
    "58596621420790...",     // 文件ID
    ""                       // 删除证明（可选）
);
```

---

### 3. search_keyword()

#### 函数签名
```cpp
SearchResult search_keyword(const std::string& PK,
                           const std::string& search_token, 
                           const std::string& latest_state,
                           const std::string& seed);
```

#### 功能描述
使用搜索令牌查找包含特定关键词的文件。

#### 输入参数
- `PK` (string): 请求者的公钥（用于权限过滤）
- `search_token` (string): 搜索令牌（Ti_bar值）
- `latest_state` (string): 最新状态（当前版本未使用）
- `seed` (string): 随机种子（当前版本未使用）

#### 输出
- **返回值** (SearchResult): 包含匹配文件的搜索结果
  - `ID_F`: 匹配的文件ID列表
  - `keyword_proofs`: 对应的关键词证明列表

#### 搜索算法

```
1. 输入验证
   └─ 验证PK格式

2. 索引查找
   ├─ 在 index_database 中查找 search_token
   └─ it = index_database.find(search_token)

3. 结果过滤
   └─ for each entry in index_database[search_token]:
       ├─ if entry.PK == PK AND entry.state == "valid":
       │   ├─ result.ID_F.push_back(entry.ID_F)
       │   └─ result.keyword_proofs.push_back(entry.keywords[0].kt_wi)
       └─ 只返回属于请求者且状态有效的文件

4. 返回结果
   └─ 返回 SearchResult 对象
```

#### 权限过滤
```cpp
// 只返回属于请求者且状态为有效的文件
if (entry.PK == PK && entry.state == "valid") {
    result.ID_F.push_back(entry.ID_F);
    if (!entry.keywords.empty()) {
        result.keyword_proofs.push_back(entry.keywords[0].kt_wi);
    }
}
```

#### 时间复杂度
- **索引查找**: O(1) - 哈希表查找
- **结果过滤**: O(n) - n 为该令牌对应的条目数

#### 使用示例
```cpp
SearchResult result = node.search_keyword(
    "932fec9942585339...",              // 客户端PK
    "a4d04362f4992349e6b3080d...",     // 搜索令牌（Ti_bar）
    "",                                 // 最新状态
    ""                                  // 种子
);

std::cout << "找到 " << result.ID_F.size() << " 个文件" << std::endl;
for (const auto& file_id : result.ID_F) {
    std::cout << "文件ID: " << file_id << std::endl;
}
```

---

### 4. generate_integrity_proof()

#### 函数签名
```cpp
std::string generate_integrity_proof(const std::string& file_id, 
                                     const std::string& seed);
```

#### 功能描述
生成文件完整性证明（当前版本为占位函数）。

#### 输入参数
- `file_id` (string): 文件唯一标识符
- `seed` (string): 随机种子

#### 输出
- **返回值** (string): 完整性证明字符串（当前返回空字符串）

#### 理论设计

如果实现，应包含以下步骤：

```
1. 验证文件存在
   └─ 检查 file_storage[file_id]

2. 计算证明
   ├─ 获取文件的 TS_F 标签
   ├─ 结合随机种子
   └─ 生成聚合签名或哈希证明

3. 公式（理论）
   Proof = H(file_id || PK || seed || TS_F[0] || ... || TS_F[n])
```

#### 使用示例
```cpp
std::string proof = node.generate_integrity_proof(
    "58596621420790...",  // 文件ID
    "random_seed_123"     // 随机种子
);
```

---

### 5. retrieve_file()

#### 函数签名
```cpp
Json::Value retrieve_file(const std::string& file_id);
```

#### 功能描述
检索文件的完整信息（当前版本为占位函数）。

#### 输入参数
- `file_id` (string): 文件唯一标识符

#### 输出
- **返回值** (Json::Value): JSON格式的文件信息

#### 理论返回格式
```json
{
  "success": true,
  "file_id": "文件ID",
  "PK": "公钥",
  "ciphertext": "加密内容",
  "state": "valid",
  "pointer": "指针",
  "file_auth_tag": "认证标签"
}
```

#### 使用示例
```cpp
Json::Value file_info = node.retrieve_file("58596621420790...");
if (file_info["success"].asBool()) {
    std::cout << "文件检索成功" << std::endl;
}
```

---

## 💾 数据库操作模块

### 1. load_index_database()

#### 函数签名
```cpp
bool load_index_database();
```

#### 功能描述
从JSON文件加载索引数据库到内存。

#### 输入参数
- 无（使用 `data_dir + "/index_db.json"`）

#### 输出
- **返回值** (bool): 加载成功返回 `true`
- **副作用**:
  - 填充 `index_database` 映射
  - 填充 `file_storage` 映射

#### 加载流程

```
1. 检查文件存在性
   └─ if not exists → 创建新数据库

2. 解析JSON
   ├─ 读取 "indices" 对象
   └─ 遍历每个令牌及其条目列表

3. 反序列化数据
   for each token in indices:
       for each entry_json in indices[token]:
           ├─ 创建 IndexEntry 对象
           ├─ 解析基本字段（ID_F, PK, state, file_path）
           ├─ 解析 TS_F 数组
           ├─ 解析 keywords 数组
           │   └─ 创建 IndexKeywords 对象
           ├─ 添加到 index_database[token]
           └─ 添加到 file_storage[ID_F]

4. 验证
   └─ 统计加载的条目数
```

#### JSON格式
```json
{
  "version": "3.4",
  "last_update": "2024-11-12T10:30:00Z",
  "indices": {
    "search_token_1": [
      {
        "ID_F": "文件ID",
        "PK": "公钥",
        "state": "valid",
        "file_path": "路径",
        "TS_F": ["标签1", "标签2"],
        "keywords": [
          {
            "ptr_i": "指针",
            "kt_wi": "关键词标签",
            "Ti_bar": "状态令牌"
          }
        ]
      }
    ]
  }
}
```

#### 使用示例
```cpp
if (node.load_index_database()) {
    std::cout << "索引数据库加载成功" << std::endl;
}
```

---

### 2. save_index_database()

#### 函数签名
```cpp
bool save_index_database();
```

#### 功能描述
将内存中的索引数据库序列化保存到JSON文件。

#### 输入参数
- 无（使用内存中的 `index_database`）

#### 输出
- **返回值** (bool): 保存成功返回 `true`
- **文件输出**: `data_dir + "/index_db.json"`

#### 保存流程

```
1. 创建JSON根对象
   ├─ version
   ├─ last_update
   └─ indices (空对象)

2. 遍历索引数据库
   for each (token, entries) in index_database:
       ├─ 创建条目数组
       └─ for each entry in entries:
           ├─ 序列化 ID_F, PK, state, file_path
           ├─ 序列化 TS_F 数组
           ├─ 序列化 keywords 数组
           └─ 添加到条目数组

3. 写入文件
   └─ 保存JSON到文件
```

#### 序列化示例
```cpp
// 序列化一个索引条目
Json::Value entry_json;
entry_json["ID_F"] = entry.ID_F;
entry_json["PK"] = entry.PK;
entry_json["state"] = entry.state;

// 序列化 TS_F 数组
Json::Value ts_f_array(Json::arrayValue);
for (const auto& tag : entry.TS_F) {
    ts_f_array.append(tag);
}
entry_json["TS_F"] = ts_f_array;

// 序列化 keywords 数组
Json::Value keywords_array(Json::arrayValue);
for (const auto& kw : entry.keywords) {
    Json::Value kw_json;
    kw_json["ptr_i"] = kw.ptr_i;
    kw_json["kt_wi"] = kw.kt_wi;
    kw_json["Ti_bar"] = kw.Ti_bar;
    keywords_array.append(kw_json);
}
entry_json["keywords"] = keywords_array;
```

#### 使用示例
```cpp
if (node.save_index_database()) {
    std::cout << "索引数据库保存成功" << std::endl;
}
```

---

## 🔧 辅助函数模块

### 1. bytes_to_hex()

#### 函数签名
```cpp
std::string bytes_to_hex(const unsigned char* data, size_t len);
```

#### 功能描述
将字节数组转换为十六进制字符串。

#### 输入参数
- `data` (unsigned char*): 字节数组指针
- `len` (size_t): 字节数组长度

#### 输出
- **返回值** (string): 十六进制字符串（长度为 `2 * len`）

#### 转换算法
```
for i = 0 to len-1:
    byte = data[i]
    hex_string += sprintf("%02x", byte)
```

每个字节转换为2个十六进制字符。

#### 使用示例
```cpp
unsigned char data[] = {0xDE, 0xAD, 0xBE, 0xEF};
std::string hex = bytes_to_hex(data, 4);
// hex = "deadbeef"
```

---

### 2. hex_to_bytes()

#### 函数签名
```cpp
std::vector<unsigned char> hex_to_bytes(const std::string& hex);
```

#### 功能描述
将十六进制字符串转换为字节数组。

#### 输入参数
- `hex` (string): 十六进制字符串（长度必须为偶数）

#### 输出
- **返回值** (vector<unsigned char>): 字节数组

#### 转换算法
```
for i = 0 to hex.length() step 2:
    byte_str = hex.substr(i, 2)
    byte = strtol(byte_str, 16)
    bytes.push_back(byte)
```

每2个十六进制字符转换为1个字节。

#### 使用示例
```cpp
std::vector<unsigned char> bytes = hex_to_bytes("deadbeef");
// bytes = {0xDE, 0xAD, 0xBE, 0xEF}
```

---

## 📈 性能特性

### 时间复杂度

| 操作 | 平均复杂度 | 最坏复杂度 | 说明 |
|------|-----------|-----------|------|
| insert_file | O(k) | O(k) | k = 关键词数量 |
| delete_file | O(n) | O(n) | n = 索引条目总数 |
| search_keyword | O(m) | O(m) | m = 该令牌的匹配数 |
| load_index_database | O(n) | O(n) | n = 总条目数 |
| save_index_database | O(n) | O(n) | n = 总条目数 |

### 空间复杂度

| 数据结构 | 空间复杂度 | 说明 |
|---------|-----------|------|
| index_database | O(n×k) | n=文件数, k=平均关键词数 |
| file_storage | O(n) | n=文件数 |
| 公共参数 | O(1) | 固定大小 |

---

## 🔒 安全特性

### 1. 权限控制
```
- 文件插入：需要有效的PK
- 文件删除：只有所有者（PK匹配）可以删除
- 文件搜索：只返回请求者自己的文件
```

### 2. 数据完整性
```
- 使用 TS_F 认证标签验证文件完整性
- 支持多个认证标签冗余
- 软删除机制保证可追溯性
```

### 3. 隐私保护
```
- 关键词通过 Ti_bar 加密令牌搜索
- 不暴露原始关键词内容
- 索引信息与加密文件分离存储
```

---

## 📝 使用示例

### 完整工作流程

```cpp
#include "storage_node.h"

int main() {
    // 1. 创建存储节点
    StorageNode node("./data", 9000);
    
    // 2. 初始化
    node.initialize_directories();
    node.load_config();
    
    // 3. 初始化密码学系统
    if (!node.setup_cryptography(512, "./data/public_params.json")) {
        std::cerr << "密码学初始化失败" << std::endl;
        return 1;
    }
    
    // 4. 加载索引数据库
    node.load_index_database();
    
    // 5. 插入文件
    if (node.insert_file("./params/file1.json", "./encrypted/file1.enc")) {
        std::cout << "文件插入成功" << std::endl;
    }
    
    // 6. 搜索文件
    SearchResult result = node.search_keyword(
        "932fec9942585339...",           // PK
        "a4d04362f4992349e6b3080d...",  // 搜索令牌
        "",                              // 状态
        ""                               // 种子
    );
    
    std::cout << "找到 " << result.ID_F.size() << " 个文件" << std::endl;
    
    // 7. 保存数据
    node.save_index_database();
    node.save_node_info();
    
    return 0;
}
```

---

## 🛠️ 编译与运行

### 依赖库
```bash
# Ubuntu/Debian
sudo apt-get install libpbc-dev libgmp-dev libssl-dev libjsoncpp-dev

# 编译
g++ -std=c++11 main.cpp storage_node.cpp -o storage_node \
    -lpbc -lgmp -lcrypto -ljsoncpp -lpthread

# 运行
./storage_node [数据目录] [端口]
```

---

## 📚 参考资料

### 密码学基础
- **配对密码学**: 使用 PBC (Pairing-Based Cryptography) 库
- **哈希函数**: SHA-256
- **群运算**: Type A 配对，G₁ = G₂

### 数据结构
- **索引数据库**: 基于哈希表的倒排索引
- **文件存储**: 键值对映射

### JSON格式
- 版本: 3.4 (2024标准)
- 编码: UTF-8
- 序列化: jsoncpp 库

---

## 📞 技术支持

如有问题，请检查：
1. 所有依赖库是否正确安装
2. JSON文件格式是否符合规范
3. 文件路径和权限是否正确
4. 密码学系统是否已初始化

---

**文档版本**: 1.0  
**最后更新**: 2024-11-12  
**作者**: 去中心化存储项目团队


$\prod_{d|40}d$