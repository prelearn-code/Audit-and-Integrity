#include "client.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <limits>

void printUsage() {
    std::cout << "\n=========================================" << std::endl;
    std::cout << "  本地加密存储工具 v4.2" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "\n🔧 系统设置:" << std::endl;
    std::cout << "  1.  init           - 初始化系统（从 public_params.json 加载参数）" << std::endl;
    std::cout << "  2.  keygen         - 生成密钥（需先初始化系统）" << std::endl;
    std::cout << "  3.  save-keys      - 保存密钥到文件" << std::endl;
    std::cout << "  4.  load-keys      - 从文件加载密钥" << std::endl;
    std::cout << "\n📁 文件操作:" << std::endl;
    std::cout << "  5.  encrypt        - 加密文件（自动管理所有输出文件）" << std::endl;
    std::cout << "  6.  decrypt        - 解密文件" << std::endl;
    std::cout << "  7.  delete         - 生成删除令牌" << std::endl;
    std::cout << "\n🔍 搜索操作:" << std::endl;
    std::cout << "  8.  search         - 生成搜索令牌" << std::endl;
    std::cout << "\n📊 状态查询:" << std::endl;
    std::cout << "  10. query-state    - 查询关键词当前状态" << std::endl;
    std::cout << "\n📖 其他:" << std::endl;
    std::cout << "  11. help           - 显示帮助" << std::endl;
    std::cout << "  12. quit           - 退出" << std::endl;
    std::cout << "=========================================\n" << std::endl;
}

void printBanner() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  🔐 本地加密存储工具 - v4.2" << std::endl;
    std::cout << "  可验证的可搜索加密系统" << std::endl;
    std::cout << "  ⭐ v4.2 新特性:" << std::endl;
    std::cout << "     - 新增删除令牌生成功能（delete）" << std::endl;
    std::cout << "     - 新增搜索令牌生成功能（search）" << std::endl;
    std::cout << "     - Deles/ 和 Search/ 目录自动创建" << std::endl;
    std::cout << "  ⭐ v4.1 特性:" << std::endl;
    std::cout << "     - 统一数据目录管理（./data）" << std::endl;
    std::cout << "     - 使用原始文件名" << std::endl;
    std::cout << "     - 自动更新 keyword_states.json" << std::endl;
    std::cout << "==================================================" << std::endl;
}

void printInitializationGuide() {
    std::cout << "\n┌─────────────────────────────────────────┐" << std::endl;
    std::cout << "│  📘 初始化指南（重要！）                │" << std::endl;
    std::cout << "├─────────────────────────────────────────┤" << std::endl;
    std::cout << "│  v4.1 简化了初始化和文件管理：          │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  1️⃣  获取 public_params.json           │" << std::endl;
    std::cout << "│     从 Storage Node 获取此文件          │" << std::endl;
    std::cout << "│     包含: N, g, μ 三个公共参数          │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  2️⃣  初始化系统                        │" << std::endl;
    std::cout << "│     运行命令: init                      │" << std::endl;
    std::cout << "│     系统会自动：                        │" << std::endl;
    std::cout << "│     • 加载所有参数                      │" << std::endl;
    std::cout << "│     • 创建 ./data 目录结构              │" << std::endl;
    std::cout << "│     • 初始化 keyword_states.json        │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  3️⃣  生成密钥                          │" << std::endl;
    std::cout << "│     运行命令: keygen                    │" << std::endl;
    std::cout << "│     生成 private_key.dat + public_key.json │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  4️⃣  加密文件                          │" << std::endl;
    std::cout << "│     运行命令: encrypt                   │" << std::endl;
    std::cout << "│     只需指定：                          │" << std::endl;
    std::cout << "│     • 文件路径                          │" << std::endl;
    std::cout << "│     • 关键词                            │" << std::endl;
    std::cout << "│     系统自动管理其他所有文件！          │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  ⚠️  注意事项:                          │" << std::endl;
    std::cout << "│  - 所有文件自动保存到 ./data 目录       │" << std::endl;
    std::cout << "│  - keyword_states.json 自动更新         │" << std::endl;
    std::cout << "│  - 文件重复时自动添加时间戳后缀         │" << std::endl;
    std::cout << "└─────────────────────────────────────────┘\n" << std::endl;
}

void printDataDirectoryStructure() {
    std::cout << "\n📂 数据目录结构:" << std::endl;
    std::cout << "./data/" << std::endl;
    std::cout << "├── Insert/           # insert.json 文件（供 Storage Node）" << std::endl;
    std::cout << "├── Deles/            # 删除令牌文件 (v4.2新增)" << std::endl;
    std::cout << "├── EncFiles/         # 加密文件 (.enc)" << std::endl;
    std::cout << "├── MetaFiles/        # 元数据文件" << std::endl;
    std::cout << "├── Search/           # 搜索令牌文件" << std::endl;
    std::cout << "└── keyword_states.json  # 关键词状态（自动维护）\n" << std::endl;
}

int main() {
    printBanner();
    
    StorageClient client;
    
    // ========================================
    // v4.1新增：检查 public_params.json
    // ========================================
    std::ifstream pub_params_check("public_params.json");
    if (!pub_params_check.good()) {
        std::cout << "\n⚠️  警告: 未找到 public_params.json 文件" << std::endl;
        std::cout << "   此文件由 Storage Node 生成，包含系统公共参数" << std::endl;
        std::cout << "   如需初始化系统，请先从 Storage Node 获取此文件\n" << std::endl;
    } else {
        std::cout << "\n✅ 检测到 public_params.json" << std::endl;
        std::cout << "   您可以运行 'init' 命令初始化系统\n" << std::endl;
    }
    pub_params_check.close();
    
    printInitializationGuide();
    printDataDirectoryStructure();
    printUsage();
    
    std::string command;
    bool running = true;
    bool first_run = true;  // 标记是否第一次运行
    
    while (running) {
        // 非首次运行时，等待用户按任意键后再显示菜单
        if (!first_run) {
            std::cout << "\n按 Enter 键继续...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.get();
            std::cout << "\n" << std::string(50, '=') << std::endl;
            printUsage();
        }
        first_run = false;  // 第一次循环后设为false
        
        std::cout << "\n💻 > ";
        std::cin >> command;
        
        try {
            if (command == "init" || command == "1") {
                std::cout << "\n⚙️  初始化加密系统..." << std::endl;
                
                std::string pub_params_file;
                std::cout << "💡 输入 public_params.json 路径（按回车使用默认: public_params.json）: ";
                std::cin.ignore();
                std::getline(std::cin, pub_params_file);
                
                if (pub_params_file.empty()) {
                    pub_params_file = "public_params.json";
                }
                
                std::cout << "\n📄 从 " << pub_params_file << " 加载公共参数..." << std::endl;
                std::cout << "🔧 配对参数: Type A 曲线（硬编码）" << std::endl;
                std::cout << "📊 公共参数: N, g, μ（从文件加载）\n" << std::endl;
                
                if (client.initialize(pub_params_file)) {
                    std::cout << "\n✅ 系统初始化成功" << std::endl;
                    
                    // ========== v4.1新增：自动初始化数据目录 ==========
                    std::cout << "\n🔧 初始化数据目录结构..." << std::endl;
                    if (client.initializeDataDirectories()) {
                        std::cout << "✅ 数据目录初始化完成" << std::endl;
                        std::cout << "\n💡 下一步: 运行 'keygen' 生成密钥" << std::endl;
                    } else {
                        std::cerr << "❌ 数据目录初始化失败" << std::endl;
                        std::cerr << "   请检查文件系统权限" << std::endl;
                    }
                } else {
                    std::cerr << "\n❌ 系统初始化失败" << std::endl;
                    std::cerr << "💡 请检查:" << std::endl;
                    std::cerr << "   1. " << pub_params_file << " 文件是否存在" << std::endl;
                    std::cerr << "   2. 文件格式是否正确（需包含 N, g, mu）" << std::endl;
                }
            }
            else if (command == "keygen" || command == "2") {
                std::cout << "\n🔑 生成密钥..." << std::endl;
                std::cout << "⚠️  注意: 如果系统尚未初始化，此操作将失败\n" << std::endl;
                
                if (client.generateKeys()) {
                    std::cout << "\n✅ 密钥生成成功" << std::endl;
                    std::cout << "📌 生成的文件:" << std::endl;
                    std::cout << "   - private_key.dat（私钥，请妥善保管）" << std::endl;
                    std::cout << "   - public_key.json（公钥）" << std::endl;
                    std::cout << "\n💡 现在可以使用 'encrypt' 命令加密文件" << std::endl;
                } else {
                    std::cerr << "\n❌ 密钥生成失败" << std::endl;
                    std::cerr << "💡 可能的原因:" << std::endl;
                    std::cerr << "   1. 系统尚未初始化（请先运行 'init'）" << std::endl;
                    std::cerr << "   2. 配对参数未正确加载" << std::endl;
                }
            }
            else if (command == "save-keys" || command == "3") {
                std::string key_file;
                std::cout << "\n💾 输入密钥文件路径: ";
                std::cin >> key_file;
                
                if (client.saveKeys(key_file)) {
                    std::cout << "✅ 密钥保存成功: " << key_file << std::endl;
                } else {
                    std::cerr << "❌ 密钥保存失败" << std::endl;
                }
            }
            else if (command == "load-keys" || command == "4") {
                std::string key_file;
                std::cout << "\n📂 输入密钥文件路径: ";
                std::cin >> key_file;
                
                std::cout << "\n💡 提示: 加载密钥前必须先初始化系统" << std::endl;
                std::cout << "   如果看到错误，请先运行 'init' 命令\n" << std::endl;
                
                if (client.loadKeys(key_file)) {
                    std::cout << "✅ 密钥加载成功: " << key_file << std::endl;
                } else {
                    std::cerr << "❌ 密钥加载失败" << std::endl;
                    std::cerr << "💡 请确保:" << std::endl;
                    std::cerr << "   1. 已初始化系统（运行 'init'）" << std::endl;
                    std::cerr << "   2. 密钥文件存在且格式正确" << std::endl;
                }
            }
            else if (command == "encrypt" || command == "5") {
                std::string file_path;
                std::cout << "\n📄 输入文件路径: ";
                std::cin >> file_path;
                
                std::cout << "🏷️  输入关键词（逗号分隔）: ";
                std::string keywords_str;
                std::cin.ignore();
                std::getline(std::cin, keywords_str);
                
                std::vector<std::string> keywords;
                std::stringstream ss(keywords_str);
                std::string keyword;
                while (std::getline(ss, keyword, ',')) {
                    keyword.erase(0, keyword.find_first_not_of(" \t"));
                    keyword.erase(keyword.find_last_not_of(" \t") + 1);
                    if (!keyword.empty()) {
                        keywords.push_back(keyword);
                    }
                }
                
                if (keywords.empty()) {
                    std::cerr << "❌ 至少需要一个关键词" << std::endl;
                    continue;
                }
                
                // ========== v4.1简化：不再需要用户输入输出路径 ==========
                std::cout << "\n🔒 开始加密..." << std::endl;
                std::cout << "💡 所有文件将自动保存到 ./data 目录" << std::endl;
                
                if (client.encryptFile(file_path, keywords)) {
                    std::cout << "\n✅ 加密完成！" << std::endl;
                    std::cout << "📂 所有文件已保存到 ./data 目录下的对应子目录" << std::endl;
                    std::cout << "   查看详细信息请查看上方的输出" << std::endl;
                } else {
                    std::cerr << "❌ 文件加密失败" << std::endl;
                }
            }
            else if (command == "decrypt" || command == "6") {
                std::string encrypted_file;
                std::cout << "\n📥 输入加密文件路径: ";
                std::cin >> encrypted_file;
                
                std::string output_path;
                std::cout << "💾 输出文件路径: ";
                std::cin >> output_path;
                
                if (client.decryptFile(encrypted_file, output_path)) {
                    std::cout << "✅ 解密成功: " << output_path << std::endl;
                } else {
                    std::cerr << "❌ 文件解密失败" << std::endl;
                }
            }
            else if (command == "delete" || command == "7") {
                std::string file_id;
                std::cout << "\n🗑️  输入文件ID (ID_F): ";
                std::cin >> file_id;
                
                std::cout << "\n💡 提示: 删除令牌用于授权 Storage Node 删除指定文件" << std::endl;
                std::cout << "   令牌将保存到 ../data/Deles/ 目录\n" << std::endl;
                
                if (client.deleteFile(file_id)) {
                    std::cout << "\n✅ 删除令牌生成成功！" << std::endl;
                    std::cout << "📌 生成的文件:" << std::endl;
                    std::cout << "   - ../data/Deles/" << file_id << ".json" << std::endl;
                    std::cout << "\n💡 下一步: 将此文件发送给 Storage Node 执行删除操作" << std::endl;
                } else {
                    std::cerr << "\n❌ 删除令牌生成失败！" << std::endl;
                    std::cerr << "💡 可能的原因:" << std::endl;
                    std::cerr << "   1. 系统尚未初始化（请先运行 'init'）" << std::endl;
                    std::cerr << "   2. 文件ID格式错误" << std::endl;
                }
            }
            else if (command == "search" || command == "8") {
                std::string keyword;
                std::cout << "\n🔍 输入关键词 (w): ";
                std::cin >> keyword;
                
                std::cout << "\n💡 提示: 搜索令牌用于在 Storage Node 上搜索包含该关键词的文件" << std::endl;
                std::cout << "   令牌将保存到 ../data/Search/ 目录\n" << std::endl;
                
                if (client.searchKeyword(keyword)) {
                    std::cout << "\n✅ 搜索令牌生成成功！" << std::endl;
                    std::cout << "📌 生成的文件:" << std::endl;
                    std::cout << "   - ../data/Search/" << keyword << ".json" << std::endl;
                    std::cout << "\n💡 下一步: 将此文件发送给 Storage Node 执行搜索操作" << std::endl;
                } else {
                    std::cerr << "\n❌ 搜索令牌生成失败！" << std::endl;
                    std::cerr << "💡 可能的原因:" << std::endl;
                    std::cerr << "   1. 系统尚未初始化（请先运行 'init'）" << std::endl;
                    std::cerr << "   2. 关键词格式错误" << std::endl;
                }
            }
            // ========== v4.1修改：移除状态文件手动管理命令 ==========
            // 状态文件现在自动管理，用户无需手动加载或保存
            
            else if (command == "query-state" || command == "10") {
                std::string keyword;
                std::cout << "\n🔍 输入要查询的关键词: ";
                std::cin >> keyword;
                
                std::string result = client.queryKeywordState(keyword);
                std::cout << result << std::endl;
            }
            else if (command == "help" || command == "11") {
                printUsage();
                printDataDirectoryStructure();
            }
            else if (command == "quit" || command == "exit" || command == "12") {
                std::cout << "\n👋 感谢使用本地加密存储工具 v4.2！" << std::endl;
                std::cout << "   所有数据已保存在 ./data 目录中。" << std::endl;
                std::cout << "   记得保护好您的密钥文件！\n" << std::endl;
                running = false;
            }
            else {
                std::cerr << "❌ 未知命令: " << command << std::endl;
                std::cerr << "   输入 'help' 或 '11' 查看完整命令列表。" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 错误: " << e.what() << std::endl;
        }
    }
    
    return 0;
}