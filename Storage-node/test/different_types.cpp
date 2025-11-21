/*
 * test_file_proof.cpp - 文件证明生成与验证简单测试
 * 
 * 功能: 测试 GetFileProof() 和 VerifyFileProof()
 * 前提: 已有完整的 ../data 目录环境
 * 
 * 编译: make test_file_proof
 * 运行: ./test_file_proof
 */

#include "storage_node.h"
#include <iostream>

int main() {
    
    // ========== 步骤1: 初始化 ==========
    std::cout << "🧪 文件证明测试程序\n" << std::endl;
    
    std::string data_dir = "../data";
    StorageNode* node = new StorageNode(data_dir, 9000);
    
    // 初始化目录
    if (!node->initialize_directories()) {
        std::cerr << "❌ 目录初始化失败" << std::endl;
        delete node;
        return 1;
    }
    
    
    // ========== 步骤2: 加载公共参数 ==========
    std::cout << "📥 加载公共参数..." << std::endl;
    
    std::string public_params_path = data_dir + "/public_params.json";
    if (!node->load_public_params(public_params_path)) {
        std::cerr << "❌ 公共参数加载失败" << std::endl;
        delete node;
        return 1;
    }
    
    std::cout << "✅ 公共参数加载成功\n" << std::endl;
    
    
    // ========== 步骤3: 加载索引数据库 ==========
    std::cout << "📥 加载索引数据库..." << std::endl;
    
    if (!node->load_index_database()) {
        std::cerr << "❌ 索引数据库加载失败" << std::endl;
        delete node;
        return 1;
    }
    
    std::cout << "✅ 索引数据库加载成功" << std::endl;
    std::cout << "📊 文件总数: " << node->get_file_count() << "\n" << std::endl;
    
    
    // ========== 步骤4: 选择测试文件 ==========
    std::cout << "📝 请输入要测试的文件ID: ";
    std::string test_file_id;
    std::getline(std::cin, test_file_id);
    
    // 验证文件是否存在
    if (!node->has_file(test_file_id)) {
        std::cerr << "❌ 文件不存在: " << test_file_id << std::endl;
        
        // 提示可用的文件列表
        std::vector<std::string> files = node->list_all_files();
        if (!files.empty()) {
            std::cout << "\n💡 可用的文件ID列表:" << std::endl;
            for (size_t i = 0; i < files.size() && i < 5; i++) {
                std::cout << "   - " << files[i] << std::endl;
            }
            if (files.size() > 5) {
                std::cout << "   ... 还有 " << (files.size() - 5) << " 个文件" << std::endl;
            }
        }
        
        delete node;
        return 1;
    }
    
    std::cout << "✅ 找到文件: " << test_file_id << "\n" << std::endl;
    
    
    // ========== 步骤5: 生成文件证明 ==========
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔨 测试1: 生成文件证明" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    bool proof_generated = node->GetFileProof(test_file_id);
    
    if (proof_generated) {
        std::cout << "\n✅ 文件证明生成成功!" << std::endl;
        
        // 显示生成的证明文件路径
        std::string proof_file_path = data_dir + "/FileProofs/" + test_file_id + ".json";
        std::cout << "📄 证明文件: " << proof_file_path << std::endl;
        
        // 读取并显示证明文件的关键信息
        Json::Value proof_data = node->load_json_from_file(proof_file_path);
        if (proof_data.isMember("FileProof")) {
            std::cout << "\n📊 证明详情:" << std::endl;
            std::cout << "   - ID_F: " << proof_data["ID_F"].asString() << std::endl;
            std::cout << "   - psi 长度: " << proof_data["FileProof"]["psi"].asString().length() << " 字符" << std::endl;
            std::cout << "   - phi 长度: " << proof_data["FileProof"]["phi"].asString().length() << " 字符" << std::endl;
            std::cout << "   - seed 长度: " << proof_data["seed"].asString().length() << " 字符" << std::endl;
            
            // 显示前几个字符用于调试
            std::string psi = proof_data["FileProof"]["psi"].asString();
            std::string phi = proof_data["FileProof"]["phi"].asString();
            std::string seed = proof_data["seed"].asString();
            
            std::cout << "\n🔍 数据预览:" << std::endl;
            std::cout << "   - psi  (前32位): " << psi.substr(0, std::min(32, (int)psi.length())) << "..." << std::endl;
            std::cout << "   - phi  (前32位): " << phi.substr(0, std::min(32, (int)phi.length())) << "..." << std::endl;
            std::cout << "   - seed (前32位): " << seed.substr(0, std::min(32, (int)seed.length())) << "..." << std::endl;
        }
        
    } else {
        std::cerr << "\n❌ 文件证明生成失败!" << std::endl;
        delete node;
        return 1;
    }
    
    
    // ========== 步骤6: 验证文件证明 ==========
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔍 测试2: 验证文件证明" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    std::string proof_file_path = data_dir + "/FileProofs/" + test_file_id + ".json";
    bool verification_result = node->VerifyFileProof(proof_file_path);
    
    if (verification_result) {
        std::cout << "\n✅ 文件证明验证成功!" << std::endl;
        std::cout << "✓ 配对等式验证通过" << std::endl;
        std::cout << "✓ 文件完整性有效" << std::endl;
    } else {
        std::cerr << "\n❌ 文件证明验证失败!" << std::endl;
        std::cerr << "✗ 配对等式不成立" << std::endl;
    }
    
    
    // ========== 步骤7: 总结 ==========
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📈 测试总结" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "文件ID: " << test_file_id << std::endl;
    std::cout << "证明生成: " << (proof_generated ? "✅ 通过" : "❌ 失败") << std::endl;
    std::cout << "证明验证: " << (verification_result ? "✅ 通过" : "❌ 失败") << std::endl;
    
    if (proof_generated && verification_result) {
        std::cout << "\n🎉 所有测试通过!" << std::endl;
    } else {
        std::cout << "\n⚠️  存在测试失败项,请检查!" << std::endl;
    }
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    
    // 清理
    delete node;
    return (proof_generated && verification_result) ? 0 : 1;
}