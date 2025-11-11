好的，我现在给出**存储端函数架构设计方案**，只说明函数的功能、输入和输出。

## 📐 **存储端函数架构设计（基于新结构体）**

---

### **一、初始化与配置模块**

#### 1.1 构造函数
```cpp
StorageNode(data_directory, encrypted_files_directory, port)
```
- **功能**：创建存储节点实例
- **输入**：
  - `data_directory`: 数据目录路径（默认 `"./data"`）
  - `encrypted_files_directory`: 加密文件存储目录（默认 `"./encrypted_files"`）
  - `port`: 服务器端口（默认 `9000`）
- **输出**：StorageNode 对象

#### 1.2 initialize_directories()
```cpp
bool initialize_directories()
```
- **功能**：初始化所有必需的目录（数据目录、加密文件目录、元数据目录）
- **输入**：无
- **输出**：`true` 成功，`false` 失败

#### 1.3 load_config()
```cpp
bool load_config()
```
- **功能**：从 `config.json` 加载配置（包括加密文件目录路径）
- **输入**：无
- **输出**：`true` 成功，`false` 失败

#### 1.4 save_config()
```cpp
bool save_config()
```
- **功能**：保存当前配置到 `config.json`
- **输入**：无
- **输出**：`true` 成功，`false` 失败

#### 1.5 create_default_config()
```cpp
bool create_default_config()
```
- **功能**：创建默认配置文件
- **输入**：无
- **输出**：`true` 成功，`false` 失败

#### 1.6 set_encrypted_files_directory()
```cpp
bool set_encrypted_files_directory(dir_path)
```
- **功能**：设置加密文件存储目录（用户可配置）
- **输入**：
  - `dir_path`: 新的加密文件存储目录路径
- **输出**：`true` 成功，`false` 失败

#### 1.7 get_encrypted_files_directory()
```cpp
string get_encrypted_files_directory()
```
- **功能**：获取当前加密文件存储目录路径
- **输入**：无
- **输出**：加密文件目录路径字符串

---

### **二、密码学模块**

#### 2.1 setup_cryptography()
```cpp
bool setup_cryptography(security_param, public_params_path)
```
- **功能**：初始化密码学参数（N, g, μ）
- **输入**：
  - `security_param`: 安全参数K（比特位数，如 512）
  - `public_params_path`: 公共参数保存路径（可选）
- **输出**：`true` 成功，`false` 失败

#### 2.2 load_public_params()
```cpp
bool load_public_params(filepath)
```
- **功能**：从文件加载公共参数并初始化密码学系统
- **输入**：
  - `filepath`: 公共参数文件路径
- **输出**：`true` 成功，`false` 失败

#### 2.3 save_public_params()
```cpp
bool save_public_params(filepath)
```
- **功能**：保存公共参数到文件
- **输入**：
  - `filepath`: 保存路径
- **输出**：`true` 成功，`false` 失败

#### 2.4 display_public_params()
```cpp
bool display_public_params(filepath)
```
- **功能**：显示公共参数（只读，不修改系统状态）
- **输入**：
  - `filepath`: 参数文件路径（可选，为空则显示内存中的参数）
- **输出**：`true` 成功，`false` 失败

#### 2.5 has_public_params_file()
```cpp
bool has_public_params_file(filepath)
```
- **功能**：检查公共参数文件是否存在
- **输入**：
  - `filepath`: 参数文件路径
- **输出**：`true` 存在，`false` 不存在

#### 2.6 is_crypto_initialized()
```cpp
bool is_crypto_initialized()
```
- **功能**：检查密码学系统是否已初始化
- **输入**：无
- **输出**：`true` 已初始化，`false` 未初始化

---

### **三、索引数据库模块**

#### 3.1 load_index_database()
```cpp
bool load_index_database()
```
- **功能**：从 `index_db.json` 加载所有文件的索引条目到内存
- **输入**：无
- **输出**：`true` 成功，`false` 失败
- **说明**：
  - 加载所有 IndexEntry（所有文件的所有关键词）
  - 使用 `ID_F` 作为 map 的 key

#### 3.2 save_index_database()
```cpp
bool save_index_database()
```
- **功能**：将内存中的索引数据库保存到 `index_db.json`
- **输入**：无
- **输出**：`true` 成功，`false` 失败
- **说明**：
  - 保存所有 IndexEntry 到统一的 JSON 文件
  - 格式：
  ```json
  {
    {
        "PK":"用户的公钥",
        "ID_F":"file_id",
        "state":"文件的存在状态",
        "TS_F":"文件认证标签集合",
        "file_path":"文件地址",
        {
            {
                "ptr":"当前的关键词的状态指针",
                "Ti_bar":"Ti_bar",
                "kt_wi":"kt_wi"
            }，
            {
                "ptr":"当前的关键词的状态指针",
                "Ti_bar":"Ti_bar",
                "kt_wi":"kt_wi"
            }，
        }
    }
  }
  ```
#### 3.3 get_index_count()
```cpp
size_t get_index_count()
```
- **功能**：获取索引条目总数
- **输入**：无
- **输出**：索引条目数量

---

### **四、文件存储数据库模块**

#### 4.4 get_file_count()
```cpp
size_t get_file_count()
```
- **功能**：获取文件总数
- **输入**：无
- **输出**：插入文件数量
---

### **五、文件操作模块**

#### 5.1 insert_file()
```cpp
bool insert_file(param_json_path, enc_file_path)
```
- **功能**：插入新文件
- **输入**：
  - `param_json_path`: 参数 JSON 文件路径
    ```json
    {
      "PK": "客户端公钥",
      "ID_F": "文件ID",
      "ptr": "文件指针",
      "TS_F": ["认证标签1", "认证标签2"],
      "state": "valid",
      "keywords": [
        {"Ti_bar": "Token1", "kt_i": "标签1"},
        {"Ti_bar": "Token2", "kt_i": "标签2"}
      ]
    }
    ```
  - `enc_file_path`: 加密文件路径
- **输出**：`true` 成功，`false` 失败
- **操作**：
  1. 将加密文件保存到 `encrypted_files_dir/<ID_F>.enc`
  2. 为每个关键词创建一个 IndexEntry，添加到 `index_database`。
  3. 创建 FileData，添加到 `file_storage`，供后面使用。
  4. 调用 `save_index_database()` 
  下面为一个文件的JSON文件示例
  ```json
  {
    {
        "PK":"用户的公钥",
        "ID_F":"file_id",
        "state":"文件的存在状态",
        "TS_F":"文件认证标签集合",
        "file_path":"文件地址",
        {
            {
                "ptr":"当前的关键词的状态指针",
                "Ti_bar":"Ti_bar",
                "kt_wi":"kt_wi"
            }，
            {
                "ptr":"当前的关键词的状态指针",
                "Ti_bar":"Ti_bar",
                "kt_wi":"kt_wi"
            }，
        }
    }
  }
  ```

#### 5.2 delete_file()
```cpp
bool delete_file(PK, file_id, del_proof)
```
- **功能**：删除文件（标记为无效）,并且重新计算索引数据库中的文件ID对应的所有文件kt_wi进行更新
函数：kt_wi = kt_wi/del
同时标文件状态为无效
- **输入**：
  - `PK`: 客户端公钥（用于身份验证）
  - `file_id`: 文件ID（ID_F）
  - `del_proof`: 删除证明
- **输出**：`true` 成功，`false` 失败
- **操作**：
  1. 验证 PK 是否与文件所有者匹配
  2. 将所有相关 IndexEntry 的 `state` 设为 `"invalid"`
  3. 更新新的kt_wi
  4. 调用 `save_index_database()` 

#### 5.3 search_keyword()
```cpp
SearchResult Search(PK, search_token, latest_state, seed)
```
暂时不设计

#### 5.4 retrieve_file()
```cpp
Json::Value retrieve_file(file_id)
```
- **功能**：检索文件的完整信息（包括密文内容）
- **输入**：
  - `file_id`: 文件ID（ID_F）
- **输出**：JSON 对象
  ```json
  {
    "success": true,
    "PK": "公钥",
    "ID_F": "文件ID",
    "C": "密文内容（从file_path读取）",
    "ptr": "指针",
    "state": "valid",
    "TS_F": ["标签1", "标签2"],
    "keywords": [
      {"Ti_bar": "Token1", "kt_wi": "标签1"},
      {"Ti_bar": "Token2", "kt_wi": "标签2"}
    ],
    "file_path": "加密文件路径",
    "file_size": 2048,
    "created_at": "时间",
    "updated_at": "时间"
  }
  ```
- **操作**：
  1. 从 `file_storage` 获取 FileData
  2. 从 `file_path` 读取密文内容
  3. 返回完整信息


#### 5.6 generate_integrity_proof()
```cpp
string generate_integrity_proof(file_id, seed)
```
- **功能**：生成文件完整性证明
- **输入**：
  - `file_id`: 文件ID（ID_F）
  - `seed`: 种子
- **输出**：完整性证明字符串
- **操作**：
  1. 从 FileData 获取第一个 TS_F 元素
  2. 计算 `H1(file_id || TS_F[0])`

---

### **六、文件列表查询模块**

#### 6.1 list_all_files()
```cpp
vector<string> list_all_files()
```
- **功能**：列出所有文件ID
- **输入**：无
- **输出**：文件ID列表 `vector<string>`

#### 6.2 list_files_by_pk()
```cpp
vector<string> list_files_by_pk(PK)
```
- **功能**：列出指定客户端的所有文件ID
- **输入**：
  - `PK`: 客户端公钥
- **输出**：文件ID列表 `vector<string>`

#### 6.3 list_valid_files()
```cpp
vector<string> list_valid_files()
```
- **功能**：列出所有状态为 "valid" 的文件ID
- **输入**：无
- **输出**：文件ID列表 `vector<string>`

#### 6.4 list_invalid_files()
```cpp
vector<string> list_invalid_files()
```
- **功能**：列出所有状态为 "invalid" 的文件ID
- **输入**：无
- **输出**：文件ID列表 `vector<string>`

---

### **七、元数据管理模块**

#### 7.1 get_file_metadata()
```cpp
Json::Value get_file_metadata(file_id)
```
- **功能**：获取文件元数据（从 `metadata/<ID_F>.json`）
- **输入**：
  - `file_id`: 文件ID（ID_F）
- **输出**：JSON 对象
  ```json
  {
    "success": true,
    "PK": "公钥",
    "ID_F": "文件ID",
    "file_size": 2048,
    "keyword_count": 2,
    "state": "valid",
    "file_path": "路径",
    "created_at": "时间",
    "updated_at": "时间"
  }
  ```

#### 7.2 export_file_metadata()
```cpp
bool export_file_metadata(file_id, output_path)
```
- **功能**：导出文件元数据到指定路径
- **输入**：
  - `file_id`: 文件ID（ID_F）
  - `output_path`: 导出路径
- **输出**：`true` 成功，`false` 失败

#### 7.3 update_file_metadata()
```cpp
bool update_file_metadata(file_id)
```
- **功能**：更新文件元数据文件（从 FileData 同步）
- **输入**：
  - `file_id`: 文件ID（ID_F）
- **输出**：`true` 成功，`false` 失败

---

### **八、加密文件管理模块**

#### 8.1 generate_file_storage_path()
```cpp
string generate_file_storage_path(file_id)
```
- **功能**：生成加密文件的存储路径
- **输入**：
  - `file_id`: 文件ID（ID_F）
- **输出**：完整路径字符串，格式：`encrypted_files_dir/<file_id>.enc`

#### 8.2 save_encrypted_file_to_storage()
```cpp
bool save_encrypted_file_to_storage(file_id, source_path)
```
- **功能**：将加密文件复制到存储目录
- **输入**：
  - `file_id`: 文件ID（ID_F）
  - `source_path`: 源文件路径
- **输出**：`true` 成功，`false` 失败
- **操作**：
  1. 生成目标路径：`encrypted_files_dir/<file_id>.enc`
  2. 复制文件内容

#### 8.3 load_encrypted_file_content()
```cpp
bool load_encrypted_file_content(file_id, ciphertext)
```
- **功能**：从存储目录读取加密文件内容
- **输入**：
  - `file_id`: 文件ID（ID_F）
  - `ciphertext`: 引用参数，用于返回密文内容
- **输出**：`true` 成功，`false` 失败
- **操作**：
  1. 从 FileData 获取 `file_path`
  2. 读取文件内容到 `ciphertext`

#### 8.4 delete_encrypted_file_from_storage()
```cpp
bool delete_encrypted_file_from_storage(file_id)
```
- **功能**：从存储目录物理删除加密文件
- **输入**：
  - `file_id`: 文件ID（ID_F）
- **输出**：`true` 成功，`false` 失败
- **说明**：用于彻底删除文件（非标记为无效）

#### 8.5 verify_encrypted_file_exists()
```cpp
bool verify_encrypted_file_exists(file_id)
```
- **功能**：验证加密文件是否在存储目录中存在
- **输入**：
  - `file_id`: 文件ID（ID_F）
- **输出**：`true` 存在，`false` 不存在

---

### **九、节点信息与统计模块**

#### 9.1 load_node_info()
```cpp
bool load_node_info()
```
- **功能**：从 `node_info.json` 加载节点信息
- **输入**：无
- **输出**：`true` 成功，`false` 失败

#### 9.2 save_node_info()
```cpp
bool save_node_info()
```
- **功能**：保存节点信息到 `node_info.json`
- **输入**：无
- **输出**：`true` 成功，`false` 失败

#### 9.3 update_statistics()
```cpp
void update_statistics(operation)
```
- **功能**：更新统计信息
- **输入**：
  - `operation`: 操作类型（"insert", "delete", "search"）
- **输出**：无

#### 9.4 get_statistics()
```cpp
Json::Value get_statistics()
```
- **功能**：获取节点统计信息
- **输入**：无
- **输出**：JSON 对象
  ```json
  {
    "total_files": 10,
    "valid_files": 8,
    "invalid_files": 2,
    "total_indices": 20,
    "storage_size_bytes": 1048576,
    "last_update": "时间"
  }
  ```
---