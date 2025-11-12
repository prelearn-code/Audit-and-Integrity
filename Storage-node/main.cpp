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
        g_node->save_search_database();
        g_node->save_node_info();
        delete g_node;
    }
    exit(0);
}

void print_banner() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📦 去中心化存储节点控制台 v3.4" << std::endl;
    std::cout << "   ✨ 新增: 改进的公共参数序列化 (element_to_bytes)" << std::endl;
    std::cout << "   ✨ 特性: 完整的参数恢复，向后兼容旧格式" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
}

void print_menu() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📋 主菜单" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::cout << "\n🔐 密码学管理:" << std::endl;
    std::cout << "  1. 🔧 初始化密码学系统 (Init)" << std::endl;
    std::cout << "  2. 💾 保存公共参数 (Save)" << std::endl;
    std::cout << "  3. 📥 加载公共参数 (Load)" << std::endl;
    std::cout << "  4. 🔑 查看公共参数 (View)" << std::endl;
    
    std::cout << "\n📁 文件操作:" << std::endl;
    std::cout << "  5. 📤 插入文件 (需要JSON参数文件)" << std::endl;
    std::cout << "  6. 🔍 搜索关键词 (需要PK验证)" << std::endl;
    std::cout << "  7. 📥 检索文件" << std::endl;
    std::cout << "  8. 🗑️  删除文件 (需要PK验证)" << std::endl;
    
    std::cout << "\n🔍 查询与管理:" << std::endl;
    std::cout << "  9. 🔐 生成完整性证明" << std::endl;
    std::cout << "  10. 📊 查看节点状态" << std::endl;
    std::cout << "  11. 📋 列出所有文件" << std::endl;
    std::cout << "  12. 💾 导出文件元数据" << std::endl;
    std::cout << "  13. 📄 查看详细状态" << std::endl;
    
    std::cout << "\n  0. 🚪 退出" << std::endl;
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "\n请选择操作 [0-13]: ";
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
    std::cout << "   - TS_F: 文件认证标签数组" << std::endl;
    std::cout << "   - state: 文件状态 (valid/invalid)" << std::endl;
    std::cout << "   - keywords: 关键词数组" << std::endl;
    std::cout << "       └─ Ti_bar: 状态令牌（必需）" << std::endl;
    std::cout << "       └─ kt_wi: 关键词标签（必需）" << std::endl;
    std::cout << "       └─ ptr_i: 指针（可选）" << std::endl;
    
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
    
    std::string pk, search_token, latest_state;
    
    std::cout << "\n请输入客户端公钥 (PK): ";
    clear_input_buffer();
    std::getline(std::cin, pk);
    
    std::cout << "请输入搜索令牌 (T_i): ";
    std::getline(std::cin, search_token);
    
    std::cout << "请输入最新状态 (可选): ";
    std::getline(std::cin, latest_state);
    
    // 修改：删除了 seed 参数的输入
    SearchResult result = node->search_keyword(pk, search_token, latest_state);
    
    std::cout << "\n📊 搜索结果:" << std::endl;
    std::cout << "   找到 " << result.ID_F.size() << " 个匹配文件" << std::endl;
    
    if (!result.ID_F.empty()) {
        std::cout << "\n📄 文件列表:" << std::endl;
        for (size_t i = 0; i < result.ID_F.size(); ++i) {
            std::cout << "   [" << (i+1) << "] " << result.ID_F[i];
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
    
    std::cout << "请输入种子: ";
    std::getline(std::cin, seed);
    
    std::string proof = node->generate_integrity_proof(file_id, seed);
    
    if (!proof.empty()) {
        std::cout << "\n✅ 完整性证明已生成!" << std::endl;
        std::cout << "   证明: " << proof.substr(0, 64) << "..." << std::endl;
    } else {
        std::cout << "\n❌ 证明生成失败!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_view_status(StorageNode* node) {
    node->print_status();
    wait_for_enter();
}

void handle_list_files(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📋 文件列表" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    auto files = node->list_all_files();
    
    if (files.empty()) {
        std::cout << "\n⚠️  暂无文件" << std::endl;
    } else {
        std::cout << "\n共有 " << files.size() << " 个文件:\n" << std::endl;
        int count = 0;
        for (const auto& file_id : files) {
            count++;
            std::cout << "   [" << count << "] " << file_id << std::endl;
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
    
    std::cout << "请输入输出路径: ";
    std::getline(std::cin, output_path);
    
    if (node->export_file_metadata(file_id, output_path)) {
        std::cout << "\n✅ 元数据导出成功!" << std::endl;
        std::cout << "   保存位置: " << output_path << std::endl;
    } else {
        std::cout << "\n❌ 导出失败!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_detailed_status(StorageNode* node) {
    node->print_detailed_status();
    wait_for_enter();
}

void handle_init_crypto(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔧 初始化密码学系统" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::cout << "\n📝 配置参数:" << std::endl;
    std::cout << "   - 安全参数: 512 bits (固定)" << std::endl;
    std::cout << "   - 配对类型: Type A pairing" << std::endl;
    std::cout << "   - 群: G1 = G2 (对称配对)" << std::endl;
    
    std::cout << "\n💡 操作说明:" << std::endl;
    std::cout << "   1. 生成公共参数 PP = {N, g, μ}" << std::endl;
    std::cout << "   2. 可选择是否立即保存参数" << std::endl;
    
    char auto_save;
    std::cout << "\n是否在初始化后自动保存公共参数? (y/n): ";
    std::cin >> auto_save;
    
    std::string save_path;
    if (auto_save == 'y' || auto_save == 'Y') {
        save_path = node->get_data_dir() + "/public_params.json";
    }
    
    std::cout << "\n⏳ 正在初始化密码学系统..." << std::endl;
    std::cout << "   - 初始化配对参数" << std::endl;
    std::cout << "   - 生成群元素 g, μ" << std::endl;
    std::cout << "   - 计算 N = p × q" << std::endl;
    
    if (node->setup_cryptography(512, save_path)) {
        std::cout << "\n✅ 密码学系统初始化成功!" << std::endl;
        std::cout << "\n💡 下一步:" << std::endl;
        if (save_path.empty()) {
            std::cout << "   请选择 '2. 保存公共参数' 以持久化参数" << std::endl;
        } else {
            std::cout << "   公共参数已自动保存，系统可以使用了" << std::endl;
        }
    } else {
        std::cout << "\n❌ 初始化失败!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_save_params(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "💾 保存公共参数" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    if (!node->is_crypto_initialized()) {
        std::cout << "\n❌ 密码学系统未初始化!" << std::endl;
        std::cout << "💡 请先选择 '1. 初始化密码学系统'" << std::endl;
        wait_for_enter();
        return;
    }
    
    std::string default_path = node->get_data_dir() + "/public_params.json";
    
    std::cout << "\n📝 文件路径配置" << std::endl;
    std::cout << "   默认路径: " << default_path << std::endl;
    std::cout << "   直接回车使用默认路径，或输入自定义路径" << std::endl;
    std::cout << "\n请输入保存路径: ";
    
    std::string path;
    clear_input_buffer();
    std::getline(std::cin, path);
    
    if (path.empty()) {
        path = default_path;
        std::cout << "   使用默认路径" << std::endl;
    }
    
    std::cout << "\n💾 保存信息:" << std::endl;
    std::cout << "   目标文件: " << path << std::endl;
    std::cout << "   序列化方法: element_to_bytes (v2.0)" << std::endl;
    std::cout << "   参数内容: N, g, μ" << std::endl;
    std::cout << "\n⏳ 正在保存公共参数..." << std::endl;
    
    // 直接调用save_public_params
    if (node->save_public_params(path)) {
        std::cout << "\n✅ 公共参数保存成功!" << std::endl;
        std::cout << "📄 文件位置: " << path << std::endl;
        std::cout << "\n💡 提示:" << std::endl;
        std::cout << "   - 此文件包含系统的公共参数，可以安全共享" << std::endl;
        std::cout << "   - 下次启动时系统会自动加载此文件" << std::endl;
        std::cout << "   - 建议备份此文件以防丢失" << std::endl;
    } else {
        std::cout << "\n❌ 保存失败!" << std::endl;
        std::cout << "   请检查文件路径和写入权限" << std::endl;
    }
    
    wait_for_enter();
}

void handle_load_params(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📥 加载公共参数" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::string default_path = node->get_data_dir() + "/public_params.json";
    
    std::cout << "\n📝 文件路径配置" << std::endl;
    std::cout << "   默认路径: " << default_path << std::endl;
    std::cout << "   直接回车使用默认路径，或输入自定义路径" << std::endl;
    std::cout << "\n请输入加载路径: ";
    
    std::string path;
    clear_input_buffer();
    std::getline(std::cin, path);
    
    if (path.empty()) {
        path = default_path;
        std::cout << "   使用默认路径" << std::endl;
    }
    
    std::cout << "\n📥 加载信息:" << std::endl;
    std::cout << "   源文件: " << path << std::endl;
    std::cout << "\n⏳ 正在加载公共参数..." << std::endl;
    std::cout << "   - 读取 JSON 配置文件" << std::endl;
    std::cout << "   - 初始化配对系统" << std::endl;
    std::cout << "   - 恢复参数 N, g, μ" << std::endl;
    
    // 直接调用load_public_params（会自动显示详细信息+初始化）
    if (node->load_public_params(path)) {
        std::cout << "\n✅ 公共参数加载成功，密码学系统已就绪!" << std::endl;
        std::cout << "\n💡 系统状态:" << std::endl;
        std::cout << "   - 密码学系统: 已初始化 ✓" << std::endl;
        std::cout << "   - 可以开始文件操作" << std::endl;
    } else {
        std::cout << "\n❌ 加载失败!" << std::endl;
        std::cout << "\n🔍 可能的原因:" << std::endl;
        std::cout << "   - 文件不存在或路径错误" << std::endl;
        std::cout << "   - JSON 格式错误" << std::endl;
        std::cout << "   - 参数数据损坏" << std::endl;
        std::cout << "\n💡 建议:" << std::endl;
        std::cout << "   如果是首次使用，请先选择 '1. 初始化密码学系统'" << std::endl;
    }
    
    wait_for_enter();
}

void handle_view_public_params(StorageNode* node) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔑 查看公共参数" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::cout << "\n📝 查看选项:" << std::endl;
    std::cout << "   1. 从文件读取并查看" << std::endl;
    std::cout << "   2. 查看内存中的参数 (需要已初始化)" << std::endl;
    
    int choice;
    std::cout << "\n请选择 (1/2): ";
    std::cin >> choice;
    
    if (choice == 1) {
        std::string default_path = node->get_data_dir() + "/public_params.json";
        std::cout << "\n默认路径: " << default_path << std::endl;
        std::cout << "直接回车使用默认路径，或输入自定义路径" << std::endl;
        std::cout << "请输入文件路径: ";
        
        std::string path;
        clear_input_buffer();
        std::getline(std::cin, path);
        
        if (path.empty()) {
            path = default_path;
        }
        
        node->display_public_params(path);
    } else if (choice == 2) {
        node->display_public_params("");  // 空路径表示显示内存中的参数
    } else {
        std::cout << "❌ 无效选择" << std::endl;
    }
    
    wait_for_enter();
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    // 默认参数
    std::string data_dir = "../data";
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
        std::cout << "\n[1/4] 📁 创建数据目录..." << std::endl;
        if (!g_node->initialize_directories()) {
            std::cerr << "❌ 数据目录创建失败" << std::endl;
            delete g_node;
            return 1;
        }
        
        // 步骤 2: 加载配置
        std::cout << "\n[2/4] ⚙️  加载配置..." << std::endl;
        if (!g_node->load_config()) {
            std::cerr << "❌ 配置加载失败" << std::endl;
            delete g_node;
            return 1;
        }
        
        // 步骤 3: 智能检测公共参数
        std::cout << "\n[3/4] 🔍 检测密码学系统..." << std::endl;
        std::string public_params_path = g_node->get_data_dir() + "/public_params.json";
        
        if (g_node->has_public_params_file(public_params_path)) {
            std::cout << "✅ 发现公共参数文件: " << public_params_path << std::endl;
            std::cout << "⏳ 自动加载公共参数..." << std::endl;
            if (g_node->load_public_params(public_params_path)) {
                std::cout << "✅ 密码学系统已就绪 (从文件恢复)" << std::endl;
            } else {
                std::cout << "⚠️  加载失败，密码学系统未初始化" << std::endl;
                std::cout << "💡 提示: 请在菜单中选择 '1. 初始化密码学系统'" << std::endl;
            }
        } else {
            std::cout << "⚠️  未找到公共参数文件" << std::endl;
            std::cout << "💡 首次使用指南:" << std::endl;
            std::cout << "   1. 选择菜单选项 '1. 初始化密码学系统'" << std::endl;
            std::cout << "   2. 选择菜单选项 '2. 保存公共参数'" << std::endl;
            std::cout << "   3. 下次启动时会自动加载参数" << std::endl;
        }
        
        // 步骤 4: 加载索引数据库
        std::cout << "\n[4/5] 💾 加载索引数据库..." << std::endl;
        if (!g_node->load_index_database()) {
            std::cerr << "❌ 索引数据库加载失败" << std::endl;
            delete g_node;
            return 1;
        }
        
        // 步骤 5: 加载搜索数据库
        std::cout << "\n[5/5] 🔍 加载搜索数据库..." << std::endl;
        if (!g_node->load_search_database()) {
            std::cerr << "⚠️  搜索数据库加载失败，已创建新数据库" << std::endl;
            // 注意：这里不退出，因为可以创建新的搜索数据库
        }
        
        // 加载节点信息
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
                std::cout << "❌ 无效输入,请输入数字 0-13" << std::endl;
                clear_input_buffer();
                wait_for_enter();
                continue;
            }
            
            switch (choice) {
                case 1:
                    handle_init_crypto(g_node);
                    break;
                case 2:
                    handle_save_params(g_node);
                    break;
                case 3:
                    handle_load_params(g_node);
                    break;
                case 4:
                    handle_view_public_params(g_node);
                    break;
                case 5:
                    handle_insert_file(g_node);
                    break;
                case 6:
                    handle_search_keyword(g_node);
                    break;
                case 7:
                    handle_retrieve_file(g_node);
                    break;
                case 8:
                    handle_delete_file(g_node);
                    break;
                case 9:
                    handle_generate_proof(g_node);
                    break;
                case 10:
                    handle_view_status(g_node);
                    break;
                case 11:
                    handle_list_files(g_node);
                    break;
                case 12:
                    handle_export_metadata(g_node);
                    break;
                case 13:
                    handle_detailed_status(g_node);
                    break;
                case 0:
                    std::cout << "\n👋 再见!" << std::endl;
                    g_node->save_index_database();
                    g_node->save_search_database();
                    g_node->save_node_info();
                    delete g_node;
                    return 0;
                default:
                    std::cout << "❌ 无效选项,请选择 0-13" << std::endl;
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