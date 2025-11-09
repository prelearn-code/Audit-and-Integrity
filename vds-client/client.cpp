#include "client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

StorageClient::StorageClient()
    : initialized_(false), states_loaded_(false) {
    mpz_init(N_);
    mpz_init(sk_);
}

StorageClient::~StorageClient() {
    if (initialized_) {
        element_clear(g_);
        element_clear(mu_);
        element_clear(pk_);
        pairing_clear(pairing_);
    }
    
    mpz_clear(N_);
    mpz_clear(sk_);
    
    memset(mk_, 0, sizeof(mk_));
    memset(ek_, 0, sizeof(ek_));
}

bool StorageClient::initialize() {
    try {
        std::cout << "初始化客户端..." << std::endl;
        std::cout << "从配置文件加载系统参数..." << std::endl;
        
        // 从JSON文件加载参数
        if (!loadSystemParams()) {
            std::cerr << "系统参数加载失败" << std::endl;
            return false;
        }
        
        // 初始化配对系统元素
        element_init_G1(g_, pairing_);
        element_init_G1(mu_, pairing_);
        element_init_G1(pk_, pairing_);
        
        // 随机生成生成元
        element_random(g_);
        element_random(mu_);
        
        initialized_ = true;
        std::cout << "✅ 客户端初始化成功" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "初始化错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::loadSystemParams(const std::string& param_file) {
    try {
        // 读取JSON文件
        std::ifstream config_file(param_file);
        if (!config_file.is_open()) {
            std::cerr << "无法打开配置文件: " << param_file << std::endl;
            std::cerr << "提示：请确保 " << param_file << " 文件存在于程序同目录下" << std::endl;
            return false;
        }
        
        Json::Value config;
        Json::Reader reader;
        if (!reader.parse(config_file, config)) {
            std::cerr << "JSON解析失败: " << reader.getFormattedErrorMessages() << std::endl;
            config_file.close();
            return false;
        }
        config_file.close();
        
        // 验证必要字段
        if (!config.isMember("parameters") || !config.isMember("system_values")) {
            std::cerr << "配置文件格式错误：缺少必要字段" << std::endl;
            return false;
        }
        
        // 构建配对参数字符串
        const Json::Value& params = config["parameters"];
        char param_str[1024];
        snprintf(param_str, sizeof(param_str),
            "type %s\n"
            "q %s\n"
            "h %s\n"
            "r %s\n"
            "exp2 %s\n"
            "exp1 %s\n"
            "sign1 %s\n"
            "sign0 %s\n",
            config["pairing_type"].asString().c_str(),
            params["q"].asString().c_str(),
            params["h"].asString().c_str(),
            params["r"].asString().c_str(),
            params["exp2"].asString().c_str(),
            params["exp1"].asString().c_str(),
            params["sign1"].asString().c_str(),
            params["sign0"].asString().c_str());
        
        // 初始化配对
        if (pairing_init_set_buf(pairing_, param_str, strlen(param_str)) != 0) {
            std::cerr << "配对初始化失败" << std::endl;
            return false;
        }
        
        // 设置N值
        std::string N_str = config["system_values"]["N"].asString();
        if (mpz_set_str(N_, N_str.c_str(), 10) != 0) {
            std::cerr << "N值设置失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ 系统参数加载成功" << std::endl;
        std::cout << "   配对类型: " << config["pairing_type"].asString() << std::endl;
        if (config.isMember("security_level")) {
            std::cout << "   安全级别: " << config["security_level"].asString() << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "参数加载错误: " << e.what() << std::endl;
        return false;
    }
}

// ============ v3.3 修改：generateKeys() ============
bool StorageClient::generateKeys(const std::string& public_params_file) {
    if (!initialized_) {
        std::cerr << "客户端未初始化" << std::endl;
        return false;
    }
    
    try {
        std::cout << "生成客户端密钥..." << std::endl;
        std::cout << "从 " << public_params_file << " 读取公共参数..." << std::endl;
        
        // 1. 读取 public_params.json
        std::ifstream params_file(public_params_file);
        if (!params_file.is_open()) {
            std::cerr << "无法打开公共参数文件: " << public_params_file << std::endl;
            std::cerr << "提示: 此文件应由 Storage Node 生成" << std::endl;
            return false;
        }
        
        Json::Value public_params;
        Json::Reader reader;
        if (!reader.parse(params_file, public_params)) {
            std::cerr << "公共参数JSON解析失败" << std::endl;
            params_file.close();
            return false;
        }
        params_file.close();
        
        // 2. 从 public_params 提取 generator_g_hex 并初始化 g_
        if (public_params.isMember("G_1") && public_params["G_1"].isMember("generator_g_hex")) {
            std::string g_hex = public_params["G_1"]["generator_g_hex"].asString();
            if (!deserializeElement(g_hex, g_)) {
                std::cerr << "生成元 g 反序列化失败" << std::endl;
                return false;
            }
            std::cout << "✅ 从公共参数加载生成元 g" << std::endl;
        } else {
            std::cerr << "公共参数中缺少 generator_g_hex" << std::endl;
            return false;
        }
        
        // 3. 生成随机 sk
        if (RAND_bytes(mk_, sizeof(mk_)) != 1) {
            std::cerr << "主密钥生成失败" << std::endl;
            return false;
        }
        
        if (RAND_bytes(ek_, sizeof(ek_)) != 1) {
            std::cerr << "加密密钥生成失败" << std::endl;
            return false;
        }
        
        element_t sk_elem;
        element_init_Zr(sk_elem, pairing_);
        element_random(sk_elem);
        element_to_mpz(sk_, sk_elem);
        element_clear(sk_elem);
        
        // 4. 计算 pk = g^sk
        element_pow_mpz(pk_, g_, sk_);
        
        std::cout << "✅ 密钥生成成功" << std::endl;
        
        // 5. 保存私钥到 private_key.dat（二进制格式）
        std::ofstream priv_file("private_key.dat", std::ios::binary);
        if (!priv_file.is_open()) {
            std::cerr << "无法创建私钥文件" << std::endl;
            return false;
        }
        
        priv_file.write((const char*)mk_, sizeof(mk_));
        priv_file.write((const char*)ek_, sizeof(ek_));
        
        char* sk_str = mpz_get_str(nullptr, 16, sk_);
        size_t sk_len = strlen(sk_str);
        priv_file.write((const char*)&sk_len, sizeof(sk_len));
        priv_file.write(sk_str, sk_len);
        free(sk_str);
        
        priv_file.close();
        std::cout << "✅ 私钥已保存到: private_key.dat" << std::endl;
        
        // 6. 保存公钥到 public_key.json（JSON格式）
        Json::Value pub_key_json;
        pub_key_json["version"] = "1.0";
        pub_key_json["created_at"] = getCurrentTimestamp();
        pub_key_json["PK"] = serializeElement(pk_);
        
        // 生成客户端ID（基于公钥的哈希）
        std::string pk_hex = serializeElement(pk_);
        unsigned char client_id_hash[SHA256_DIGEST_LENGTH];
        SHA256((const unsigned char*)pk_hex.c_str(), pk_hex.length(), client_id_hash);
        std::stringstream client_id_ss;
        for (int i = 0; i < 8; i++) {  // 使用前8字节作为ID
            client_id_ss << std::hex << std::setw(2) << std::setfill('0') << (int)client_id_hash[i];
        }
        
        pub_key_json["node_info"]["client_id"] = client_id_ss.str();
        pub_key_json["node_info"]["key_generated_at"] = getCurrentTimestamp();
        
        Json::StyledWriter writer;
        std::ofstream pub_file("public_key.json");
        if (!pub_file.is_open()) {
            std::cerr << "无法创建公钥文件" << std::endl;
            return false;
        }
        
        pub_file << writer.write(pub_key_json);
        pub_file.close();
        std::cout << "✅ 公钥已保存到: public_key.json" << std::endl;
        std::cout << "📌 客户端ID: " << client_id_ss.str() << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "密钥生成错误: " << e.what() << std::endl;
        return false;
    }
}

// ============ v3.3 修改：encryptFile() ============
bool StorageClient::encryptFile(const std::string& file_path, 
                                const std::vector<std::string>& keywords,
                                const std::string& output_prefix,
                                const std::string& insert_json_path) {
    if (!initialized_) {
        std::cerr << "客户端未初始化" << std::endl;
        return false;
    }
    
    try {
        std::cout << "加密文件: " << file_path << std::endl;
        
        // 提取原始文件名
        size_t last_slash = file_path.find_last_of("/\\");
        std::string original_filename = (last_slash == std::string::npos) ? 
                                       file_path : file_path.substr(last_slash + 1);
        
        // 读取文件
        std::vector<unsigned char> plaintext;
        if (!readFile(file_path, plaintext)) {
            std::cerr << "文件读取失败" << std::endl;
            return false;
        }
        
        std::cout << "文件大小: " << plaintext.size() << " 字节" << std::endl;
        
        // 加密文件
        std::vector<unsigned char> ciphertext;
        if (!encryptFileData(plaintext, ciphertext)) {
            std::cerr << "文件加密失败" << std::endl;
            return false;
        }
        
        // 生成文件ID
        std::string file_id;
        {
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256(ciphertext.data(), ciphertext.size(), hash);
            
            std::stringstream ss;
            for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
            }
            file_id = ss.str();
        }
        
        std::cout << "文件ID: " << file_id << std::endl;
        
        // 生成认证标签
        std::vector<std::string> auth_tags;
        if (!generateAuthTags(file_id, ciphertext, auth_tags)) {
            std::cerr << "认证标签生成失败" << std::endl;
            return false;
        }
        
        std::cout << "生成了 " << auth_tags.size() << " 个认证标签" << std::endl;
        
        // 生成关键词标签
        Json::Value keywords_array;
        Json::Value local_keywords_meta;
        
        for (const auto& keyword : keywords) {
            // 生成搜索令牌 Ti
            std::string Ti = encryptKeyword(keyword);
            
            // 生成或获取状态
            std::string current_state = generateRandomState();
            std::string previous_state;
            
            if (keyword_states_.find(keyword) != keyword_states_.end()) {
                previous_state = keyword_states_[keyword];
            }
            
            // 更新状态
            keyword_states_[keyword] = current_state;
            
            // 更新状态文件（如果已加载）
            if (states_loaded_) {
                updateKeywordState(keyword, current_state, file_id);
            }
            
            // 生成状态关联令牌 kt^wi
            std::string kt;
            if (!generateStateAssociatedToken(file_id, Ti, current_state, previous_state, kt)) {
                std::cerr << "状态关联令牌生成失败" << std::endl;
                return false;
            }
            
            // 为 insert.json 准备数据
            Json::Value keyword_entry;
            keyword_entry["T_i"] = Ti;
            keyword_entry["kt_i"] = kt;
            keywords_array.append(keyword_entry);
            
            // 为本地元数据准备数据
            Json::Value local_kw_entry;
            local_kw_entry["current_state"] = current_state;
            local_kw_entry["previous_state"] = previous_state.empty() ? Json::Value::null : previous_state;
            local_keywords_meta[keyword] = local_kw_entry;
        }
        
        std::cout << "生成了 " << keywords.size() << " 个关键词标签" << std::endl;
        
        // 保存加密文件
        std::string encrypted_filename = output_prefix + ".enc";
        if (!writeFile(encrypted_filename, ciphertext)) {
            std::cerr << "加密文件保存失败" << std::endl;
            return false;
        }
        
        // 1. 生成 insert.json（供 Storage Node 使用）
        Json::Value insert_json;
        
        // 读取公钥
        std::ifstream pub_key_file("public_key.json");
        if (pub_key_file.is_open()) {
            Json::Value pub_key_data;
            Json::Reader reader;
            if (reader.parse(pub_key_file, pub_key_data)) {
                insert_json["PK"] = pub_key_data["PK"];
            }
            pub_key_file.close();
        } else {
            insert_json["PK"] = serializeElement(pk_);
        }
        
        insert_json["ID_F"] = file_id;
        insert_json["ptr"] = ""; // 指针数据（如需要可添加）
        
        // 认证标签转为JSON数组
        Json::Value ts_f_array;
        for (const auto& tag : auth_tags) {
            ts_f_array.append(tag);
        }
        insert_json["TS_F"] = ts_f_array;
        insert_json["state"] = "valid";
        insert_json["keywords"] = keywords_array;
        
        Json::StyledWriter writer;
        std::ofstream insert_file(insert_json_path);
        if (!insert_file.is_open()) {
            std::cerr << "无法创建 insert.json" << std::endl;
            return false;
        }
        insert_file << writer.write(insert_json);
        insert_file.close();
        
        std::cout << "✅ insert.json 已生成: " << insert_json_path << std::endl;
        
        // 2. 生成本地元数据 JSON
        Json::Value local_meta;
        local_meta["original_filename"] = original_filename;
        local_meta["encrypted_filename"] = encrypted_filename;
        local_meta["file_id"] = file_id;
        local_meta["file_size"] = (Json::Value::UInt64)plaintext.size();
        local_meta["encryption_time"] = getCurrentTimestamp();
        
        Json::Value kw_array;
        for (const auto& kw : keywords) {
            kw_array.append(kw);
        }
        local_meta["keywords"] = kw_array;
        local_meta["states"] = local_keywords_meta;
        
        std::string metadata_filename = original_filename + "_metadata.json";
        std::ofstream meta_file(metadata_filename);
        if (!meta_file.is_open()) {
            std::cerr << "无法创建本地元数据文件" << std::endl;
            return false;
        }
        meta_file << writer.write(local_meta);
        meta_file.close();
        
        std::cout << "✅ 本地元数据已生成: " << metadata_filename << std::endl;
        
        // 保存状态文件（如果已加载）
        if (states_loaded_) {
            saveKeywordStates(keyword_states_file_);
        }
        
        std::cout << "\n🎉 文件加密完成！" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "📦 加密文件: " << encrypted_filename << std::endl;
        std::cout << "📋 插入数据: " << insert_json_path << std::endl;
        std::cout << "📄 本地元数据: " << metadata_filename << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "文件加密错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::decryptFile(const std::string& encrypted_file, 
                                const std::string& output_path) {
    if (!initialized_) {
        std::cerr << "客户端未初始化" << std::endl;
        return false;
    }
    
    try {
        std::cout << "解密文件: " << encrypted_file << std::endl;
        
        std::vector<unsigned char> ciphertext;
        if (!readFile(encrypted_file, ciphertext)) {
            std::cerr << "加密文件读取失败" << std::endl;
            return false;
        }
        
        std::vector<unsigned char> plaintext;
        if (!decryptFileData(ciphertext, plaintext)) {
            std::cerr << "文件解密失败" << std::endl;
            return false;
        }
        
        if (!writeFile(output_path, plaintext)) {
            std::cerr << "解密文件保存失败" << std::endl;
            return false;
        }
        
        std::cout << "✅ 文件解密成功" << std::endl;
        std::cout << "保存到: " << output_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "解密错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::encryptFileData(const std::vector<unsigned char>& plaintext,
                                    std::vector<unsigned char>& ciphertext) {
    try {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;
        
        unsigned char iv[16];
        if (RAND_bytes(iv, sizeof(iv)) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, ek_, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        
        ciphertext.resize(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()) + 16);
        memcpy(ciphertext.data(), iv, 16);
        
        int len, ciphertext_len = 0;
        if (EVP_EncryptUpdate(ctx, ciphertext.data() + 16, &len,
                             plaintext.data(), plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        ciphertext_len += len;
        
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + 16 + ciphertext_len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        ciphertext_len += len;
        
        ciphertext.resize(16 + ciphertext_len);
        
        EVP_CIPHER_CTX_free(ctx);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "加密数据错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::decryptFileData(const std::vector<unsigned char>& ciphertext,
                                    std::vector<unsigned char>& plaintext) {
    try {
        if (ciphertext.size() < 16) {
            std::cerr << "密文太短" << std::endl;
            return false;
        }
        
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return false;
        
        unsigned char iv[16];
        memcpy(iv, ciphertext.data(), 16);
        
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, ek_, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        
        plaintext.resize(ciphertext.size());
        
        int len, plaintext_len = 0;
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                             ciphertext.data() + 16, ciphertext.size() - 16) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        plaintext_len += len;
        
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + plaintext_len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return false;
        }
        plaintext_len += len;
        
        plaintext.resize(plaintext_len);
        
        EVP_CIPHER_CTX_free(ctx);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "解密数据错误: " << e.what() << std::endl;
        return false;
    }
}

// ============ v3.3 重写：generateAuthTags() ============
bool StorageClient::generateAuthTags(const std::string& file_id,
                                     const std::vector<unsigned char>& ciphertext,
                                     std::vector<std::string>& auth_tags) {
    try {
        // 1. 分块：每块BLOCK_SIZE字节
        auto blocks = splitIntoBlocks(ciphertext, BLOCK_SIZE);
        
        auth_tags.clear();
        auth_tags.reserve(blocks.size());
        
        // 2. 对每个块生成认证标签
        for (size_t i = 0; i < blocks.size(); i++) {
            // 2.1 计算 H_2(ID_F||i)
            std::string block_index_str = file_id + std::to_string(i);
            element_t h2_result;
            element_init_G1(h2_result, pairing_);
            computeHashH2(block_index_str, h2_result);
            
            // 2.2 将块分为s个扇区
            const auto& block = blocks[i];
            element_t product;
            element_init_G1(product, pairing_);
            element_set1(product);  // 初始化为单位元
            
            // 2.3 对每个扇区计算 μ^{c_{i,j}}
            for (size_t j = 0; j < SECTORS_PER_BLOCK && j * SECTOR_SIZE < block.size(); j++) {
                size_t sector_start = j * SECTOR_SIZE;
                size_t sector_end = std::min(sector_start + SECTOR_SIZE, block.size());
                
                // 将扇区数据转换为大整数 c_{i,j}
                mpz_t c_ij;
                mpz_init(c_ij);
                mpz_import(c_ij, sector_end - sector_start, 1, 1, 0, 0, 
                          block.data() + sector_start);
                
                // 计算 μ^{c_{i,j}}
                element_t mu_pow;
                element_init_G1(mu_pow, pairing_);
                element_pow_mpz(mu_pow, mu_, c_ij);
                
                // 累乘
                element_mul(product, product, mu_pow);
                
                element_clear(mu_pow);
                mpz_clear(c_ij);
            }
            
            // 2.4 计算 H_2(ID_F||i) * product
            element_mul(h2_result, h2_result, product);
            
            // 2.5 签名: [result]^sk
            element_t sigma_i;
            element_init_G1(sigma_i, pairing_);
            element_pow_mpz(sigma_i, h2_result, sk_);
            
            // 2.6 序列化并保存
            auth_tags.push_back(serializeElement(sigma_i));
            
            element_clear(sigma_i);
            element_clear(product);
            element_clear(h2_result);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "认证标签生成错误: " << e.what() << std::endl;
        return false;
    }
}

// ============ v3.3 改名和修改：generateStateAssociatedToken() ============
bool StorageClient::generateStateAssociatedToken(
    const std::string& file_id,
    const std::string& Ti,
    const std::string& current_state,
    const std::string& previous_state,
    std::string& kt_output) {
    
    try {
        // 1. 计算 H_2(ID_F)
        element_t h2_id;
        element_init_G1(h2_id, pairing_);
        computeHashH2(file_id, h2_id);
        
        // 2. 计算 H_2(st_d||Ti)
        std::string current_concat = current_state + Ti;
        element_t h2_current;
        element_init_G1(h2_current, pairing_);
        computeHashH2(current_concat, h2_current);
        
        element_t result;
        element_init_G1(result, pairing_);
        
        if (previous_state.empty()) {
            // 情况2: 第一个文件
            // result = H_2(ID_F) * H_2(st_d||Ti)
            element_mul(result, h2_id, h2_current);
        } else {
            // 情况1: 有前一个状态
            // result = H_2(ID_F) * H_2(st_d||Ti) / H_2(st_{d-1}||Ti)
            
            // 计算 H_2(st_{d-1}||Ti)
            std::string previous_concat = previous_state + Ti;
            element_t h2_previous;
            element_init_G1(h2_previous, pairing_);
            computeHashH2(previous_concat, h2_previous);
            
            // 计算 H_2(st_d||Ti) / H_2(st_{d-1}||Ti)
            element_t quotient;
            element_init_G1(quotient, pairing_);
            element_div(quotient, h2_current, h2_previous);
            
            // 计算 H_2(ID_F) * quotient
            element_mul(result, h2_id, quotient);
            
            element_clear(quotient);
            element_clear(h2_previous);
        }
        
        // 3. 签名: [result]^sk
        element_t kt;
        element_init_G1(kt, pairing_);
        element_pow_mpz(kt, result, sk_);
        
        // 4. 序列化输出
        kt_output = serializeElement(kt);
        
        element_clear(kt);
        element_clear(result);
        element_clear(h2_current);
        element_clear(h2_id);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "状态关联令牌生成错误: " << e.what() << std::endl;
        return false;
    }
}

void StorageClient::computeHashH1(const std::string& input, mpz_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.length(), hash);
    mpz_import(result, SHA256_DIGEST_LENGTH, 1, 1, 0, 0, hash);
    mpz_mod(result, result, N_);
}

void StorageClient::computeHashH2(const std::string& input, element_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.length(), hash);
    element_from_hash(result, hash, SHA256_DIGEST_LENGTH);
}

std::string StorageClient::computeHashH3(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.length(), hash);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// ============ v3.3 简化：encryptKeyword() ============
std::string StorageClient::encryptKeyword(const std::string& keyword) {
    // 只返回 Ti = H3(H1(mk || keyword))
    std::string combined = std::string((char*)mk_, 32) + keyword;
    
    mpz_t hash_result;
    mpz_init(hash_result);
    computeHashH1(combined, hash_result);
    
    char* hash_str = mpz_get_str(nullptr, 16, hash_result);
    std::string Ti = computeHashH3(hash_str);
    free(hash_str);
    mpz_clear(hash_result);
    
    return Ti;
}

std::string StorageClient::generateRandomState() {
    unsigned char random_bytes[32];
    if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
        return "";
    }
    
    std::stringstream ss;
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)random_bytes[i];
    }
    return ss.str();
}

// ============ v3.3 修改：encryptPointer() ============
std::string StorageClient::encryptPointer(const std::string& previous_state,
                                          const std::string& current_state_hash) {
    // 使用当前状态的哈希值作为密钥，加密前一个状态
    // 如果是第一个文件（previous_state为空），则加密当前状态哈希
    
    std::string data_to_encrypt = previous_state.empty() ? current_state_hash : previous_state;
    std::string encryption_key = current_state_hash;
    
    try {
        unsigned char aes_key[32];
        memset(aes_key, 0, sizeof(aes_key));
        size_t key_len = std::min(encryption_key.size() / 2, (size_t)32);
        for (size_t i = 0; i < key_len; i++) {
            sscanf(encryption_key.substr(i * 2, 2).c_str(), "%02hhx", &aes_key[i]);
        }
        
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return "";
        
        unsigned char iv[16];
        memset(iv, 0, sizeof(iv));
        
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, aes_key, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        
        std::vector<unsigned char> ciphertext(data_to_encrypt.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        int len, ciphertext_len = 0;
        
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                             (const unsigned char*)data_to_encrypt.c_str(), data_to_encrypt.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        ciphertext_len += len;
        
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + ciphertext_len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        ciphertext_len += len;
        
        ciphertext.resize(ciphertext_len);
        
        std::stringstream ss;
        for (auto byte : ciphertext) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
        }
        
        EVP_CIPHER_CTX_free(ctx);
        return ss.str();
        
    } catch (const std::exception& e) {
        std::cerr << "指针加密错误: " << e.what() << std::endl;
        return "";
    }
}

// ============ 关键词状态管理功能 ============

bool StorageClient::loadKeywordStates(const std::string& file_path) {
    try {
        std::ifstream state_file(file_path);
        if (!state_file.is_open()) {
            std::cerr << "无法打开状态文件: " << file_path << std::endl;
            std::cerr << "💡 如果是首次使用，这是正常的。加密文件后会自动创建。" << std::endl;
            return false;
        }
        
        Json::Reader reader;
        if (!reader.parse(state_file, keyword_states_data_)) {
            std::cerr << "状态文件JSON解析失败" << std::endl;
            state_file.close();
            return false;
        }
        state_file.close();
        
        // 从JSON加载到内存映射
        keyword_states_.clear();
        if (keyword_states_data_.isMember("keywords")) {
            const Json::Value& keywords_obj = keyword_states_data_["keywords"];
            for (auto it = keywords_obj.begin(); it != keywords_obj.end(); ++it) {
                std::string keyword = it.key().asString();
                if ((*it).isMember("current_state")) {
                    keyword_states_[keyword] = (*it)["current_state"].asString();
                }
            }
        }
        
        keyword_states_file_ = file_path;
        states_loaded_ = true;
        
        std::cout << "✅ 已加载 " << keyword_states_.size() << " 个关键词状态" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "状态文件加载错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::saveKeywordStates(const std::string& file_path) {
    try {
        // 更新JSON数据
        keyword_states_data_["version"] = "1.0";
        keyword_states_data_["last_updated"] = getCurrentTimestamp();
        
        Json::Value keywords_obj;
        for (const auto& pair : keyword_states_) {
            Json::Value kw_data;
            kw_data["current_state"] = pair.second;
            
            // 保留历史记录（如果存在）
            if (keyword_states_data_["keywords"].isMember(pair.first) &&
                keyword_states_data_["keywords"][pair.first].isMember("history")) {
                kw_data["history"] = keyword_states_data_["keywords"][pair.first]["history"];
            } else {
                kw_data["history"] = Json::Value(Json::arrayValue);
            }
            
            keywords_obj[pair.first] = kw_data;
        }
        keyword_states_data_["keywords"] = keywords_obj;
        
        Json::StyledWriter writer;
        std::ofstream state_file(file_path);
        if (!state_file.is_open()) {
            std::cerr << "无法创建状态文件: " << file_path << std::endl;
            return false;
        }
        
        state_file << writer.write(keyword_states_data_);
        state_file.close();
        
        keyword_states_file_ = file_path;
        states_loaded_ = true;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "状态文件保存错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::updateKeywordState(const std::string& keyword,
                                       const std::string& new_state,
                                       const std::string& file_id) {
    try {
        keyword_states_[keyword] = new_state;
        
        // 更新JSON数据
        Json::Value& kw_data = keyword_states_data_["keywords"][keyword];
        kw_data["current_state"] = new_state;
        
        // 添加历史记录
        Json::Value history_entry;
        history_entry["state"] = new_state;
        history_entry["file_id"] = file_id;
        history_entry["timestamp"] = getCurrentTimestamp();
        history_entry["is_current"] = true;
        
        // 将之前的记录标记为非当前
        if (kw_data.isMember("history")) {
            Json::Value& history = kw_data["history"];
            for (Json::ArrayIndex i = 0; i < history.size(); i++) {
                history[i]["is_current"] = false;
            }
            history.append(history_entry);
        } else {
            Json::Value history(Json::arrayValue);
            history.append(history_entry);
            kw_data["history"] = history;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "状态更新错误: " << e.what() << std::endl;
        return false;
    }
}

std::string StorageClient::queryKeywordState(const std::string& keyword) {
    std::stringstream result;
    
    if (keyword_states_.find(keyword) == keyword_states_.end()) {
        result << "\n❌ 关键词 \"" << keyword << "\" 未找到" << std::endl;
        result << "💡 可能还未加密过包含此关键词的文件" << std::endl;
        return result.str();
    }
    
    result << "\n🔍 关键词状态查询结果" << std::endl;
    result << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    result << "关键词: " << keyword << std::endl;
    result << "当前状态: " << keyword_states_[keyword] << std::endl;
    
    if (states_loaded_ && keyword_states_data_["keywords"].isMember(keyword)) {
        const Json::Value& kw_data = keyword_states_data_["keywords"][keyword];
        
        if (kw_data.isMember("history")) {
            const Json::Value& history = kw_data["history"];
            result << "\n📜 历史记录 (" << history.size() << " 条):" << std::endl;
            
            for (Json::ArrayIndex i = 0; i < history.size() && i < 5; i++) {
                const Json::Value& entry = history[history.size() - 1 - i];
                result << "  " << (i + 1) << ". ";
                if (entry["is_current"].asBool()) {
                    result << "[当前] ";
                }
                result << "文件ID: " << entry["file_id"].asString().substr(0, 16) << "..." << std::endl;
                result << "     时间: " << entry["timestamp"].asString() << std::endl;
            }
            
            if (history.size() > 5) {
                result << "  ... 还有 " << (history.size() - 5) << " 条记录" << std::endl;
            }
        }
    }
    
    result << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    return result.str();
}

// ============ 辅助函数 ============

bool StorageClient::readFile(const std::string& file_path, 
                             std::vector<unsigned char>& data) {
    try {
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << file_path << std::endl;
            return false;
        }
        
        file.seekg(0, std::ios::end);
        size_t file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        data.resize(file_size);
        file.read((char*)data.data(), file_size);
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "文件读取错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::writeFile(const std::string& file_path,
                              const std::vector<unsigned char>& data) {
    try {
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法创建文件: " << file_path << std::endl;
            return false;
        }
        
        file.write((const char*)data.data(), data.size());
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "文件写入错误: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::vector<unsigned char>> StorageClient::splitIntoBlocks(
    const std::vector<unsigned char>& data, 
    size_t block_size) {
    
    std::vector<std::vector<unsigned char>> blocks;
    
    for (size_t i = 0; i < data.size(); i += block_size) {
        size_t remaining = data.size() - i;
        size_t current_block_size = std::min(block_size, remaining);
        
        std::vector<unsigned char> block(data.begin() + i, 
                                        data.begin() + i + current_block_size);
        blocks.push_back(block);
    }
    
    return blocks;
}

std::string StorageClient::serializeElement(element_t elem) {
    int len = element_length_in_bytes(elem);
    std::vector<unsigned char> bytes(len);
    element_to_bytes(bytes.data(), elem);
    
    std::stringstream ss;
    for (auto byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

bool StorageClient::deserializeElement(const std::string& hex_str, element_t elem) {
    if (hex_str.empty() || hex_str.length() % 2 != 0) {
        return false;
    }
    
    std::vector<unsigned char> bytes(hex_str.length() / 2);
    for (size_t i = 0; i < bytes.size(); i++) {
        sscanf(hex_str.substr(i * 2, 2).c_str(), "%02hhx", &bytes[i]);
    }
    
    return element_from_bytes(elem, bytes.data()) == 0;
}

std::string StorageClient::bytesToHex(const std::vector<unsigned char>& bytes) {
    std::stringstream ss;
    for (auto byte : bytes) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return ss.str();
}

std::string StorageClient::getCurrentTimestamp() {
    time_t now = time(nullptr);
    struct tm* timeinfo = gmtime(&now);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
    return std::string(buffer);
}

std::string StorageClient::getPublicKey() {
    return serializeElement(pk_);
}

bool StorageClient::saveKeys(const std::string& key_file) {
    try {
        std::ofstream file(key_file, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
        
        file.write((const char*)mk_, sizeof(mk_));
        file.write((const char*)ek_, sizeof(ek_));
        
        char* sk_str = mpz_get_str(nullptr, 16, sk_);
        size_t sk_len = strlen(sk_str);
        file.write((const char*)&sk_len, sizeof(sk_len));
        file.write(sk_str, sk_len);
        free(sk_str);
        
        size_t state_count = keyword_states_.size();
        file.write((const char*)&state_count, sizeof(state_count));
        for (const auto& pair : keyword_states_) {
            size_t keyword_len = pair.first.length();
            file.write((const char*)&keyword_len, sizeof(keyword_len));
            file.write(pair.first.c_str(), keyword_len);
            
            size_t state_len = pair.second.length();
            file.write((const char*)&state_len, sizeof(state_len));
            file.write(pair.second.c_str(), state_len);
        }
        
        file.close();
        std::cout << "✅ 密钥已保存到: " << key_file << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "密钥保存错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::loadKeys(const std::string& key_file) {
    try {
        std::ifstream file(key_file, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "无法打开密钥文件: " << key_file << std::endl;
            return false;
        }
        
        file.read((char*)mk_, sizeof(mk_));
        file.read((char*)ek_, sizeof(ek_));
        
        size_t sk_len;
        file.read((char*)&sk_len, sizeof(sk_len));
        std::vector<char> sk_str(sk_len + 1);
        file.read(sk_str.data(), sk_len);
        sk_str[sk_len] = '\0';
        mpz_set_str(sk_, sk_str.data(), 16);
        
        if (initialized_) {
            element_pow_mpz(pk_, g_, sk_);
        }
        
        size_t state_count;
        file.read((char*)&state_count, sizeof(state_count));
        keyword_states_.clear();
        
        for (size_t i = 0; i < state_count; i++) {
            size_t keyword_len;
            file.read((char*)&keyword_len, sizeof(keyword_len));
            std::string keyword(keyword_len, '\0');
            file.read(&keyword[0], keyword_len);
            
            size_t state_len;
            file.read((char*)&state_len, sizeof(state_len));
            std::string state(state_len, '\0');
            file.read(&state[0], state_len);
            
            keyword_states_[keyword] = state;
        }
        
        file.close();
        std::cout << "✅ 密钥已从 " << key_file << " 加载" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "密钥加载错误: " << e.what() << std::endl;
        return false;
    }
}