#include "storage_node.h"
#include <iostream>
#include <csignal>
#include <cstring>
#include <limits>
#include <iomanip>

StorageNode* g_node = nullptr;

// ============================================================================
// 信号处理和程序控制
// ============================================================================

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

// ============================================================================
// 界面显示函数
// ============================================================================

void print_banner() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║      📦 去中心化存储节点控制台 v3.5                      ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    std::cout << "║  ✨ 新增: 文件删除功能 (delete_file_from_json)          ║" << std::endl;
    std::cout << "║  ✨ 新增: 搜索关键词关联文件证明                         ║" << std::endl;
    std::cout << "║  ✨ 改进: 哈希函数支持文件分块处理                       ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
}

void print_menu() {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                      📋 主菜单                            ║" << std::endl;
    std::cout << "╠══════════════════════════════════════════════════════════╣" << std::endl;
    
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "║  🔐 密码学管理                                            ║" << std::endl;
    std::cout << "║     1  初始化密码学系统                                  ║" << std::endl;
    std::cout << "║     2  保存公共参数                                      ║" << std::endl;
    std::cout << "║     3  加载公共参数                                      ║" << std::endl;
    std::cout << "║     4  查看公共参数                                      ║" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "║  📁 文件操作                                              ║" << std::endl;
    std::cout << "║     5  插入文件 (需要JSON参数)                           ║" << std::endl;
    std::cout << "║     6  检索文件                                          ║" << std::endl;
    std::cout << "║     7  删除文件 (从JSON)                                 ║" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "║  🔍 搜索功能                                              ║" << std::endl;
    std::cout << "║     8  搜索关键词关联文件证明 (完整搜索)                 ║" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "║  🔐 证明与验证                                            ║" << std::endl;
    std::cout << "║     9  获取文件证明 (待实现)                            ║" << std::endl;
    std::cout << "║     10 验证搜索证明 (待实现)                            ║" << std::endl;
    std::cout << "║     11 验证文件证明 (待实现)                            ║" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "║  📊 查询与管理                                            ║" << std::endl;
    std::cout << "║     12 查看节点状态                                     ║" << std::endl;
    std::cout << "║     13 列出所有文件                                     ║" << std::endl;
    std::cout << "║     14 导出文件元数据                                   ║" << std::endl;
    std::cout << "║     15 查看详细状态                                     ║" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "║     0  退出程序                                          ║" << std::endl;
    std::cout << "║                                                          ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n👉 请输入选项 [0-15]: ";
}

// ============================================================================
// 辅助函数
// ============================================================================

void clear_input_buffer() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void wait_for_enter() {
    std::cout << "\n⏎ 按 Enter 继续...";
    clear_input_buffer();
    std::cin.get();
}

void print_section_header(const std::string& title, const std::string& icon = "📋") {
    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  " << icon << " " << title;
    // 补充空格以对齐右边框
    int padding = 54 - title.length() - icon.length();
    for (int i = 0; i < padding; i++) std::cout << " ";
    std::cout << "║" << std::endl;
    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
}

// ============================================================================
// 密码学管理处理函数
// ============================================================================

void handle_init_crypto(StorageNode* node) {
    print_section_header("初始化密码学系统", "🔧");
    
    int security_param;
    std::cout << "\n🔒 请输入安全参数 (推荐: 256 或 512): ";
    std::cin >> security_param;
    
    std::cout << "\n⚙️  正在初始化密码学系统..." << std::endl;
    std::cout << "   安全参数: " << security_param << " bits" << std::endl;
    std::cout << "   这可能需要几秒钟时间..." << std::endl;
    
    if (node->setup_cryptography(security_param, "")) {
        std::cout << "\n✅ 密码学系统初始化成功!" << std::endl;
        std::cout << "\n💡 重要提示:" << std::endl;
        std::cout << "   ├─ 密码学系统已在内存中初始化" << std::endl;
        std::cout << "   ├─ 建议立即执行 '2. 保存公共参数' 进行持久化" << std::endl;
        std::cout << "   └─ 这样下次启动时可以自动加载参数" << std::endl;
    } else {
        std::cout << "\n❌ 初始化失败!" << std::endl;
        std::cout << "\n🔍 可能的原因:" << std::endl;
        std::cout << "   ├─ 系统资源不足" << std::endl;
        std::cout << "   ├─ 密码学库未正确安装" << std::endl;
        std::cout << "   └─ 内存分配失败" << std::endl;
    }
    
    wait_for_enter();
}

void handle_save_params(StorageNode* node) {
    print_section_header("保存公共参数", "💾");
    
    std::string default_path = node->get_data_dir() + "/public_params.json";
    std::cout << "\n📂 默认路径: " << default_path << std::endl;
    std::cout << "   直接按 Enter 使用默认路径，或输入自定义路径" << std::endl;
    std::cout << "\n请输入保存路径: ";
    
    std::string path;
    clear_input_buffer();
    std::getline(std::cin, path);
    
    if (path.empty()) {
        path = default_path;
    }
    
    std::cout << "\n💾 正在保存到: " << path << std::endl;
    
    if (node->save_public_params(path)) {
        std::cout << "\n✅ 公共参数保存成功!" << std::endl;
        std::cout << "\n📝 文件信息:" << std::endl;
        std::cout << "   ├─ 保存路径: " << path << std::endl;
        std::cout << "   ├─ 格式: JSON" << std::endl;
        std::cout << "   └─ 下次启动时将自动加载" << std::endl;
    } else {
        std::cout << "\n❌ 保存失败!" << std::endl;
        std::cout << "\n🔍 可能的原因:" << std::endl;
        std::cout << "   ├─ 密码学系统未初始化" << std::endl;
        std::cout << "   ├─ 文件路径不存在" << std::endl;
        std::cout << "   └─ 没有写入权限" << std::endl;
        std::cout << "\n💡 建议: 请先执行 '1. 初始化密码学系统'" << std::endl;
    }
    
    wait_for_enter();
}

void handle_load_params(StorageNode* node) {
    print_section_header("加载公共参数", "📥");
    
    std::string default_path = node->get_data_dir() + "/public_params.json";
    std::cout << "\n📂 默认路径: " << default_path << std::endl;
    std::cout << "   直接按 Enter 使用默认路径，或输入自定义路径" << std::endl;
    std::cout << "\n请输入文件路径: ";
    
    std::string path;
    clear_input_buffer();
    std::getline(std::cin, path);
    
    if (path.empty()) {
        path = default_path;
    }
    
    std::cout << "\n📥 正在加载: " << path << std::endl;
    
    if (node->load_public_params(path)) {
        std::cout << "\n✅ 公共参数加载成功!" << std::endl;
        std::cout << "\n💡 系统状态:" << std::endl;
        std::cout << "   ├─ 密码学系统: 已初始化 ✓" << std::endl;
        std::cout << "   └─ 可以开始文件操作" << std::endl;
    } else {
        std::cout << "\n❌ 加载失败!" << std::endl;
        std::cout << "\n🔍 可能的原因:" << std::endl;
        std::cout << "   ├─ 文件不存在或路径错误" << std::endl;
        std::cout << "   ├─ JSON 格式错误" << std::endl;
        std::cout << "   └─ 参数数据损坏" << std::endl;
        std::cout << "\n💡 建议:" << std::endl;
        std::cout << "   如果是首次使用，请先选择 '1. 初始化密码学系统'" << std::endl;
    }
    
    wait_for_enter();
}

void handle_view_public_params(StorageNode* node) {
    print_section_header("查看公共参数", "🔑");
    
    std::cout << "\n📝 查看选项:" << std::endl;
    std::cout << "   1️⃣  从文件读取并查看" << std::endl;
    std::cout << "   2️⃣  查看内存中的参数 (需要已初始化)" << std::endl;
    
    int choice;
    std::cout << "\n请选择 (1/2): ";
    std::cin >> choice;
    
    if (choice == 1) {
        std::string default_path = node->get_data_dir() + "/public_params.json";
        std::cout << "\n📂 默认路径: " << default_path << std::endl;
        std::cout << "   直接按 Enter 使用默认路径，或输入自定义路径" << std::endl;
        std::cout << "\n请输入文件路径: ";
        
        std::string path;
        clear_input_buffer();
        std::getline(std::cin, path);
        
        if (path.empty()) {
            path = default_path;
        }
        
        node->display_public_params(path);
    } else if (choice == 2) {
        node->display_public_params("");
    } else {
        std::cout << "\n❌ 无效选择" << std::endl;
    }
    
    wait_for_enter();
}

// ============================================================================
// 文件操作处理函数
// ============================================================================

void handle_insert_file(StorageNode* node) {
    print_section_header("插入文件", "📤");
    
    std::string param_json_path, enc_file_path;
    
    std::cout << "\n💡 JSON参数文件格式说明:" << std::endl;
    std::cout << "   ├─ PK: 客户端公钥" << std::endl;
    std::cout << "   ├─ ID_F: 文件唯一标识" << std::endl;
    std::cout << "   ├─ TS_F: 文件认证标签数组" << std::endl;
    std::cout << "   ├─ state: 文件状态 (valid/invalid)" << std::endl;
    std::cout << "   └─ keywords: 关键词数组" << std::endl;
    std::cout << "       ├─ Ti_bar: 状态令牌（必需）" << std::endl;
    std::cout << "       ├─ kt_wi: 关键词标签（必需）" << std::endl;
    std::cout << "       └─ ptr_i: 指针（可选）" << std::endl;
    
    std::cout << "\n📂 请输入参数JSON文件路径: ";
    clear_input_buffer();
    std::getline(std::cin, param_json_path);
    
    std::cout << "📂 请输入加密文件路径: ";
    std::getline(std::cin, enc_file_path);
    
    std::cout << "\n⏳ 正在插入文件..." << std::endl;
    
    if (node->insert_file(param_json_path, enc_file_path)) {
        std::cout << "\n✅ 文件插入成功!" << std::endl;
        std::cout << "   ├─ 文件已存储" << std::endl;
        std::cout << "   ├─ 索引已更新" << std::endl;
        std::cout << "   └─ 关键词已建立关联" << std::endl;
    } else {
        std::cout << "\n❌ 文件插入失败!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_retrieve_file(StorageNode* node) {
    print_section_header("检索文件", "📥");
    
    std::string file_id;
    
    std::cout << "\n🔖 请输入文件ID: ";
    clear_input_buffer();
    std::getline(std::cin, file_id);
    
    std::cout << "\n🔍 正在检索文件..." << std::endl;
    Json::Value result = node->retrieve_file(file_id);
    
    if (result["success"].asBool()) {
        std::cout << "\n✅ 文件检索成功!" << std::endl;
        std::cout << "\n📋 文件信息:" << std::endl;
        std::cout << "   ├─ 文件ID:     " << result["file_id"].asString() << std::endl;
        std::cout << "   ├─ 客户端PK:   " << result["PK"].asString().substr(0, 16) << "..." << std::endl;
        std::cout << "   ├─ 密文大小:   " << result["ciphertext"].asString().length() << " 字节" << std::endl;
        std::cout << "   └─ 状态:       " << result["state"].asString() << std::endl;
        
        if (result.isMember("pointer")) {
            std::cout << "   ├─ 指针:       " << result["pointer"].asString().substr(0, 32) << "..." << std::endl;
        }
        if (result.isMember("file_auth_tag")) {
            std::cout << "   └─ 认证标签:   " << result["file_auth_tag"].asString().substr(0, 32) << "..." << std::endl;
        }
        
        char save_choice;
        std::cout << "\n💾 是否保存密文到文件? (y/n): ";
        std::cin >> save_choice;
        
        if (save_choice == 'y' || save_choice == 'Y') {
            std::string output_path;
            std::cout << "📂 输出文件路径: ";
            clear_input_buffer();
            std::getline(std::cin, output_path);
            
            std::ofstream outfile(output_path, std::ios::binary);
            if (outfile.is_open()) {
                outfile << result["ciphertext"].asString();
                outfile.close();
                std::cout << "\n✅ 密文已保存到: " << output_path << std::endl;
            } else {
                std::cout << "\n❌ 无法保存文件" << std::endl;
            }
        }
    } else {
        std::cout << "\n❌ 文件不存在!" << std::endl;
    }
    
    wait_for_enter();
}

void handle_delete_file_from_json(StorageNode* node) {
    print_section_header("删除文件", "🗑️");
    
    std::string json_path;
    
    std::cout << "\n💡 JSON文件格式说明:" << std::endl;
    std::cout << "   ├─ ID_F: 文件唯一标识" << std::endl;
    std::cout << "   ├─ PK: 客户端公钥" << std::endl;
    std::cout << "   └─ del: 删除证明" << std::endl;
    
    std::cout << "\n📂 请输入删除参数JSON文件路径: ";
    clear_input_buffer();
    std::getline(std::cin, json_path);
    
    std::cout << "\n⚠️  警告: 此操作将标记文件为无效并更新所有相关索引!" << std::endl;
    char confirm;
    std::cout << "❓ 确认删除? (y/n): ";
    std::cin >> confirm;
    
    if (confirm == 'y' || confirm == 'Y') {
        std::cout << "\n⏳ 正在删除文件..." << std::endl;
        if (node->delete_file_from_json(json_path)) {
            std::cout << "\n✅ 文件删除成功!" << std::endl;
            std::cout << "   ├─ 文件已标记为无效" << std::endl;
            std::cout << "   └─ 索引已更新" << std::endl;
        } else {
            std::cout << "\n❌ 删除操作失败!" << std::endl;
        }
    } else {
        std::cout << "\n🚫 操作已取消" << std::endl;
    }
    
    wait_for_enter();
}

// ============================================================================
// 搜索功能处理函数
// ============================================================================

void handle_search_keywords_proof(StorageNode* node) {
    print_section_header("搜索关键词关联文件证明 (完整)", "🔍");
    
    std::string json_path;
    
    std::cout << "\n💡 JSON文件格式说明:" << std::endl;
    std::cout << "   ├─ PK: 客户端公钥" << std::endl;
    std::cout << "   ├─ T: 搜索令牌" << std::endl;
    std::cout << "   └─ std: 最新状态" << std::endl;
    
    std::cout << "\n📂 请输入搜索参数JSON文件路径: ";
    clear_input_buffer();
    std::getline(std::cin, json_path);
    
    std::cout << "\n🔍 正在搜索并生成证明..." << std::endl;
    
    if (node->SearchKeywordsAssociatedFilesProof(json_path)) {
        std::cout << "\n✅ 搜索完成并已生成证明!" << std::endl;
        std::cout << "   ├─ 已找到匹配的文件" << std::endl;
        std::cout << "   ├─ 证明已生成" << std::endl;
        std::cout << "   └─ 结果已保存" << std::endl;
    } else {
        std::cout << "\n❌ 搜索失败!" << std::endl;
    }
    
    wait_for_enter();
}

// ============================================================================
// 证明与验证处理函数
// ============================================================================

void handle_get_file_proof(StorageNode* node) {
    print_section_header("获取文件证明", "📄");
    
    std::string json_path;
    
    std::cout << "\n💡 JSON文件格式说明:" << std::endl;
    std::cout << "   ├─ file_id: 文件标识" << std::endl;
    std::cout << "   └─ proof_type: 证明类型" << std::endl;
    
    std::cout << "\n📂 请输入文件证明参数JSON文件路径: ";
    clear_input_buffer();
    std::getline(std::cin, json_path);
    
    std::cout << "\n⏳ 正在获取文件证明..." << std::endl;
    
    if (node->GetFileProof(json_path)) {
        std::cout << "\n✅ 文件证明获取成功!" << std::endl;
    } else {
        std::cout << "\n⚠️  此功能正在开发中..." << std::endl;
        std::cout << "\n💡 即将支持:" << std::endl;
        std::cout << "   ├─ 获取单个文件的存在性证明" << std::endl;
        std::cout << "   ├─ 生成文件所有权证明" << std::endl;
        std::cout << "   └─ 导出文件证明数据" << std::endl;
    }
    
    wait_for_enter();
}

void handle_verify_search_proof(StorageNode* node) {
    print_section_header("验证搜索证明", "✅");
    
    std::string json_path;
    
    std::cout << "\n💡 JSON文件格式说明:" << std::endl;
    std::cout << "   ├─ proof: 搜索证明数据" << std::endl;
    std::cout << "   ├─ search_token: 搜索令牌" << std::endl;
    std::cout << "   └─ result: 搜索结果" << std::endl;
    
    std::cout << "\n📂 请输入搜索证明JSON文件路径: ";
    clear_input_buffer();
    std::getline(std::cin, json_path);
    
    std::cout << "\n⏳ 正在验证搜索证明..." << std::endl;
    
    if (node->VerifySearchProof(json_path)) {
        std::cout << "\n✅ 搜索证明验证成功!" << std::endl;
    } else {
        std::cout << "\n⚠️  此功能正在开发中..." << std::endl;
        std::cout << "\n💡 即将支持:" << std::endl;
        std::cout << "   ├─ 验证搜索结果的正确性" << std::endl;
        std::cout << "   ├─ 检查关键词关联的完整性" << std::endl;
        std::cout << "   └─ 确认搜索证明的有效性" << std::endl;
    }
    
    wait_for_enter();
}

void handle_verify_file_proof(StorageNode* node) {
    print_section_header("验证文件证明", "✅");
    
    std::string json_path;
    
    std::cout << "\n💡 JSON文件格式说明:" << std::endl;
    std::cout << "   ├─ proof: 文件证明数据" << std::endl;
    std::cout << "   ├─ file_id: 文件标识" << std::endl;
    std::cout << "   └─ metadata: 文件元数据" << std::endl;
    
    std::cout << "\n📂 请输入文件证明JSON文件路径: ";
    clear_input_buffer();
    std::getline(std::cin, json_path);
    
    std::cout << "\n⏳ 正在验证文件证明..." << std::endl;
    
    if (node->VerifyFileProof(json_path)) {
        std::cout << "\n✅ 文件证明验证成功!" << std::endl;
    } else {
        std::cout << "\n⚠️  此功能正在开发中..." << std::endl;
        std::cout << "\n💡 即将支持:" << std::endl;
        std::cout << "   ├─ 验证文件存在性证明" << std::endl;
        std::cout << "   ├─ 检查文件完整性证明" << std::endl;
        std::cout << "   └─ 确认文件所有权证明" << std::endl;
    }
    
    wait_for_enter();
}

// ============================================================================
// 查询与管理处理函数
// ============================================================================

void handle_view_status(StorageNode* node) {
    print_section_header("节点状态", "📊");
    
    std::cout << "\n";
    node->print_status();
    
    wait_for_enter();
}

void handle_list_files(StorageNode* node) {
    print_section_header("文件列表", "📋");
    
    std::vector<std::string> file_list = node->list_all_files();
    
    if (file_list.empty()) {
        std::cout << "\n📭 暂无文件" << std::endl;
    } else {
        std::cout << "\n📁 共有 " << file_list.size() << " 个文件:\n" << std::endl;
        
        int index = 1;
        for (const auto& file_id : file_list) {
            std::cout << "   " << std::setw(3) << index++ << ". " << file_id << std::endl;
        }
        
        std::cout << "\n💡 提示: 使用选项 6 可以检索单个文件的详细信息" << std::endl;
    }
    
    wait_for_enter();
}

void handle_export_metadata(StorageNode* node) {
    print_section_header("导出文件元数据", "💾");
    
    std::string file_id, output_path;
    
    std::cout << "\n🔖 请输入文件ID: ";
    clear_input_buffer();
    std::getline(std::cin, file_id);
    
    std::string default_path = node->get_data_dir() + "/metadata_" + file_id + ".json";
    std::cout << "\n📂 默认路径: " << default_path << std::endl;
    std::cout << "   直接按 Enter 使用默认路径，或输入自定义路径" << std::endl;
    std::cout << "\n请输入导出路径: ";
    
    std::getline(std::cin, output_path);
    
    if (output_path.empty()) {
        output_path = default_path;
    }
    
    std::cout << "\n⏳ 正在导出元数据..." << std::endl;
    std::cout << "   ├─ 文件ID:   " << file_id << std::endl;
    std::cout << "   └─ 输出路径: " << output_path << std::endl;
    
    if (node->export_file_metadata(file_id, output_path)) {
        std::cout << "\n✅ 元数据导出成功!" << std::endl;
        std::cout << "   └─ 保存路径: " << output_path << std::endl;
    } else {
        std::cout << "\n❌ 导出失败!" << std::endl;
        std::cout << "   └─ 请检查文件ID是否存在" << std::endl;
    }
    
    wait_for_enter();
}

void handle_detailed_status(StorageNode* node) {
    print_section_header("详细状态", "📄");
    
    std::cout << "\n";
    node->print_detailed_status();
    
    wait_for_enter();
}

// ============================================================================
// 主程序
// ============================================================================

int main(int argc, char* argv[]) {
    // 注册信号处理器
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
    
    // 显示欢迎横幅
    print_banner();
    
    std::cout << "\n📡 启动信息" << std::endl;
    std::cout << "   ├─ 数据目录: " << data_dir << std::endl;
    std::cout << "   └─ 端口:     " << port << std::endl;
    
    try {
        g_node = new StorageNode(data_dir, port);
        
        // ========================================
        // 初始化流程
        // ========================================
        
        std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                    🚀 初始化流程                          ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
        
        // 步骤 1: 创建数据目录
        std::cout << "\n[1/5] 📁 创建数据目录..." << std::endl;
        if (!g_node->initialize_directories()) {
            std::cerr << "   └─ ❌ 数据目录创建失败" << std::endl;
            delete g_node;
            return 1;
        }
        std::cout << "   └─ ✅ 完成" << std::endl;
        
        // 步骤 2: 加载配置
        std::cout << "\n[2/5] ⚙️  加载配置文件..." << std::endl;
        if (!g_node->load_config()) {
            std::cerr << "   └─ ❌ 配置加载失败" << std::endl;
            delete g_node;
            return 1;
        }
        std::cout << "   └─ ✅ 完成" << std::endl;
        
        // 步骤 3: 智能检测并加载密码学参数
        std::cout << "\n[3/5] 🔍 检测密码学系统..." << std::endl;
        std::string public_params_path = g_node->get_data_dir() + "/public_params.json";
        
        if (g_node->has_public_params_file(public_params_path)) {
            std::cout << "   ├─ ✅ 发现公共参数文件" << std::endl;
            std::cout << "   ├─ ⏳ 正在自动加载..." << std::endl;
            if (g_node->load_public_params(public_params_path)) {
                std::cout << "   └─ ✅ 密码学系统已就绪" << std::endl;
            } else {
                std::cout << "   ├─ ⚠️  加载失败，密码学系统未初始化" << std::endl;
                std::cout << "   └─ 💡 请在菜单中选择 '1️⃣ 初始化密码学系统'" << std::endl;
            }
        } else {
            std::cout << "   ├─ ⚠️  未找到公共参数文件" << std::endl;
            std::cout << "   └─ 💡 首次使用指南:" << std::endl;
            std::cout << "       ├─ 选择 '1️⃣ 初始化密码学系统'" << std::endl;
            std::cout << "       ├─ 选择 '2️⃣ 保存公共参数'" << std::endl;
            std::cout << "       └─ 下次启动时会自动加载" << std::endl;
        }
        
        // 步骤 4: 加载索引数据库
        std::cout << "\n[4/5] 💾 加载索引数据库..." << std::endl;
        if (!g_node->load_index_database()) {
            std::cerr << "   └─ ❌ 索引数据库加载失败" << std::endl;
            delete g_node;
            return 1;
        }
        std::cout << "   └─ ✅ 完成" << std::endl;
        
        // 步骤 5: 加载搜索数据库
        std::cout << "\n[5/5] 🔍 加载搜索数据库..." << std::endl;
        if (!g_node->load_search_database()) {
            std::cout << "   └─ ⚠️  已创建新数据库" << std::endl;
        } else {
            std::cout << "   └─ ✅ 完成" << std::endl;
        }
        
        // 加载节点信息
        if (!g_node->load_node_info()) {
            std::cout << "\n⚠️  节点信息加载失败，将创建新信息" << std::endl;
        }
        
        std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║              ✅ 初始化完成，系统就绪!                     ║" << std::endl;
        std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
        
        // 显示初始状态
        g_node->print_status();
        
        // ========================================
        // 主菜单循环
        // ========================================
        
        while (true) {
            print_menu();
            
            int choice;
            std::cin >> choice;
            
            if (std::cin.fail()) {
                std::cout << "\n❌ 输入无效，请输入数字 0-17" << std::endl;
                clear_input_buffer();
                wait_for_enter();
                continue;
            }
            
            switch (choice) {
                // 密码学管理
                case 1:  handle_init_crypto(g_node);              break;
                case 2:  handle_save_params(g_node);              break;
                case 3:  handle_load_params(g_node);              break;
                case 4:  handle_view_public_params(g_node);       break;
                
                // 文件操作
                case 5:  handle_insert_file(g_node);              break;
                case 6:  handle_retrieve_file(g_node);            break;
                case 7:  handle_delete_file_from_json(g_node);    break;
                
                // 搜索功能
                case 8:  handle_search_keywords_proof(g_node);    break;
                
                // 证明与验证
                case 9: handle_get_file_proof(g_node);           break;
                case 10: handle_verify_search_proof(g_node);      break;
                case 11: handle_verify_file_proof(g_node);        break;
                
                // 查询与管理
                case 12: handle_view_status(g_node);              break;
                case 13: handle_list_files(g_node);               break;
                case 14: handle_export_metadata(g_node);          break;
                case 15: handle_detailed_status(g_node);          break;
                
                // 退出
                case 0:
                    std::cout << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
                    std::cout << "║                 👋 感谢使用，再见!                        ║" << std::endl;
                    std::cout << "╚══════════════════════════════════════════════════════════╝" << std::endl;
                    std::cout << "\n💾 正在保存数据..." << std::endl;
                    g_node->save_index_database();
                    g_node->save_search_database();
                    g_node->save_node_info();
                    std::cout << "✅ 数据已保存" << std::endl;
                    delete g_node;
                    return 0;
                
                default:
                    std::cout << "\n❌ 无效选项，请选择 0-17" << std::endl;
                    wait_for_enter();
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "\n╔══════════════════════════════════════════════════════════╗" << std::endl;
        std::cerr << "║                   ❌ 致命错误                             ║" << std::endl;
        std::cerr << "╚══════════════════════════════════════════════════════════╝" << std::endl;
        std::cerr << "\n错误信息: " << e.what() << std::endl;
        if (g_node) delete g_node;
        return 1;
    }
    
    return 0;
}