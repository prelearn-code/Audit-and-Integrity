/*
 * test_file_proof_debug.cpp - 文件证明生成与验证完整调试版本
 * 
 * 功能: 
 * 1. 完全重新实现 GetFileProof() 和 VerifyFileProof()
 * 2. 输出所有中间变量到控制台
 * 3. 保存所有中间数据到 debug_output.txt
 * 4. 每一步都可以暂停检查
 * 
 * 编译: 
 * g++ -std=c++11 -o test_file_proof_debug test_file_proof_debug.cpp storage_node.o \
 *     -I/usr/local/include/pbc -lpbc -lgmp -lcrypto -ljsoncpp
 * 
 * 运行: ./test_file_proof_debug
 */

#include "storage_node.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

// ============================================================================
// 全局调试输出文件
// ============================================================================

std::ofstream debug_file;

// 辅助函数：同时输出到控制台和文件
void log_output(const std::string& msg, bool console = true, bool file = true) {
    if (console) {
        std::cout << msg;
    }
    if (file && debug_file.is_open()) {
        debug_file << msg;
    }
}

// 格式化输出分隔线
void log_separator(const std::string& title = "") {
    std::string line = "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    log_output("\n" + line + "\n");
    if (!title.empty()) {
        log_output(title + "\n");
        log_output(line + "\n");
    }
}

// 输出十六进制数据（带截断）
void log_hex_data(const std::string& name, const std::string& hex_str, 
                  int preview_len = 64, bool show_full = false) {
    log_output("  " + name + ":\n");
    log_output("    长度: " + std::to_string(hex_str.length()) + " 字符\n");
    
    if (show_full || hex_str.length() <= preview_len) {
        log_output("    完整: " + hex_str + "\n");
    } else {
        log_output("    前" + std::to_string(preview_len) + "位: " + 
                  hex_str.substr(0, preview_len) + "...\n");
        log_output("    后" + std::to_string(preview_len) + "位: " + 
                  hex_str.substr(hex_str.length() - preview_len) + "\n");
    }
}

// 输出 mpz_t 数据
void log_mpz_data(const std::string& name, mpz_t value, int preview_len = 64) {
    char* value_str = mpz_get_str(NULL, 16, value);
    log_hex_data(name, std::string(value_str), preview_len);
    free(value_str);
}

// 输出 element_t 数据
void log_element_data(const std::string& name, element_t elem, int preview_len = 64) {
    int len = element_length_in_bytes(elem);
    std::vector<unsigned char> bytes(len);
    element_to_bytes(bytes.data(), elem);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < len; i++) {
        ss << std::setw(2) << static_cast<int>(bytes[i]);
    }
    
    log_hex_data(name, ss.str(), preview_len);
}

// 等待用户按回车继续
void wait_for_continue(const std::string& prompt = "按 Enter 继续下一步...") {
    std::cout << "\n⏸️  " << prompt;
    std::cin.get();
}

// ============================================================================
// 完整实现：文件证明生成（带详细输出）
// ============================================================================

bool DebugGetFileProof(StorageNode* node, const std::string& ID_F) {
    
    log_separator("🔨 文件证明生成 - 完整调试版本");
    log_output("文件ID: " + ID_F + "\n");
    
    // ========== 步骤1: 加载文件信息 ==========
    log_separator("【步骤1】加载文件信息");
    
    auto it = node->index_database.find(ID_F);
    if (it == node->index_database.end()) {
        log_output("❌ 错误: 文件不存在\n");
        return false;
    }
    
    const IndexEntry& entry = it->second;
    const std::vector<std::string>& TS_F = entry.TS_F;
    int n = TS_F.size();
    std::string PK = entry.PK;
    
    log_output("✅ 文件信息加载成功\n");
    log_output("  块数量 n: " + std::to_string(n) + "\n");
    log_hex_data("  公钥 PK", PK, 32);
    
    for (int i = 0; i < n && i < 3; i++) {
        log_hex_data("  TS_F[" + std::to_string(i) + "]", TS_F[i], 32);
    }
    if (n > 3) {
        log_output("  ... (共 " + std::to_string(n) + " 个认证标签)\n");
    }
    
    wait_for_continue();
    
    // ========== 步骤2: 加载密文 ==========
    log_separator("【步骤2】加载密文文件");
    
    std::string ciphertext;
    if (!node->load_encrypted_file(ID_F, ciphertext)) {
        log_output("❌ 错误: 无法加载密文文件\n");
        return false;
    }
    
    log_output("✅ 密文加载成功\n");
    log_output("  密文大小: " + std::to_string(ciphertext.size()) + " bytes\n");
    log_output("  BLOCK_SIZE: " + std::to_string(StorageNode::BLOCK_SIZE) + " bytes\n");
    log_output("  SECTOR_SIZE: " + std::to_string(StorageNode::SECTOR_SIZE) + " bytes\n");
    log_output("  每块扇区数: " + std::to_string(StorageNode::SECTORS_PER_BLOCK) + "\n");
    
    // 显示密文前几个字节
    std::stringstream cipher_preview;
    cipher_preview << std::hex << std::setfill('0');
    for (size_t i = 0; i < std::min((size_t)32, ciphertext.size()); i++) {
        cipher_preview << std::setw(2) << (int)(unsigned char)ciphertext[i];
    }
    log_output("  密文前64位: " + cipher_preview.str() + "...\n");
    
    wait_for_continue();
    
    // ========== 步骤3: 生成随机种子 ==========
    log_separator("【步骤3】生成随机种子");
    
    std::string seed = node->generate_random_seed();
    
    log_output("✅ 种子生成成功\n");
    log_hex_data("  seed", seed);
    
    wait_for_continue();
    
    // ========== 步骤4: 初始化累积变量 ==========
    log_separator("【步骤4】初始化累积变量");
    
    element_t phi_element;
    element_init_G1(phi_element, node->pairing);
    element_set1(phi_element);
    log_output("✅ phi_element 初始化为单位元 1\n");
    log_element_data("  phi_element (初始)", phi_element, 32);
    
    mpz_t psi_mpz;
    mpz_init_set_ui(psi_mpz, 0);
    log_output("✅ psi_mpz 初始化为 0\n");
    log_mpz_data("  psi_mpz (初始)", psi_mpz, 32);
    
    wait_for_continue();
    
    // ========== 步骤5: 主循环 - 遍历所有块 ==========
    log_separator("【步骤5】主循环 - 遍历所有块");
    
    log_output("开始遍历 " + std::to_string(n) + " 个块...\n");
    
    for (int i = 0; i < n; i++) {
        log_output("\n┌─────────────────────────────────────────┐\n");
        log_output("│ 处理块 [" + std::to_string(i) + "/" + std::to_string(n-1) + "]\n");
        log_output("└─────────────────────────────────────────┘\n");
        
        // --- 步骤5.1: 计算PRF ---
        log_output("\n  [5.1] 计算 PRF(seed, ID_F, " + std::to_string(i) + ")\n");
        
        mpz_t prf_result;
        mpz_init(prf_result);
        node->compute_prf(prf_result, seed, ID_F, i);
        
        log_output("    ✅ PRF 计算完成\n");
        log_mpz_data("    prf_result", prf_result, 32);
        
        // --- 步骤5.2: 处理该块的所有扇区 ---
        log_output("\n  [5.2] 处理块的所有扇区\n");
        
        size_t block_start = i * StorageNode::BLOCK_SIZE;
        size_t block_end = std::min(block_start + StorageNode::BLOCK_SIZE, ciphertext.size());
        
        log_output("    块起始位置: " + std::to_string(block_start) + "\n");
        log_output("    块结束位置: " + std::to_string(block_end) + "\n");
        log_output("    块大小: " + std::to_string(block_end - block_start) + " bytes\n");
        
        int sector_count = 0;
        for (size_t j = 0; j < StorageNode::SECTORS_PER_BLOCK && 
             (block_start + j * StorageNode::SECTOR_SIZE) < block_end; j++) {
            
            size_t sector_start = block_start + j * StorageNode::SECTOR_SIZE;
            size_t sector_end = std::min(sector_start + StorageNode::SECTOR_SIZE, block_end);
            
            // 提取扇区数据
            std::vector<unsigned char> sector_data(
                ciphertext.begin() + sector_start,
                ciphertext.begin() + sector_end
            );
            
            if (i == 0 && j == 0) {
                // 只在第一个块的第一个扇区输出详细信息
                log_output("\n    扇区 [" + std::to_string(j) + "] (首个扇区详情):\n");
                log_output("      起始: " + std::to_string(sector_start) + "\n");
                log_output("      结束: " + std::to_string(sector_end) + "\n");
                log_output("      大小: " + std::to_string(sector_data.size()) + " bytes\n");
                
                // 显示扇区前几个字节
                std::stringstream sector_preview;
                sector_preview << std::hex << std::setfill('0');
                for (size_t k = 0; k < std::min((size_t)16, sector_data.size()); k++) {
                    sector_preview << std::setw(2) << (int)sector_data[k] << " ";
                }
                log_output("      数据预览: " + sector_preview.str() + "...\n");
            }
            
            // 转换为 mpz_t
            mpz_t C_ij;
            mpz_init(C_ij);
            mpz_import(C_ij, sector_data.size(), 1, 1, 0, 0, sector_data.data());
            
            if (i == 0 && j == 0) {
                log_mpz_data("      C_ij", C_ij, 32);
            }
            
            // 计算 product = prf_result * C_ij
            mpz_t product;
            mpz_init(product);
            mpz_mul(product, prf_result, C_ij);
            
            if (i == 0 && j == 0) {
                log_mpz_data("      product = prf * C_ij", product, 32);
            }
            
            // 累加到 psi_mpz
            mpz_add(psi_mpz, psi_mpz, product);
            mpz_mod(psi_mpz, psi_mpz, node->N);
            
            if (i == 0 && j == 0) {
                log_mpz_data("      psi_mpz (累加后)", psi_mpz, 32);
            }
            
            mpz_clear(C_ij);
            mpz_clear(product);
            sector_count++;
        }
        
        log_output("    ✅ 处理了 " + std::to_string(sector_count) + " 个扇区\n");
        
        // --- 步骤5.3: 计算 phi 累乘 ---
        log_output("\n  [5.3] 计算 phi *= (TS_F[" + std::to_string(i) + "])^prf_result\n");
        
        if (i < (int)TS_F.size()) {
            element_t theta_i;
            element_init_G1(theta_i, node->pairing);
            
            // 反序列化 TS_F[i]
            std::vector<unsigned char> theta_bytes = node->hexToBytes(TS_F[i]);
            if (!theta_bytes.empty()) {
                int bytes_read = element_from_bytes(theta_i, theta_bytes.data());
                
                if (i == 0) {
                    log_output("    ✅ TS_F[" + std::to_string(i) + "] 反序列化成功\n");
                    log_output("    读取字节数: " + std::to_string(bytes_read) + "\n");
                    log_element_data("    theta_i", theta_i, 32);
                }
                
                // 计算 theta_i^prf_result
                element_t phi_temp;
                element_init_G1(phi_temp, node->pairing);
                element_pow_mpz(phi_temp, theta_i, prf_result);
                
                if (i == 0) {
                    log_output("    ✅ 幂运算完成\n");
                    log_element_data("    phi_temp = theta_i^prf", phi_temp, 32);
                }
                
                // 累乘
                element_mul(phi_element, phi_element, phi_temp);
                
                if (i == 0) {
                    log_output("    ✅ phi 累乘完成\n");
                    log_element_data("    phi_element (更新后)", phi_element, 32);
                }
                
                element_clear(phi_temp);
            }
            
            element_clear(theta_i);
        }
        
        mpz_clear(prf_result);
        
        // 每处理一个块后询问是否继续
        if (i < n - 1 && i < 2) {
            wait_for_continue("块 [" + std::to_string(i) + "] 处理完成，按 Enter 继续下一块...");
        } else if (i == 2 && n > 3) {
            log_output("\n  ... 继续处理剩余 " + std::to_string(n - 3) + " 个块（不显示详情）...\n");
        }
    }
    
    log_output("\n✅ 所有块处理完成！\n");
    
    wait_for_continue();
    
    // ========== 步骤6: 转换最终结果 ==========
    log_separator("【步骤6】转换最终结果");
    
    // 转换 psi
    char* psi_str = mpz_get_str(NULL, 16, psi_mpz);
    std::string psi_final(psi_str);
    free(psi_str);
    
    log_output("✅ psi 转换为十六进制字符串\n");
    log_hex_data("  psi (最终)", psi_final);
    
    // 转换 phi
    int phi_len = element_length_in_bytes(phi_element);
    std::vector<unsigned char> phi_bytes(phi_len);
    element_to_bytes(phi_bytes.data(), phi_element);
    std::string phi_final = node->bytesToHex(phi_bytes.data(), phi_len);
    
    log_output("✅ phi 转换为十六进制字符串\n");
    log_hex_data("  phi (最终)", phi_final);
    
    mpz_clear(psi_mpz);
    element_clear(phi_element);
    
    wait_for_continue();
    
    // ========== 步骤7: 构建并保存JSON ==========
    log_separator("【步骤7】构建并保存JSON");
    
    Json::Value output;
    output["ID_F"] = ID_F;
    
    Json::Value fileproof_json;
    fileproof_json["psi"] = psi_final;
    fileproof_json["phi"] = phi_final;
    output["FileProof"] = fileproof_json;
    
    output["seed"] = seed;
    
    std::string output_path = node->get_data_dir() + "/FileProofs/" + ID_F + ".json";
    
    // 确保目录存在
    node->create_directory(node->get_data_dir() + "/FileProofs");
    
    if (!node->save_json_to_file(output, output_path)) {
        log_output("❌ 错误: JSON保存失败\n");
        return false;
    }
    
    log_output("✅ JSON保存成功\n");
    log_output("  输出路径: " + output_path + "\n");
    log_output("\n📄 JSON内容:\n");
    log_output("  {\n");
    log_output("    \"ID_F\": \"" + ID_F + "\",\n");
    log_output("    \"FileProof\": {\n");
    log_output("      \"psi\": \"" + psi_final.substr(0, 64) + "...\",\n");
    log_output("      \"phi\": \"" + phi_final.substr(0, 64) + "...\"\n");
    log_output("    },\n");
    log_output("    \"seed\": \"" + seed + "\"\n");
    log_output("  }\n");
    
    log_separator("✅ 文件证明生成完成！");
    
    return true;
}

// ============================================================================
// 完整实现：文件证明验证（带详细输出）
// ============================================================================

bool DebugVerifyFileProof(StorageNode* node, const std::string& file_proof_json_path) {
    
    log_separator("🔍 文件证明验证 - 完整调试版本");
    log_output("证明文件: " + file_proof_json_path + "\n");
    
    // ========== 步骤1: 加载证明文件 ==========
    log_separator("【步骤1】加载证明文件");
    
    if (!node->file_exists(file_proof_json_path)) {
        log_output("❌ 错误: 证明文件不存在\n");
        return false;
    }
    
    Json::Value proof_data = node->load_json_from_file(file_proof_json_path);
    
    if (!proof_data.isMember("ID_F") || !proof_data.isMember("FileProof") ||
        !proof_data.isMember("seed")) {
        log_output("❌ 错误: 证明文件缺少必需字段\n");
        return false;
    }
    
    std::string ID_F = proof_data["ID_F"].asString();
    std::string seed = proof_data["seed"].asString();
    
    const Json::Value& fileproof_json = proof_data["FileProof"];
    std::string psi = fileproof_json["psi"].asString();
    std::string phi = fileproof_json["phi"].asString();
    
    log_output("✅ 证明文件加载成功\n");
    log_output("  文件ID: " + ID_F + "\n");
    log_hex_data("  seed", seed);
    log_hex_data("  psi", psi);
    log_hex_data("  phi", phi);
    
    wait_for_continue();
    
    // ========== 步骤2: 加载文件信息 ==========
    log_separator("【步骤2】加载文件信息");
    
    if (!node->load_index_database()) {
        log_output("❌ 错误: 索引数据库加载失败\n");
        return false;
    }
    
    auto it = node->index_database.find(ID_F);
    if (it == node->index_database.end()) {
        log_output("❌ 错误: 文件不存在\n");
        return false;
    }
    
    int n = it->second.TS_F.size();
    std::string PK = it->second.PK;
    
    log_output("✅ 文件信息加载成功\n");
    log_output("  块数量 n: " + std::to_string(n) + "\n");
    log_hex_data("  公钥 PK", PK, 32);
    
    wait_for_continue();
    
    // ========== 步骤3: 反序列化证明数据 ==========
    log_separator("【步骤3】反序列化证明数据");
    
    // 反序列化 phi
    log_output("[3.1] 反序列化 phi\n");
    element_t phi_elem;
    element_init_G1(phi_elem, node->pairing);
    if (!node->deserializeElement(phi, phi_elem)) {
        log_output("❌ 错误: phi 反序列化失败\n");
        element_clear(phi_elem);
        return false;
    }
    log_output("  ✅ phi 反序列化成功\n");
    log_element_data("  phi_elem", phi_elem, 32);
    
    // 转换 psi
    log_output("\n[3.2] 转换 psi\n");
    mpz_t psi_mpz;
    mpz_init(psi_mpz);
    if (mpz_set_str(psi_mpz, psi.c_str(), 16) != 0) {
        log_output("❌ 错误: psi 转换失败\n");
        element_clear(phi_elem);
        mpz_clear(psi_mpz);
        return false;
    }
    log_output("  ✅ psi 转换成功\n");
    log_mpz_data("  psi_mpz", psi_mpz, 32);
    
    // 反序列化 PK
    log_output("\n[3.3] 反序列化 PK\n");
    element_t PK_elem;
    element_init_G1(PK_elem, node->pairing);
    if (!node->deserializeElement(PK, PK_elem)) {
        log_output("❌ 错误: PK 反序列化失败\n");
        element_clear(phi_elem);
        mpz_clear(psi_mpz);
        element_clear(PK_elem);
        return false;
    }
    log_output("  ✅ PK 反序列化成功\n");
    log_element_data("  PK_elem", PK_elem, 32);
    
    wait_for_continue();
    
    // ========== 步骤4: 计算 zeta ==========
    log_separator("【步骤4】计算 zeta");
    
    element_t zeta;
    element_init_G1(zeta, node->pairing);
    element_set1(zeta);
    log_output("✅ zeta 初始化为单位元 1\n");
    log_element_data("  zeta (初始)", zeta, 32);
    
    log_output("\n开始循环计算 zeta...\n");
    
    for (int i = 0; i < n; i++) {
        if (i < 3 || i == n - 1) {
            log_output("\n  [块 " + std::to_string(i) + "]\n");
        } else if (i == 3) {
            log_output("\n  ... 处理中间块（不显示详情）...\n");
        }
        
        // 计算 prf_temp
        mpz_t prf_temp;
        mpz_init(prf_temp);
        node->compute_prf(prf_temp, seed, ID_F, i);
        
        if (i < 2) {
            log_mpz_data("    prf_temp", prf_temp, 32);
        }
        
        // 计算 h2_temp = H2(ID_F || i)
        std::string id_with_index = ID_F + std::to_string(i);
        element_t h2_temp;
        element_init_G1(h2_temp, node->pairing);
        node->computeHashH2(id_with_index, h2_temp);
        
        if (i < 2) {
            log_output("    H2 输入: \"" + id_with_index + "\"\n");
            log_element_data("    h2_temp", h2_temp, 32);
        }
        
        // 计算 h2_temp^prf_temp
        element_t temp_pow;
        element_init_G1(temp_pow, node->pairing);
        element_pow_mpz(temp_pow, h2_temp, prf_temp);
        
        if (i < 2) {
            log_element_data("    temp_pow = h2_temp^prf", temp_pow, 32);
        }
        
        // 累乘
        element_mul(zeta, zeta, temp_pow);
        
        if (i < 2) {
            log_element_data("    zeta (更新后)", zeta, 32);
        }
        
        element_clear(h2_temp);
        element_clear(temp_pow);
        mpz_clear(prf_temp);
        
        if (i < 2) {
            wait_for_continue("块 [" + std::to_string(i) + "] 处理完成，按 Enter 继续...");
        }
    }
    
    log_output("\n✅ zeta 计算完成\n");
    log_element_data("  zeta (最终)", zeta);
    
    wait_for_continue();
    
    // ========== 步骤5: 计算配对等式左边 ==========
    log_separator("【步骤5】计算配对等式左边 left = e(phi, g)");
    
    element_t left_pairing;
    element_init_GT(left_pairing, node->pairing);
    pairing_apply(left_pairing, phi_elem, node->g, node->pairing);
    
    log_output("✅ left 计算完成\n");
    log_element_data("  left = e(phi, g)", left_pairing);
    
    wait_for_continue();
    
    // ========== 步骤6: 计算配对等式右边 ==========
    log_separator("【步骤6】计算配对等式右边 right = e(zeta * μ^psi, PK)");
    
    // 计算 μ^psi
    log_output("[6.1] 计算 μ^psi\n");
    element_t mu_pow_psi;
    element_init_G1(mu_pow_psi, node->pairing);
    element_pow_mpz(mu_pow_psi, node->mu, psi_mpz);
    
    log_output("  ✅ μ^psi 计算完成\n");
    log_element_data("  μ^psi", mu_pow_psi, 32);
    
    // 计算 right_g1 = zeta * μ^psi
    log_output("\n[6.2] 计算 right_g1 = zeta * μ^psi\n");
    element_t right_g1;
    element_init_G1(right_g1, node->pairing);
    element_mul(right_g1, zeta, mu_pow_psi);
    
    log_output("  ✅ right_g1 计算完成\n");
    log_element_data("  right_g1", right_g1, 32);
    
    // 计算 right = e(right_g1, PK)
    log_output("\n[6.3] 计算 right = e(right_g1, PK)\n");
    element_t right_pairing;
    element_init_GT(right_pairing, node->pairing);
    pairing_apply(right_pairing, right_g1, PK_elem, node->pairing);
    
    log_output("  ✅ right 计算完成\n");
    log_element_data("  right = e(right_g1, PK)", right_pairing);
    
    wait_for_continue();
    
    // ========== 步骤7: 比较配对结果 ==========
    log_separator("【步骤7】比较配对等式: left == right ?");
    
    log_element_data("  left", left_pairing);
    log_element_data("  right", right_pairing);
    
    int comparison = element_cmp(left_pairing, right_pairing);
    
    log_output("\n  element_cmp(left, right) = " + std::to_string(comparison) + "\n");
    log_output("  (0 表示相等，非0表示不等)\n\n");
    
    bool verification_result = (comparison == 0);
    
    if (verification_result) {
        log_output("  ✅ 配对等式成立!\n");
        log_output("  ✅ e(phi, g) == e(zeta * μ^psi, PK)\n");
    } else {
        log_output("  ❌ 配对等式不成立!\n");
        log_output("  ❌ e(phi, g) != e(zeta * μ^psi, PK)\n");
    }
    
    // 清理资源
    element_clear(phi_elem);
    mpz_clear(psi_mpz);
    element_clear(PK_elem);
    element_clear(zeta);
    element_clear(left_pairing);
    element_clear(right_pairing);
    element_clear(mu_pow_psi);
    element_clear(right_g1);
    
    log_separator(verification_result ? "✅ 文件证明验证成功！" : "❌ 文件证明验证失败！");
    
    return verification_result;
}

// ============================================================================
// 主函数
// ============================================================================

int main() {
    
    // 打开调试输出文件
    debug_file.open("debug_output.txt", std::ios::out | std::ios::trunc);
    if (!debug_file.is_open()) {
        std::cerr << "警告: 无法创建 debug_output.txt 文件" << std::endl;
    }
    
    log_separator("🧪 文件证明完整调试测试程序");
    log_output("所有中间数据将输出到: debug_output.txt\n");
    log_output("每个关键步骤都会暂停等待用户确认\n");
    
    // ========== 初始化 ==========
    std::string data_dir = "../data";
    StorageNode* node = new StorageNode(data_dir, 9000);
    
    if (!node->initialize_directories()) {
        log_output("❌ 目录初始化失败\n");
        debug_file.close();
        delete node;
        return 1;
    }
    
    log_output("\n📥 加载公共参数...\n");
    std::string public_params_path = data_dir + "/public_params.json";
    if (!node->load_public_params(public_params_path)) {
        log_output("❌ 公共参数加载失败\n");
        debug_file.close();
        delete node;
        return 1;
    }
    log_output("✅ 公共参数加载成功\n");
    
    log_output("\n📥 加载索引数据库...\n");
    if (!node->load_index_database()) {
        log_output("❌ 索引数据库加载失败\n");
        debug_file.close();
        delete node;
        return 1;
    }
    log_output("✅ 索引数据库加载成功\n");
    log_output("📊 文件总数: " + std::to_string(node->get_file_count()) + "\n");
    
    // 列出可用文件
    std::vector<std::string> files = node->list_all_files();
    if (!files.empty()) {
        log_output("\n💡 可用的文件ID:\n");
        for (size_t i = 0; i < files.size() && i < 5; i++) {
            log_output("   " + std::to_string(i+1) + ". " + files[i] + "\n");
        }
    }
    
    // ========== 选择测试文件 ==========
    std::cout << "\n📝 请输入要测试的文件ID: ";
    std::string test_file_id;
    std::getline(std::cin, test_file_id);
    
    if (!node->has_file(test_file_id)) {
        log_output("❌ 文件不存在: " + test_file_id + "\n");
        debug_file.close();
        delete node;
        return 1;
    }
    
    log_output("✅ 找到文件: " + test_file_id + "\n");
    
    wait_for_continue("\n准备开始测试，按 Enter 继续...");
    
    // ========== 执行证明生成 ==========
    bool proof_generated = DebugGetFileProof(node, test_file_id);
    
    if (!proof_generated) {
        log_output("\n❌ 证明生成失败\n");
        debug_file.close();
        delete node;
        return 1;
    }
    
    wait_for_continue("\n证明生成完成，按 Enter 继续验证测试...");
    
    // ========== 执行证明验证 ==========
    std::string proof_file_path = data_dir + "/FileProofs/" + test_file_id + ".json";
    bool verification_result = DebugVerifyFileProof(node, proof_file_path);
    
    // ========== 最终总结 ==========
    log_separator("📊 测试总结");
    log_output("文件ID: " + test_file_id + "\n");
    log_output("证明生成: " + std::string(proof_generated ? "✅ 成功" : "❌ 失败") + "\n");
    log_output("证明验证: " + std::string(verification_result ? "✅ 成功" : "❌ 失败") + "\n");
    
    if (proof_generated && verification_result) {
        log_output("\n🎉 所有测试通过!\n");
        log_separator();
    } else {
        log_output("\n⚠️  测试失败，请查看 debug_output.txt 获取详细信息\n");
        log_separator();
    }
    
    log_output("\n所有调试数据已保存到: debug_output.txt\n");
    
    // 清理
    debug_file.close();
    delete node;
    
    return (proof_generated && verification_result) ? 0 : 1;
}