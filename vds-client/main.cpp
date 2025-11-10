#include "client.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

void printUsage() {
    std::cout << "\n=========================================" << std::endl;
    std::cout << "  本地加密存储工具 v4.0" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "\n🔧 系统设置:" << std::endl;
    std::cout << "  1.  init           - 初始化系统（从 public_params.json 加载参数）" << std::endl;
    std::cout << "  2.  keygen         - 生成密钥（需先初始化系统）" << std::endl;
    std::cout << "  3.  save-keys      - 保存密钥到文件" << std::endl;
    std::cout << "  4.  load-keys      - 从文件加载密钥" << std::endl;
    std::cout << "\n📁 文件操作:" << std::endl;
    std::cout << "  5.  encrypt        - 加密文件（生成 .enc, insert.json, metadata.json）" << std::endl;
    std::cout << "  6.  decrypt        - 解密文件" << std::endl;
    std::cout << "\n📊 状态管理:" << std::endl;
    std::cout << "  10. load-states    - 加载关键词状态文件" << std::endl;
    std::cout << "  11. save-states    - 保存关键词状态文件" << std::endl;
    std::cout << "  12. query-state    - 查询关键词当前状态" << std::endl;
    std::cout << "\n📖 其他:" << std::endl;
    std::cout << "  13. help           - 显示帮助" << std::endl;
    std::cout << "  14. quit           - 退出" << std::endl;
    std::cout << "=========================================\n" << std::endl;
}

void printBanner() {
    std::cout << "==================================================" << std::endl;
    std::cout << "  🔐 本地加密存储工具 - v4.0" << std::endl;
    std::cout << "  可验证的可搜索加密系统（方案A重构版）" << std::endl;
    std::cout << "  ⭐ v4.0 新特性:" << std::endl;
    std::cout << "     - 统一使用 public_params.json" << std::endl;
    std::cout << "     - 配对参数硬编码（Type A曲线）" << std::endl;
    std::cout << "     - 修复参数不一致问题" << std::endl;
    std::cout << "==================================================" << std::endl;
}

void printInitializationGuide() {
    std::cout << "\n┌─────────────────────────────────────────┐" << std::endl;
    std::cout << "│  📘 初始化指南（重要！）                │" << std::endl;
    std::cout << "├─────────────────────────────────────────┤" << std::endl;
    std::cout << "│  v4.0 简化了初始化流程：                │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  1️⃣  获取 public_params.json           │" << std::endl;
    std::cout << "│     从 Storage Node 获取此文件          │" << std::endl;
    std::cout << "│     包含: N, g, μ 三个公共参数          │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  2️⃣  初始化系统                        │" << std::endl;
    std::cout << "│     运行命令: init                      │" << std::endl;
    std::cout << "│     系统会自动加载所有参数              │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  3️⃣  生成密钥                          │" << std::endl;
    std::cout << "│     运行命令: keygen                    │" << std::endl;
    std::cout << "│     生成 private_key.dat + public_key.json │" << std::endl;
    std::cout << "│                                         │" << std::endl;
    std::cout << "│  ⚠️  注意事项:                          │" << std::endl;
    std::cout << "│  - 不再需要 system_params.json         │" << std::endl;
    std::cout << "│  - 配对参数已硬编码到程序中            │" << std::endl;
    std::cout << "│  - 必须先 init 再 keygen               │" << std::endl;
    std::cout << "└─────────────────────────────────────────┘\n" << std::endl;
}

int main() {
    printBanner();
    
    StorageClient client;
    
    // ========================================
    // 检查 public_params.json（唯一必需的参数文件）
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
    printUsage();
    
    std::string command;
    bool running = true;
    
    while (running) {
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
                    std::cout << "💡 下一步: 运行 'keygen' 生成密钥" << std::endl;
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
                
                std::string output_prefix;
                std::cout << "💾 输出文件前缀（将生成 .enc 和相关 JSON）: ";
                std::cin >> output_prefix;
                
                std::string insert_json_path;
                std::cout << "💾 insert.json 输出路径（按回车使用默认: insert.json）: ";
                std::cin.ignore();
                std::getline(std::cin, insert_json_path);
                
                if (insert_json_path.empty()) {
                    insert_json_path = "insert.json";
                }
                
                if (client.encryptFile(file_path, keywords, output_prefix, insert_json_path)) {
                    std::cout << "\n✅ 加密完成！" << std::endl;
                    std::cout << "📦 生成的文件:" << std::endl;
                    std::cout << "   - " << output_prefix << ".enc（加密文件）" << std::endl;
                    std::cout << "   - " << insert_json_path << "（供 Storage Node 使用）" << std::endl;
                    std::cout << "   - " << file_path << "_metadata.json（本地元数据）" << std::endl;
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
            // ============ 状态管理命令 ============
            else if (command == "load-states" || command == "10") {
                std::string state_file;
                std::cout << "\n📂 输入状态文件路径: ";
                std::cin >> state_file;
                
                if (client.loadKeywordStates(state_file)) {
                    std::cout << "✅ 状态文件加载成功" << std::endl;
                } else {
                    std::cerr << "❌ 状态文件加载失败" << std::endl;
                }
            }
            else if (command == "save-states" || command == "11") {
                std::string state_file;
                std::cout << "\n💾 输入保存路径: ";
                std::cin >> state_file;
                
                if (client.saveKeywordStates(state_file)) {
                    std::cout << "✅ 状态文件保存成功: " << state_file << std::endl;
                } else {
                    std::cerr << "❌ 状态文件保存失败" << std::endl;
                }
            }
            else if (command == "query-state" || command == "12") {
                std::string keyword;
                std::cout << "\n🔍 输入要查询的关键词: ";
                std::cin >> keyword;
                
                std::string result = client.queryKeywordState(keyword);
                std::cout << result << std::endl;
            }
            else if (command == "help" || command == "13") {
                printUsage();
            }
            else if (command == "quit" || command == "exit" || command == "14") {
                std::cout << "\n👋 感谢使用本地加密存储工具 v4.0！" << std::endl;
                std::cout << "   记得保存您的密钥文件和状态文件。\n" << std::endl;
                running = false;
            }
            else {
                std::cerr << "❌ 未知命令。输入 'help' 查看帮助。" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "❌ 错误: " << e.what() << std::endl;
        }
    }
    
    return 0;
}