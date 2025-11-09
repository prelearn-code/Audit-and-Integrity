# 项目文件说明 - v3.2

## 📦 文件清单

本次更新提供以下文件：

### 核心源代码（3个文件）

1. **storage\_node.h** - 头文件
   * 修改了 `setup_cryptography()` 函数签名
   * 新增了 `save_public_params()` 函数声明
   * 新增了 `load_public_params()` 函数声明
   * 更新版本号为 v3.2
2. **storage\_node.cpp** - 实现文件
   * 修改了 `setup_cryptography()` 实现，接受安全参数K
   * 新增了 `save_public_params()` 函数实现（约70行）
   * 新增了 `load_public_params()` 函数实现（约50行）
   * 更新配置文件版本到 3.2
3. **main.cpp** - 主程序
   * 新增安全参数K输入逻辑（步骤3/6）
   * 修改 `setup_cryptography()` 调用，传入安全参数和保存路径
   * 新增菜单选项10：查看公共参数
   * 新增 `handle_view_public_params()` 函数
   * 更新初始化流程从5步到6步

### 文档文件（3个文件）

4. **README.md** - 完整使用手册
   * 版本更新说明
   * 系统要求和依赖安装
   * 编译和运行指南
   * 公共参数功能详解
   * 所有菜单功能说明
   * API文档和数据结构
   * 常见问题解答
5. **QUICKSTART.md** - 快速入门指南
   * 10分钟快速上手教程
   * 基本操作示例
   * 重要文件位置说明
   * 公共参数使用方法
   * 常见问题和最佳实践
6. **CHANGELOG.md** - 版本更新日志
   * v3.2 详细更新内容
   * v3.1 和 v3.0 历史记录
   * 升级指南
   * 版本规则说明
   * 未来路线图

### 构建和示例文件（4个文件）

7. **Makefile** - 编译脚本
   * 简化编译过程
   * 依赖检查和安装
   * 清理和运行命令
   * 示例文件生成
   * 帮助信息
8. **sample\_params.json** - 参数文件示例
   * 展示插入文件所需的JSON格式
   * 包含PK、ID\_F、ptr、TS\_F、state和keywords字段
   * 可作为模板使用
9. **public\_params\_example.json** - 公共参数示例
   * 展示公共参数的JSON格式
   * 包含p、q、G\_1、G\_2、e的完整描述
   * 帮助理解公共参数结构
10. **FILES.md** - 本文件
    * 所有文件的详细说明
    * 文件关系和依赖
    * 使用建议

---

## 📋 代码修改总结

### storage\_node.h 的修改

**修改位置**：第108-126行

**修改内容**：

```cpp
// 旧版本（v3.1）
bool setup_cryptography();

// 新版本（v3.2）
bool setup_cryptography(int security_param, 
                       const std::string& public_params_path = "");

// 新增函数声明
bool save_public_params(const std::string& filepath);
bool load_public_params(const std::string& filepath);
```

### storage\_node.cpp 的修改

**修改1**：setup\_cryptography() 实现（第35-88行）

* 新增 `security_param` 参数接收
* 新增公共参数保存逻辑
* 输出安全参数信息

**新增2**：save\_public\_params() 函数（第90-150行）

* 创建JSON对象
* 序列化群参数 p, q
* 序列化生成元 g
* 描述 G\_1, G\_2, e
* 保存到文件

**新增3**：load\_public\_params() 函数（第152-195行）

* 读取JSON文件
* 解析公共参数
* 格式化输出显示

### main.cpp 的修改

**修改1**：print\_banner() 更新（第20-23行）

* 版本号更新为 v3.2
* 说明新增公共参数功能

**修改2**：print\_menu() 更新（第26-42行）

* 新增选项10：查看公共参数
* 更新选项范围说明 [0-10]

**新增3**：handle\_view\_public\_params() 函数（第336-342行）

* 读取公共参数文件
* 调用 load\_public\_params()
* 错误处理

**修改4**：初始化流程（第362-400行）

* 步骤3：新增安全参数输入
* 步骤4：调用 setup\_cryptography() 时传入参数
* 更新步骤编号和说明

**修改5**：主循环（第418-456行）

* 新增 case 10 处理查看公共参数

---

## 🔗 文件关系图

```
┌─────────────────────────────────────────────┐
│              用户交互层                      │
│  ┌─────────────────────────────────────┐   │
│  │         main.cpp                    │   │
│  │  - 控制台菜单                        │   │
│  │  - 用户输入处理                      │   │
│  │  - 初始化流程（含安全参数输入）       │   │
│  └─────────────────────────────────────┘   │
└─────────────────┬───────────────────────────┘
                  │ 调用
                  ↓
┌─────────────────────────────────────────────┐
│             核心逻辑层                       │
│  ┌─────────────────────────────────────┐   │
│  │      storage_node.h/.cpp            │   │
│  │  - StorageNode 类                   │   │
│  │  - 密码学函数                        │   │
│  │  - 文件操作                          │   │
│  │  - 公共参数管理 (v3.2新增)          │   │
│  └─────────────────────────────────────┘   │
└─────────────────┬───────────────────────────┘
                  │ 生成/读取
                  ↓
┌─────────────────────────────────────────────┐
│              数据层                          │
│  ./data/                                    │
│  ├── public_params.json (v3.2新增)          │
│  ├── config.json                            │
│  ├── index_db.json                          │
│  ├── node_info.json                         │
│  ├── files/*.enc                            │
│  └── metadata/*.json                        │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│            构建和文档层                      │
│  - Makefile (编译脚本)                      │
│  - README.md (完整文档)                     │
│  - QUICKSTART.md (快速入门)                 │
│  - CHANGELOG.md (更新日志)                  │
│  - sample_params.json (示例)                │
│  - public_params_example.json (示例)        │
└─────────────────────────────────────────────┘
```

---

## 🚀 使用建议

### 第一次使用

1. **阅读文档顺序**：
   * `QUICKSTART.md` → 快速了解基本操作
   * `README.md` → 深入了解所有功能
   * `CHANGELOG.md` → 了解版本更新内容
2. **编译和运行**：
   ```bash
   # 使用 Makefile
   make              # 编译
   make run          # 运行

   # 或手动编译
   g++ -std=c++11 main.cpp storage_node.cpp -o storage_node \
       -lssl -lcrypto -lpbc -lgmp -ljsoncpp -lpthread
   ./storage_node
   ```
3. **首次启动**：
   * 输入安全参数K（建议512，直接回车即可）
   * 系统自动生成公共参数
   * 查看菜单选项10验证公共参数

### 从v3.1升级

1. **备份数据**：
   ```bash
   cp -r ./data ./data_backup
   ```
2. **替换文件**：
   * 替换 storage\_node.h
   * 替换 storage\_node.cpp
   * 替换 main.cpp
3. **重新编译**：
   ```bash
   make clean
   make
   ```
4. **首次运行**：
   * 输入安全参数K
   * 验证版本为v3.2
   * 查看公共参数

### 开发和调试

1. **调试编译**：
   ```bash
   make debug
   ```
2. **检查依赖**：
   ```bash
   make check-deps
   ```
3. **生成示例文件**：
   ```bash
   make example
   ```

---

## 📊 代码统计

### 修改统计


| 文件              | 修改类型    | 行数变化 | 说明                         |
| ----------------- | ----------- | -------- | ---------------------------- |
| storage\_node.h   | 修改 + 新增 | +20      | 函数签名修改 + 2个新函数声明 |
| storage\_node.cpp | 修改 + 新增 | +140     | 函数实现修改 + 2个新函数实现 |
| main.cpp          | 修改 + 新增 | +45      | 初始化流程修改 + 新菜单项    |
| **总计**          | -           | **+205** | 核心代码增加约205行          |

### 文档统计


| 文件          | 类型     | 行数       | 说明             |
| ------------- | -------- | ---------- | ---------------- |
| README.md     | 完整文档 | \~850      | 使用手册         |
| QUICKSTART.md | 快速指南 | \~400      | 入门教程         |
| CHANGELOG.md  | 更新日志 | \~300      | 版本历史         |
| FILES.md      | 说明文档 | \~250      | 本文件           |
| **总计**      | -        | **\~1800** | 文档总计约1800行 |

---

## ✅ 质量保证

### 代码质量

* ✅ 遵循C++11标准
* ✅ 完整的错误处理
* ✅ 详细的注释说明
* ✅ 一致的代码风格
* ✅ 内存安全管理

### 功能完整性

* ✅ 所有v3.1功能保持不变
* ✅ 新增功能完全实现
* ✅ 向后兼容现有数据
* ✅ 完整的输入验证
* ✅ 友好的错误提示

### 文档完整性

* ✅ API完整文档
* ✅ 使用示例
* ✅ 常见问题解答
* ✅ 升级指南
* ✅ 版本更新日志

---

## 🔧 技术细节

### 公共参数格式

公共参数使用JSON格式存储，包含以下字段：

```json
{
    "version": "1.0",
    "created_at": "时间戳",
    "description": "描述",
    "public_params": {
        "p": "群阶（大质数）",
        "q": "群阶（大质数）",
        "G_1": {
            "type": "G1 (Elliptic Curve Group)",
            "order": "群的阶",
            "description": "描述",
            "generator_g_hex": "生成元g的十六进制表示"
        },
        "G_2": {
            "type": "G_T (Target Group)",
            "order": "群的阶",
            "description": "描述"
        },
        "e": {
            "type": "Bilinear Pairing",
            "mapping": "e: G_1 × G_1 → G_2",
            "properties": ["bilinearity", "non-degeneracy", "computability"],
            "pairing_type": "type_a (symmetric)"
        }
    }
}
```

### 安全参数范围

* **最小值**: 128 bits
* **推荐值**: 512 bits（标准安全级别）
* **高安全**: 1024 bits
* **最大值**: 2048 bits

### 依赖库版本

* OpenSSL: 1.1.0 或更高
* PBC: 0.5.14 或更高
* GMP: 6.0 或更高
* JsonCpp: 1.7.0 或更高

---

## 📞 支持

### 获取帮助

1. **查看文档**：
   * README.md - 完整功能说明
   * QUICKSTART.md - 快速入门
   * CHANGELOG.md - 版本历史
2. **常见问题**：
   * 参考 README.md 的"常见问题"章节
   * 参考 QUICKSTART.md 的"常见问题"章节
3. **提交问题**：
   * 在项目仓库提交 Issue
   * 提供详细的错误信息和复现步骤

### 贡献代码

欢迎提交 Pull Request！请确保：

* 代码风格一致
* 添加必要的注释
* 更新相关文档
* 通过编译测试

---

## 📄 许可证

本项目采用 MIT 许可证。

---

**文件版本**: 1.0
**最后更新**: 2024-11-09
**项目版本**: v3.2.0
