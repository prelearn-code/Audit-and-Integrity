#include "client.h"
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

void printUsage() {
    std::cout << "\n=========================================" << std::endl;
    std::cout << "  本地加密存储工具 v3.2" << std::endl;
    std::cout << "=========================================" << std::endl;
    std::cout << "\n🔧 系统设置:" << std::endl;
    std::cout << "  1.  init           - 初始化加密系统（从JSON加载参数）" << std::endl;
    std::cout << "  2.  keygen         - 生成密钥" << std::endl;
    std::cout << "  3.  save-keys      - 保存密钥到文件" << std::endl;
    std::cout << "  4.  load-keys      - 从文件加载密钥" << std::endl;
    std::cout << "\n📁 文件操作:" << std::endl;
    std::cout << "  5.  encrypt        - 加密文件（生成.enc + .json）" << std::endl;
    std::cout << "  6.  decrypt        - 解密文件" << std::endl;
    std::cout << "\n🔍 搜索功能:" << std::endl;
    std::cout << "  7.  search-token   - 生成搜索令牌" << std::endl;
    std::cout << "\n📊 状态管理: ⭐ 新增功能" << std::endl;
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
    std::cout << "  🔐 本地加密存储工具 - v3.2" << std::endl;
    std::cout << "  可验证的可搜索加密系统（支持JSON配置）" << std::endl;
    std::cout << "  ⭐ 新增: 关键词状态管理功能" << std::endl;
    std::cout << "==================================================" << std::endl;
}

int main() {
    printBanner();
    
    StorageClient client;
    
    // 检查系统参数配置文件是否存在
    std::ifstream config_check("system_params.json");
    if (!config_check.good()) {
        std::cout << "\n⚠️  警告: 未找到 system_params.json 配置文件" << std::endl;
        std::cout << "   请确保配置文件存在于程序同目录下" << std::endl;
        std::cout << "   否则初始化系统时将失败\n" << std::endl;
    } else {
        std::cout << "\n✅ 检测到系统参数配置文件\n" << std::endl;
    }
    config_check.close();
    
    printUsage();
    
    std::string command;
    bool running = true;
    
    while (running) {
        std::cout << "\n💻 > ";
        std::cin >> command;
        
        try {
            if (command == "init" || command == "1") {
                std::cout << "\n⚙️  初始化加密系统..." << std::endl;
                std::cout << "📄 使用配置文件: system_params.json" << std::endl;
                std::cout << "💡 提示: 请确保配置文件在程序同目录下\n" << std::endl;
                
                if (client.initialize()) {
                    std::cout << "✅ 系统初始化成功" << std::endl;
                } else {
                    std::cerr << "❌ 系统初始化失败" << std::endl;
                    std::cerr << "💡 请检查 system_params.json 文件是否存在且格式正确" << std::endl;
                }
            }
            else if (command == "keygen" || command == "2") {
                std::cout << "\n🔑 生成密钥..." << std::endl;
                if (client.generateKeys()) {
                    std::cout << "✅ 密钥生成成功" << std::endl;
                    std::cout << "📌 公钥: " << client.getPublicKey().substr(0, 32) << "..." << std::endl;
                } else {
                    std::cerr << "❌ 密钥生成失败" << std::endl;
                }
            }
            else if (command == "save-keys" || command == "3") {
                std::string key_file;
                std::cout << "\n💾 输入密钥文件路径: ";
                std::cin >> key_file;
                
                if (client.saveKeys(key_file)) {
                    std::cout << "✅ 密钥保存成功" << std::endl;
                } else {
                    std::cerr << "❌ 密钥保存失败" << std::endl;
                }
            }
            else if (command == "load-keys" || command == "4") {
                std::string key_file;
                std::cout << "\n📂 输入密钥文件路径: ";
                std::cin >> key_file;
                
                if (client.loadKeys(key_file)) {
                    std::cout << "✅ 密钥加载成功" << std::endl;
                } else {
                    std::cerr << "❌ 密钥加载失败" << std::endl;
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
                    // 去除首尾空格
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
                std::cout << "💾 输出文件前缀（将生成 .enc 和 .json）: ";
                std::cin >> output_prefix;
                
                std::cout << "\n🔐 加密中..." << std::endl;
                if (client.encryptFile(file_path, keywords, output_prefix)) {
                    std::cout << "\n🎉 加密完成！" << std::endl;
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
                
                std::cout << "\n🔓 解密中..." << std::endl;
                if (client.decryptFile(encrypted_file, output_path)) {
                    std::cout << "✅ 解密成功！" << std::endl;
                } else {
                    std::cerr << "❌ 文件解密失败" << std::endl;
                }
            }
            else if (command == "search-token" || command == "7") {
                std::string keyword;
                std::cout << "\n🔍 输入关键词: ";
                std::cin >> keyword;
                
                std::string output_file;
                std::cout << "💾 输出JSON文件路径: ";
                std::cin >> output_file;
                
                if (client.generateSearchToken(keyword, output_file)) {
                    std::cout << "✅ 搜索令牌生成成功！" << std::endl;
                } else {
                    std::cerr << "❌ 搜索令牌生成失败" << std::endl;
                }
            }
            // ============ 新增命令 ============
            else if (command == "load-states" || command == "10") {
                std::string state_file;
                std::cout << "\n📂 输入状态文件路径: ";
                std::cin >> state_file;
                
                std::cout << "\n📥 加载关键词状态..." << std::endl;
                if (client.loadKeywordStates(state_file)) {
                    std::cout << "✅ 状态文件加载成功" << std::endl;
                    std::cout << "💡 提示: 加密文件时会自动更新此状态文件" << std::endl;
                } else {
                    std::cerr << "❌ 状态文件加载失败" << std::endl;
                }
            }
            else if (command == "save-states" || command == "11") {
                std::string state_file;
                std::cout << "\n💾 输入保存路径: ";
                std::cin >> state_file;
                
                std::cout << "\n💾 保存关键词状态..." << std::endl;
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
                std::cout << "\n👋 感谢使用本地加密存储工具！" << std::endl;
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