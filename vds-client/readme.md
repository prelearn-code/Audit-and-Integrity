# 本地加密存储工具 v3.1

> 基于可搜索加密的本地文件加密工具，支持前向安全性和可验证性  
> **新版本特性：支持JSON配置文件导入系统参数**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-14-blue.svg)](https://isocpp.org/)
[![Version](https://img.shields.io/badge/version-3.1-green.svg)](https://github.com/your-repo)

---

## 📖 简介

本工具是一个**纯本地运行**的文件加密工具，使用军事级加密算法（AES-256-CBC）和可搜索加密技术，让您可以：

- 🔐 加密文件并生成元数据
- 🔍 为关键词生成搜索令牌（前向安全）
- 🔓 解密文件
- 📋 生成认证标签用于完整性验证

**核心特性：**
- ✅ 完全本地运行，无需网络
- ✅ 不依赖区块链或外部服务
- ✅ 军事级加密（AES-256）
- ✅ 可搜索加密（无需解密即可搜索）
- ✅ 前向安全性（新文件不泄露旧搜索模式）
- ✨ **新功能：JSON配置文件管理系统参数**

---

## 🆕 v3.1 版本更新

### 主要变更
- ✨ **配置文件导入**：系统参数不再硬编码，改用 `system_params.json` 配置文件
- 🔧 **灵活配置**：支持不同安全级别参数切换，无需重新编译
- 📝 **参数管理**：系统参数可追踪、版本控制
- 🛡️ **安全性提升**：方便更换配对参数提升安全等级

### 配置文件结构
```json
{
  "pairing_type": "a",
  "parameters": {
    "q": "大素数",
    "h": "子群阶",
    "r": "阶参数",
    "exp2": "指数参数2",
    "exp1": "指数参数1",
    "sign1": "符号1",
    "sign0": "符号0"
  },
  "system_values": {
    "N": "系统模数"
  },
  "security_level": "512-bit"
}
```

---

## 🔧 安装依赖

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake
sudo apt-get install -y libssl-dev libgmp-dev libjsoncpp-dev libpbc-dev
```

### CentOS/RHEL
```bash
sudo yum install -y gcc-c++ cmake
sudo yum install -y openssl-devel gmp-devel jsoncpp-devel pbc-devel
```

---

## 🏗️ 编译

```bash
# 创建构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译
make -j4

# 查看可执行文件
ls -lh storage_client
```

---

## 🚀 快速开始

### 准备工作

**重要：在运行前，请确保 `system_params.json` 文件在可执行文件同目录下！**

```bash
# 检查配置文件
ls -l system_params.json

# 如果不存在，请从项目目录复制
cp system_params.json /path/to/build/
```

### 第一步：初始化系统

```bash
./storage_client
```

程序会自动检测配置文件：

```
==================================================
  🔐 本地加密存储工具 - v3.1
  可验证的可搜索加密系统（支持JSON配置）
==================================================

✅ 检测到系统参数配置文件

=========================================
  本地加密存储工具 v3.1
=========================================
...
```

运行初始化命令：

```
💻 > init

⚙️  初始化加密系统...
📄 使用配置文件: system_params.json
💡 提示: 请确保配置文件在程序同目录下

初始化客户端...
从配置文件加载系统参数...
✅ 系统参数加载成功
   配对类型: a
   安全级别: 512-bit
✅ 客户端初始化成功
```

### 第二步：生成密钥

```
💻 > keygen

🔑 生成密钥...
生成客户端密钥...
✅ 密钥生成成功
📌 公钥: 04a1b2c3d4e5f6789abcdef0123...
```

### 第三步：保存密钥

```
💻 > save-keys

💾 输入密钥文件路径: my_keys.dat
✅ 密钥已保存到: my_keys.dat
```

**⚠️ 重要：请妥善保管密钥文件！丢失密钥将无法解密文件！**

---

## 📚 完整使用教程

### 1. 加密文件

```
💻 > encrypt

📄 输入文件路径: documents/report.pdf
🏷️  输入关键词（逗号分隔）: confidential, finance, Q4-2024
💾 输出文件前缀（将生成 .enc 和 .json）: encrypted_report

🔐 加密中...
加密文件: documents/report.pdf
文件大小: 524288 字节
文件ID: a3f5e8d2b1c7a4f9e2d8c3b6a1f5e9d2...
生成了 128 个认证标签
生成了 3 个关键词标签

✅ 文件加密成功！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
📦 加密文件: encrypted_report.enc
📋 元数据:   encrypted_report.json
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**输出文件说明：**
- `encrypted_report.enc` - 加密后的文件（包含IV和密文）
- `encrypted_report.json` - 元数据文件（文件ID、认证标签、关键词标签）

### 2. 解密文件

```
💻 > decrypt

📥 输入加密文件路径: encrypted_report.enc
💾 输出文件路径: decrypted_report.pdf

🔓 解密中...
解密文件: encrypted_report.enc
✅ 文件解密成功
保存到: decrypted_report.pdf
```

### 3. 生成搜索令牌

```
💻 > search-token

🔍 输入关键词: confidential
💾 输出JSON文件路径: search_confidential.json

生成搜索令牌: confidential
✅ 搜索令牌已生成
保存到: search_confidential.json
```

**搜索令牌用途：** 可用于在不解密文件的情况下，在存储系统中搜索包含特定关键词的文件。

### 4. 加载已有密钥

```
💻 > load-keys

📂 输入密钥文件路径: my_keys.dat
✅ 密钥已从 my_keys.dat 加载
```

---

## 📋 命令参考

| 命令 | 快捷键 | 功能 |
|------|--------|------|
| `init` | 1 | 初始化加密系统（从JSON加载配对参数） |
| `keygen` | 2 | 生成新的加密密钥 |
| `save-keys` | 3 | 保存密钥到文件 |
| `load-keys` | 4 | 从文件加载密钥 |
| `encrypt` | 5 | 加密文件并生成元数据 |
| `decrypt` | 6 | 解密文件 |
| `search-token` | 7 | 生成搜索令牌 |
| `help` | 8 | 显示帮助信息 |
| `quit` | 9 | 退出程序 |

---

## 📄 文件格式说明

### 1. 系统参数配置文件（system_params.json）⭐ 新增

这是v3.1版本新增的核心配置文件，必须与可执行文件在同一目录。

```json
{
  "pairing_type": "a",
  "parameters": {
    "q": "8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791",
    "h": "12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776",
    "r": "730750818665451621361119245571504901405976559617",
    "exp2": "159",
    "exp1": "107",
    "sign1": "1",
    "sign0": "1"
  },
  "system_values": {
    "N": "730750818665451621361119245571504901405976559617"
  },
  "description": "Type A pairing parameters for 512-bit security level",
  "security_level": "512-bit",
  "version": "1.0"
}
```

**字段说明：**
- `pairing_type`: 配对类型（Type A）
- `parameters`: 配对密码学参数（见下文详细解释）
- `system_values`: 系统运算参数
- `security_level`: 安全级别标识

详细的参数解释请参见 [PARAMETERS.md](PARAMETERS.md) 文件。

### 2. 加密文件格式（.enc）

```
[16字节 IV][AES-256-CBC 加密数据]
```

### 3. 元数据格式（.json）

```json
{
  "file_id": "a3f5e8d2b1c7a4f9...",
  "original_filename": "report.pdf",
  "encrypted_filename": "encrypted_report.enc",
  "file_size": 524288,
  "timestamp": 1699459200,
  "keywords": ["confidential", "finance", "Q4-2024"],
  "auth_tags": [
    "tag_block_0_hex",
    "tag_block_1_hex",
    ...
  ],
  "keyword_tags": {
    "confidential": {
      "kt": "keyword_tag_hex",
      "state_token": "state_token_hex",
      "pointer": "pointer_hex",
      "current_state": "current_state_hex"
    },
    ...
  }
}
```

### 4. 搜索令牌格式（.json）

```json
{
  "keyword": "confidential",
  "search_token": "encrypted_keyword_hex",
  "latest_state": "state_hex",
  "seed": "random_seed_hex",
  "timestamp": 1699459200
}
```

---

## 🔐 安全性说明

### 加密算法

| 组件 | 算法 | 安全级别 |
|------|------|----------|
| 文件加密 | AES-256-CBC | 256位 |
| 关键词加密 | AES-256-CBC | 256位 |
| 认证标签 | 配对密码学（PBC） | 512位 |
| 哈希函数 | SHA-256 | 256位 |

### 前向安全性

本工具实现了**前向安全的可搜索加密**：
- 每次上传新文件时，关键词状态会更新
- 即使获得当前搜索令牌，也无法搜索历史文件
- 保护用户的搜索隐私

### 密钥管理建议

**DO（推荐）：**
- ✅ 立即备份密钥文件到多个安全位置
- ✅ 使用强密码加密存储密钥文件
- ✅ 定期测试密钥备份是否可用
- ✅ 将密钥存储在离线设备（U盘、硬盘）
- ✅ 妥善保管 `system_params.json` 配置文件

**DON'T（禁止）：**
- ❌ 不要将密钥文件存储在云端（除非加密）
- ❌ 不要通过邮件/聊天软件发送密钥
- ❌ 不要在公共电脑上生成或使用密钥
- ❌ 不要忘记备份密钥
- ❌ 不要随意修改配置文件参数（除非理解其含义）

---

## 🔧 配置文件管理

### 更换系统参数

如需更换更高安全级别的参数：

1. **备份当前配置**
   ```bash
   cp system_params.json system_params_backup.json
   ```

2. **编辑配置文件**
   ```bash
   nano system_params.json
   ```

3. **重新初始化系统**
   ```bash
   ./storage_client
   > init
   ```

### 多环境配置

可以创建多个配置文件用于不同环境：

```bash
# 开发环境（较低安全级别，快速测试）
system_params_dev.json

# 生产环境（高安全级别）
system_params_prod.json

# 使用时重命名
cp system_params_prod.json system_params.json
```

---

## 🛠️ 常见问题

### Q1：丢失密钥文件怎么办？
**A：无法恢复！** 密钥丢失后，所有加密文件将永久无法解密。请务必做好备份。

### Q2：丢失 system_params.json 怎么办？
**A：** 可以从项目源码或其他已部署的实例复制该文件。参数是公开的，不影响安全性。但如果参数不一致，将无法正确验证文件。

### Q3：可以修改已加密文件的关键词吗？
**A：不可以。** 关键词在加密时就已固定。如需修改，请先解密，然后用新关键词重新加密。

### Q4：初始化失败：找不到配置文件
**A：** 请确保 `system_params.json` 与可执行文件在同一目录。检查文件名拼写是否正确。

### Q5：可以自定义系统参数吗？
**A：** 可以，但需要深入理解配对密码学。建议使用默认参数或咨询密码学专家。错误的参数可能导致安全性降低。

### Q6：元数据文件（.json）可以公开吗？
**A：不建议。** 虽然元数据不包含文件内容，但包含关键词标签信息，可能泄露隐私。

### Q7：如何验证文件完整性？
**A：** 元数据中的 `auth_tags` 用于完整性验证。可以开发验证工具来检查文件是否被篡改。

### Q8：支持批量加密吗？
**A：** 当前版本不支持。可以编写脚本循环调用 `encrypt` 命令。

---

## 📊 性能参数

| 项目 | 参数 |
|------|------|
| 块大小 | 4096 字节 |
| 扇区大小 | 256 字节 |
| 最大文件大小 | 理论无限制（受存储空间限制） |
| 加密速度 | ~50 MB/s（取决于CPU性能） |
| 解密速度 | ~50 MB/s（取决于CPU性能） |
| 配置加载时间 | <100ms |

---

## 🔬 技术细节

### 系统架构

```
系统启动
   ↓
加载 system_params.json
   ↓
初始化配对密码系统
   ↓
用户文件
   ↓
AES-256-CBC 加密
   ↓
分块（4KB/块）
   ↓
生成认证标签（配对密码学）
   ↓
生成关键词标签（可搜索加密）
   ↓
输出：.enc + .json
```

### 核心组件

1. **配置管理（新增）**
   - JSON解析与验证
   - 参数动态加载
   - 错误处理与提示

2. **配对密码系统**
   - 使用 PBC 库
   - Type A 曲线
   - 可配置参数
   - 512位安全性（默认）

3. **哈希函数**
   - H1: 字符串 → Zp
   - H2: 字符串 → G1
   - H3: 字符串 → 字符串（SHA-256）

4. **关键词状态链**
   - 每个关键词维护一个状态
   - 状态通过指针连接（前向安全）
   - 支持高效的关键词搜索

---

## 📂 项目文件结构

```
project/
├── client.h              # 客户端头文件
├── client.cpp            # 客户端实现
├── main.cpp              # 主程序
├── system_params.json    # 系统参数配置 ⭐ 必需
├── PARAMETERS.md         # 参数详细说明 ⭐ 新增
├── CMakeLists.txt        # CMake配置
└── README.md             # 本文件
```

---

## 🤝 贡献

欢迎贡献代码！请遵循以下步骤：

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开 Pull Request

---

## 📜 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 🙏 致谢

本项目基于以下研究论文：

**"Enabling Verifiable Search and Integrity Auditing in Encrypted Decentralized Storage Using One Proof"**  
作者：Mingyang Song, Zhongyun Hua, et al.

使用的开源库：
- [PBC Library](https://crypto.stanford.edu/pbc/) - 配对密码学
- [OpenSSL](https://www.openssl.org/) - 加密算法
- [GMP](https://gmplib.org/) - 大数运算
- [JsonCpp](https://github.com/open-source-parsers/jsoncpp) - JSON解析

---

## 📞 支持

- 📧 Email: support@example.com
- 🐛 Issues: [GitHub Issues](https://github.com/your-repo/issues)
- 📖 文档: [完整文档](https://docs.example.com)

---

## 📋 更新日志

### v3.1.0 (2025-11-08)
- ✨ 新增：JSON配置文件支持
- 🔧 改进：系统参数可配置化
- 📝 新增：PARAMETERS.md 参数说明文档
- 🐛 修复：配置文件缺失时的错误提示
- 📖 更新：README文档

### v3.0.0 (2025-11-01)
- 🎉 初始版本发布
- ✅ 基本加密解密功能
- ✅ 可搜索加密
- ✅ 前向安全性

---

## ⚠️ 免责声明

本工具仅供学习和研究使用。作者不对使用本工具造成的任何数据丢失或安全问题负责。请务必：

1. 在重要文件加密前先备份
2. 妥善保管密钥文件和配置文件
3. 在生产环境使用前充分测试
4. 遵守当地法律法规
5. 不要随意修改配置参数（除非理解其含义）

---

**版本**: 3.1.0  
**最后更新**: 2025年11月08日  
**状态**: 生产就绪 ✅  
**配置方式**: JSON导入 🆕

---

**Stay Secure! 🔐**


# 版本更新说明 - v3.2

## 🎉 更新概述

本次更新为 v3.1 版本增加了**关键词状态管理功能**，升级至 v3.2。

---

## ✨ 新增功能

### 1. 关键词状态文件管理
- 新增 `keyword_states.json` 文件，独立管理所有关键词的历史状态
- 与密钥文件分离，便于备份和版本控制
- 支持手动加载、保存和查询

### 2. 新增命令

| 命令 | 快捷键 | 功能 |
|------|--------|------|
| `load-states` | 10 | 加载关键词状态文件 |
| `save-states` | 11 | 保存关键词状态文件 |
| `query-state` | 12 | 查询关键词当前状态和历史 |

### 3. 自动状态更新
- 加密文件时，如果已加载状态文件，会自动更新状态并保存
- 无需手动操作，提升使用便利性

---

## 📝 修改文件清单

### 修改的文件

1. **client.h**
   - 新增函数声明：
     - `loadKeywordStates()`
     - `saveKeywordStates()`
     - `updateKeywordState()`
     - `queryKeywordState()`
     - `getCurrentTimestamp()` (辅助函数)
   - 新增成员变量：
     - `keyword_states_file_` - 状态文件路径
     - `states_loaded_` - 加载标记
     - `keyword_states_data_` - JSON数据存储

2. **client.cpp**
   - 实现所有新增函数
   - 修改 `encryptFile()` 函数，添加自动状态更新逻辑
   - 修改构造函数，初始化 `states_loaded_ = false`

3. **main.cpp**
   - 添加三个新命令的处理逻辑
   - 更新帮助菜单显示
   - 更新版本号为 v3.2
   - 添加状态管理提示信息

---

## 📦 新增文件

1. **keyword_states_example.json** - 示例状态文件
2. **KEYWORD_STATES_USAGE.md** - 详细使用说明文档

---

## 🔄 工作流程变化

### 之前的流程 (v3.1)
```
init → keygen → encrypt → encrypt → ...
```

### 现在的流程 (v3.2)
```
init → keygen → load-states → encrypt → encrypt → ...
                  ↑                ↓
                  └─── 自动更新 ────┘
```

---

## 💡 使用示例

### 快速开始

```bash
# 1. 初始化系统
./storage_client
💻 > init
💻 > keygen
💻 > save-keys
输入密钥文件路径: my_keys.dat

# 2. 首次加密（如果状态文件不存在）
💻 > encrypt
# ... 输入文件和关键词 ...

# 3. 保存状态文件
💻 > save-states
输入保存路径: keyword_states.json

# 4. 下次使用时先加载状态
💻 > load-keys
输入密钥文件路径: my_keys.dat

💻 > load-states
输入状态文件路径: keyword_states.json

# 5. 查询关键词状态
💻 > query-state
输入要查询的关键词: confidential

# 6. 继续加密（自动更新状态）
💻 > encrypt
# 状态会自动更新到 keyword_states.json
```

---

## 🔍 状态文件格式

```json
{
  "version": "1.0",
  "last_updated": "2025-11-09T14:30:00",
  "keywords": {
    "关键词名": {
      "current_state": "当前状态值（64字符十六进制）",
      "history": [
        {
          "state": "历史状态值",
          "file_id": "关联的文件ID",
          "timestamp": "ISO 8601时间戳",
          "is_current": true/false
        }
      ]
    }
  }
}
```

---

## ⚠️ 重要提示

### 状态文件的作用
- **记录关键词演化**：每个关键词的所有历史状态
- **支持前向安全**：保证搜索令牌的有效性
- **审计追踪**：可追溯每个关键词的使用历史

### 注意事项
1. ✅ 状态文件可以与多人共享（不包含敏感信息）
2. ✅ 建议定期备份状态文件
3. ⚠️ 状态文件丢失不影响解密，但会丢失历史记录
4. ⚠️ 多台机器同时修改同一状态文件可能导致冲突

---

## 🆚 与 v3.1 的区别

| 特性 | v3.1 | v3.2 |
|------|------|------|
| 状态存储 | 仅在密钥文件中 | 独立的JSON文件 |
| 历史记录 | 无 | 完整的状态历史 |
| 状态查询 | 不支持 | 支持查询和显示 |
| 状态管理 | 自动（不可见） | 手动+自动结合 |
| 文件格式 | 二进制 | JSON（可读） |

---

## 📖 详细文档

完整的使用说明请查看：**KEYWORD_STATES_USAGE.md**

---

## 🔧 编译说明

编译方式与 v3.1 相同，无需额外依赖：

```bash
mkdir build && cd build
cmake ..
make -j4
```

---

## 🐛 已知问题

无

---

## 🚀 后续计划

可能的增强功能：
- [ ] 状态文件加密
- [ ] 批量导入/导出关键词
- [ ] 状态冲突自动合并
- [ ] Web界面可视化管理

---

## 📞 反馈与支持

如有问题或建议，请联系开发者。

---

**版本**: 3.2  
**发布日期**: 2025-11-09  
**兼容性**: 向后兼容 v3.1 的所有功能