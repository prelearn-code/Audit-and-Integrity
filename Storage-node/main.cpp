#include "storage_node.h"
#include <iostream>
#include <csignal>
#include <cstring>
#include <limits>

StorageNode* g_node = nullptr;

void signal_handler(int signal) {
    std::cout << "\n\n🛑 正在优雅地关闭存储节点..." << std::endl;
    if (g_node) {
        g_node->save_index_database();
        g_node->save_node_info();
        delete g_node;
    }
    exit(0);
}

void print_banner() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📦 去中心化存储节点控制台 v3.1" << std::endl;
    std::cout << "   ✨ 新增: 客户端公钥 (PK) 身份验证" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
}

void print_menu() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📋 主菜单" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "\n  1. 📤 插入文件 (需要JSON参数文件)" << std::endl;
    std::cout << "  2. 🔍 搜索关键词 (需要PK验证)" << std::endl;
    std::cout << "  3. 📥 检索文件" << std::endl;
    std::cout << "  4. 🗑️  删除文件 (需要PK验证)" << std::endl;
    std::cout << "  5. 🔐 生成完整性证明" << std::endl;
    std::cout << "  6. 📊 查看节点状态" << std::endl;
    std::cout << "  7. 📋 列出所有文件" << std::endl;
    std::cout << "  8. 💾 导出文件元数据" << std::endl;
    std::cout << "  9. 📄 查看详细状态" << std::endl;
    std::cout << "  0. 🚪 退出" << std::endl;
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "\n请选择操作 [0-9]: ";
}

void clear_input_buffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void wait_for_enter() {
    std::cout << "\n按 Enter 继续...";
    clear_input_buffer();
    std::cin.get();
}

void handle_insert_file(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📤 插入文件" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::string param_json_path, enc_file_path;
    
    std::cout << "\n💡 提示: JSON参数文件应包含以下字段:" << std::endl;
    std::cout << "   - PK: 客户端公钥" << std::endl;
    std::cout << "   - ID_F: 文件唯一标识" << std::endl;
    std::cout << "   - ptr: 文件指针" << std::endl;
    std::cout << "   - TS_F: 文件认证标签" << std::endl;
    std::cout << "   - state: 文件状态 (valid/invalid)" << std::endl;
    std::cout << "   - keywords: 关键词数组 [{'T_i': '...', 'kt_i': '...'}]" << std::endl;
    
    std::cout << "\n请输入参数JSON文件路径: ";
    clear_input_buffer();
    std::getline(std::cin, param_json_path);
    
    std::cout << "请输入加密文件路径: ";
    std::getline(std::cin, enc_file_path);
    
    if (node->insert_file(param_json_path, enc_file_path)) {
        std::cout << "\n🎉 操作成功完成!" << std::endl;
    } else {
        std::cout << "\n❌ 操作失败!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_search_keyword(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔍 搜索关键词" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::string pk, search_token, latest_state, seed;
    
    std::cout << "\n请输入客户端公钥 (PK): ";
    clear_input_buffer();
    std::getline(std::cin, pk);
    
    std::cout << "请输入搜索令牌 (T_i): ";
    std::getline(std::cin, search_token);
    
    std::cout << "请输入最新状态 (可选): ";
    std::getline(std::cin, latest_state);
    
    std::cout << "请输入种子 (可选): ";
    std::getline(std::cin, seed);
    
    SearchResult result = node->search_keyword(pk, search_token, latest_state, seed);
    
    std::cout << "\n📊 搜索结果:" << std::endl;
    std::cout << "   找到 " << result.file_identifiers.size() << " 个匹配文件" << std::endl;
    
    if (!result.file_identifiers.empty()) {
        std::cout << "\n📄 文件列表:" << std::endl;
        for (size_t i = 0; i < result.file_identifiers.size(); ++i) {
            std::cout << "   [" << (i+1) << "] " << result.file_identifiers[i];
            if (i < result.keyword_proofs.size()) {
                std::cout << " (关键词: " << result.keyword_proofs[i] << ")";
            }
            std::cout << std::endl;
        }
    } else {
        std::cout << "\n⚠️  未找到匹配的文件" << std::endl;
        std::cout << "   请检查:" << std::endl;
        std::cout << "   1. PK是否正确" << std::endl;
        std::cout << "   2. 搜索令牌是否正确" << std::endl;
        std::cout << "   3. 文件状态是否为 'valid'" << std::endl;
    }
    
    wait_for_enter();
}

void handle_retrieve_file(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📥 检索文件" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::string file_id;
    
    std::cout << "\n请输入文件ID: ";
    clear_input_buffer();
    std::getline(std::cin, file_id);
    
    Json::Value result = node->retrieve_file(file_id);
    
    if (result["success"].asBool()) {
        std::cout << "\n✅ 文件检索成功!" << std::endl;
        std::cout << "   文件ID:       " << result["file_id"].asString() << std::endl;
        std::cout << "   客户端PK:     " << result["PK"].asString().substr(0, 16) << "..." << std::endl;
        std::cout << "   密文大小:     " << result["ciphertext"].asString().length() << " 字节" << std::endl;
        std::cout << "   指针:         " << result["pointer"].asString().substr(0, 32) << "..." << std::endl;
        std::cout << "   认证标签:     " << result["file_auth_tag"].asString().substr(0, 32) << "..." << std::endl;
        std::cout << "   状态:         " << result["state"].asString() << std::endl;
        
        char save_choice;
        std::cout << "\n是否保存密文到文件? (y/n): ";
        std::cin >> save_choice;
        
        if (save_choice == 'y' || save_choice == 'Y') {
            std::string output_path;
            std::cout << "输出文件路径: ";
            clear_input_buffer();
            std::getline(std::cin, output_path);
            
            std::ofstream outfile(output_path, std::ios::binary);
            if (outfile.is_open()) {
                outfile << result["ciphertext"].asString();
                outfile.close();
                std::cout << "✅ 密文已保存到: " << output_path << std::endl;
            } else {
                std::cout << "❌ 无法保存文件" << std::endl;
            }
        }
    } else {
        std::cout << "\n❌ 文件不存在!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_delete_file(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🗑️  删除文件" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::string pk, file_id, del_proof;
    
    std::cout << "\n请输入客户端公钥 (PK): ";
    clear_input_buffer();
    std::getline(std::cin, pk);
    
    std::cout << "请输入文件ID: ";
    std::getline(std::cin, file_id);
    
    std::cout << "请输入删除证明 (可选): ";
    std::getline(std::cin, del_proof);
    
    std::cout << "\n⚠️  警告: 此操作将标记文件为无效!" << std::endl;
    std::cout << "   只有文件所有者 (PK匹配) 才能删除文件" << std::endl;
    char confirm;
    std::cout << "确认删除? (y/n): ";
    std::cin >> confirm;
    
    if (confirm == 'y' || confirm == 'Y') {
        if (node->delete_file(pk, file_id, del_proof)) {
            std::cout << "\n✅ 文件已删除!" << std::endl;
        } else {
            std::cout << "\n❌ 删除失败!" << std::endl;
        }
    } else {
        std::cout << "\n❌ 操作已取消" << std::endl;
    }
    
    wait_for_enter();
}

void handle_generate_proof(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔐 生成完整性证明" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::string file_id, seed;
    
    std::cout << "\n请输入文件ID: ";
    clear_input_buffer();
    std::getline(std::cin, file_id);
    
    std::cout << "请输入种子 (可选): ";
    std::getline(std::cin, seed);
    
    std::string proof = node->generate_integrity_proof(file_id, seed);
    
    if (!proof.empty()) {
        std::cout << "\n✅ 完整性证明生成成功!" << std::endl;
        std::cout << "   证明: " << proof << std::endl;
        
        char save_choice;
        std::cout << "\n是否保存证明到文件? (y/n): ";
        std::cin >> save_choice;
        
        if (save_choice == 'y' || save_choice == 'Y') {
            std::string output_path;
            std::cout << "输出文件路径: ";
            clear_input_buffer();
            std::getline(std::cin, output_path);
            
            std::ofstream outfile(output_path);
            if (outfile.is_open()) {
                outfile << proof;
                outfile.close();
                std::cout << "✅ 证明已保存到: " << output_path << std::endl;
            }
        }
    } else {
        std::cout << "\n❌ 文件不存在或证明生成失败!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_view_status(StorageNode* node) {
    node->print_status();
    wait_for_enter();
}

void handle_list_files(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📋 所有文件列表" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::vector<std::string> files = node->list_all_files();
    
    if (files.empty()) {
        std::cout << "\n⚠️  当前没有存储任何文件" << std::endl;
    } else {
        std::cout << "\n📄 共有 " << files.size() << " 个文件:\n" << std::endl;
        
        for (size_t i = 0; i < files.size(); ++i) {
            std::cout << "   [" << (i+1) << "] " << files[i] << std::endl;
            
            // 显示元数据
            Json::Value metadata = node->get_file_metadata(files[i]);
            if (metadata.isMember("PK")) {
                std::cout << "       PK: " << metadata["PK"].asString().substr(0, 16) << "...";
            }
            if (metadata.isMember("file_size")) {
                std::cout << ", 大小: " << metadata["file_size"].asInt() << " 字节";
            }
            if (metadata.isMember("keyword_count")) {
                std::cout << ", 关键词: " << metadata["keyword_count"].asInt();
            }
            if (metadata.isMember("state")) {
                std::cout << ", 状态: " << metadata["state"].asString();
            }
            if (metadata.isMember("insert_time")) {
                std::cout << ", 插入时间: " << metadata["insert_time"].asString();
            }
            std::cout << std::endl;
        }
    }
    
    wait_for_enter();
}

void handle_export_metadata(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "💾 导出文件元数据" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::string file_id, output_path;
    
    std::cout << "\n请输入文件ID: ";
    clear_input_buffer();
    std::getline(std::cin, file_id);
    
    std::cout << "请输入输出文件路径: ";
    std::getline(std::cin, output_path);
    
    if (node->export_file_metadata(file_id, output_path)) {
        std::cout << "\n✅ 元数据已导出到: " << output_path << std::endl;
    } else {
        std::cout << "\n❌ 导出失败!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_detailed_status(StorageNode* node) {
    node->print_detailed_status();
    wait_for_enter();
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // 默认参数
    std::string data_dir = "./data";
    int port = 9000;
    
    // 解析命令行参数
    if (argc > 1) {
        data_dir = argv[1];
    }
    if (argc > 2) {
        port = std::atoi(argv[2]);
    }
    
    print_banner();
    
    std::cout << "\n📡 初始化存储节点..." << std::endl;
    std::cout << "   数据目录: " << data_dir << std::endl;
    std::cout << "   端口: " << port << std::endl;
    
    try {
        g_node = new StorageNode(data_dir, port);
        
        // 初始化步骤
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "🚀 初始化流程" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        // 步骤 1: 创建数据目录
        std::cout << "\n[1/5] 📁 创建数据目录..." << std::endl;
        if (!g_node->initialize_directories()) {
            std::cerr << "❌ 数据目录创建失败" << std::endl;
            delete g_node;
            return 1;
        }
        
        // 步骤 2: 加载配置
        std::cout << "\n[2/5] ⚙️  加载配置..." << std::endl;
        if (!g_node->load_config()) {
            std::cerr << "❌ 配置加载失败" << std::endl;
            delete g_node;
            return 1;
        }
        
        // 步骤 3: 初始化密码学
        std::cout << "\n[3/5] 🔧 初始化密码学..." << std::endl;
        if (!g_node->setup_cryptography()) {
            std::cerr << "❌ 密码学初始化失败" << std::endl;
            delete g_node;
            return 1;
        }
        
        // 步骤 4: 加载索引数据库
        std::cout << "\n[4/5] 💾 加载索引数据库..." << std::endl;
        if (!g_node->load_index_database()) {
            std::cerr << "❌ 索引数据库加载失败" << std::endl;
            delete g_node;
            return 1;
        }
        
        // 步骤 5: 加载节点信息
        std::cout << "\n[5/5] 📊 加载节点信息..." << std::endl;
        if (!g_node->load_node_info()) {
            std::cerr << "⚠️  节点信息加载失败,将创建新信息" << std::endl;
        }
        
        std::cout << "\n✅ 初始化完成!" << std::endl;
        
        // 显示初始状态
        g_node->print_status();
        
        // 主循环
        while (true) {
            print_menu();
            
            int choice;
            std::cin >> choice;
            
            if (std::cin.fail()) {
                std::cout << "❌ 无效输入,请输入数字 0-9" << std::endl;
                clear_input_buffer();
                wait_for_enter();
                continue;
            }
            
            switch (choice) {
                case 1:
                    handle_insert_file(g_node);
                    break;
                case 2:
                    handle_search_keyword(g_node);
                    break;
                case 3:
                    handle_retrieve_file(g_node);
                    break;
                case 4:
                    handle_delete_file(g_node);
                    break;
                case 5:
                    handle_generate_proof(g_node);
                    break;
                case 6:
                    handle_view_status(g_node);
                    break;
                case 7:
                    handle_list_files(g_node);
                    break;
                case 8:
                    handle_export_metadata(g_node);
                    break;
                case 9:
                    handle_detailed_status(g_node);
                    break;
                case 0:
                    std::cout << "\n👋 再见!" << std::endl;
                    g_node->save_index_database();
                    g_node->save_node_info();
                    delete g_node;
                    return 0;
                default:
                    std::cout << "❌ 无效选项,请选择 0-9" << std::endl;
                    wait_for_enter();
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ 致命错误: " << e.what() << std::endl;
        if (g_node) delete g_node;
        return 1;
    }
    
    return 0;
}