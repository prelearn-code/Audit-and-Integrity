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
#include <fstream>

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

bool StorageClient::generateKeys() {
    if (!initialized_) {
        std::cerr << "客户端未初始化" << std::endl;
        return false;
    }
    
    try {
        std::cout << "生成客户端密钥..." << std::endl;
        
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
        
        element_pow_mpz(pk_, g_, sk_);
        
        std::cout << "✅ 密钥生成成功" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "密钥生成错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::encryptFile(const std::string& file_path, 
                                const std::vector<std::string>& keywords,
                                const std::string& output_prefix) {
    if (!initialized_) {
        std::cerr << "客户端未初始化" << std::endl;
        return false;
    }
    
    try {
        std::cout << "加密文件: " << file_path << std::endl;
        
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
        Json::Value keyword_data;
        
        for (const auto& keyword : keywords) {
            std::string search_token = encryptKeyword(keyword);
            std::string current_state = generateRandomState();
            std::string previous_state;
            
            if (keyword_states_.find(keyword) != keyword_states_.end()) {
                previous_state = keyword_states_[keyword];
            }
            
            keyword_states_[keyword] = current_state;
            
            std::string state_token = computeHashH3(search_token + current_state);
            
            std::string pointer;
            if (previous_state.empty()) {
                pointer = encryptPointer(current_state, computeHashH3(current_state));
            } else {
                pointer = encryptPointer(previous_state, computeHashH3(current_state));
            }
            
            std::string kt;
            if (!generateKeywordTag(file_id, search_token, current_state, previous_state, kt)) {
                std::cerr << "关键词标签生成失败" << std::endl;
                return false;
            }
            
            Json::Value keyword_entry;
            keyword_entry["kt"] = kt;
            keyword_entry["state_token"] = state_token;
            keyword_entry["pointer"] = pointer;
            keyword_entry["current_state"] = current_state;
            
            keyword_data[keyword] = keyword_entry;
            
            // ============ 新增：更新关键词状态文件 ============
            if (states_loaded_) {
                updateKeywordState(keyword, current_state, file_id);
            }
        }
        
        std::cout << "生成了 " << keywords.size() << " 个关键词标签" << std::endl;
        
        // ============ 新增：保存状态文件 ============
        if (states_loaded_ && !keyword_states_file_.empty()) {
            if (saveKeywordStates(keyword_states_file_)) {
                std::cout << "✅ 关键词状态已更新到: " << keyword_states_file_ << std::endl;
            }
        }
        
        // 保存加密文件
        std::string encrypted_file = output_prefix + ".enc";
        if (!writeFile(encrypted_file, ciphertext)) {
            std::cerr << "加密文件保存失败" << std::endl;
            return false;
        }
        
        // 创建元数据
        Json::Value metadata;
        metadata["file_id"] = file_id;
        metadata["original_filename"] = file_path;
        metadata["encrypted_filename"] = encrypted_file;
        metadata["file_size"] = (Json::UInt64)plaintext.size();
        metadata["timestamp"] = (Json::UInt64)time(nullptr);
        
        Json::Value keywords_array(Json::arrayValue);
        for (const auto& kw : keywords) {
            keywords_array.append(kw);
        }
        metadata["keywords"] = keywords_array;
        
        Json::Value auth_tags_array(Json::arrayValue);
        for (const auto& tag : auth_tags) {
            auth_tags_array.append(tag);
        }
        metadata["auth_tags"] = auth_tags_array;
        
        metadata["keyword_tags"] = keyword_data;
        
        // 保存元数据
        std::string metadata_file = output_prefix + ".json";
        std::ofstream meta_out(metadata_file);
        if (!meta_out.is_open()) {
            std::cerr << "元数据文件保存失败" << std::endl;
            return false;
        }
        
        Json::StyledWriter writer;
        meta_out << writer.write(metadata);
        meta_out.close();
        
        std::cout << "\n✅ 文件加密成功！" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "📦 加密文件: " << encrypted_file << std::endl;
        std::cout << "📋 元数据:   " << metadata_file << std::endl;
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
        std::cerr << "文件解密错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::generateSearchToken(const std::string& keyword,
                                        const std::string& output_file) {
    if (!initialized_) {
        std::cerr << "客户端未初始化" << std::endl;
        return false;
    }
    
    try {
        std::cout << "生成搜索令牌: " << keyword << std::endl;
        
        std::string search_token = encryptKeyword(keyword);
        
        std::string latest_state;
        if (keyword_states_.find(keyword) != keyword_states_.end()) {
            latest_state = keyword_states_[keyword];
        }
        
        std::string random_seed = generateRandomState();
        
        Json::Value token_data;
        token_data["keyword"] = keyword;
        token_data["search_token"] = search_token;
        token_data["latest_state"] = latest_state;
        token_data["seed"] = random_seed;
        token_data["timestamp"] = (Json::UInt64)time(nullptr);
        
        std::ofstream out(output_file);
        if (!out.is_open()) {
            std::cerr << "无法创建输出文件" << std::endl;
            return false;
        }
        
        Json::StyledWriter writer;
        out << writer.write(token_data);
        out.close();
        
        std::cout << "✅ 搜索令牌已生成" << std::endl;
        std::cout << "保存到: " << output_file << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "搜索令牌生成错误: " << e.what() << std::endl;
        return false;
    }
}

// ============ 新增：关键词状态管理功能实现 ============

bool StorageClient::loadKeywordStates(const std::string& file_path) {
    try {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "无法打开状态文件: " << file_path << std::endl;
            std::cerr << "💡 提示: 如果是首次使用，状态文件会在加密文件后自动创建" << std::endl;
            return false;
        }
        
        Json::Reader reader;
        if (!reader.parse(file, keyword_states_data_)) {
            std::cerr << "JSON解析失败: " << reader.getFormattedErrorMessages() << std::endl;
            file.close();
            return false;
        }
        file.close();
        
        // 加载当前状态到内存映射中
        if (keyword_states_data_.isMember("keywords")) {
            const Json::Value& keywords = keyword_states_data_["keywords"];
            keyword_states_.clear();
            
            for (auto it = keywords.begin(); it != keywords.end(); ++it) {
                std::string keyword = it.key().asString();
                if ((*it).isMember("current_state")) {
                    keyword_states_[keyword] = (*it)["current_state"].asString();
                }
            }
        }
        
        keyword_states_file_ = file_path;
        states_loaded_ = true;
        
        int keyword_count = keyword_states_.size();
        std::cout << "✅ 状态文件加载成功" << std::endl;
        std::cout << "   文件路径: " << file_path << std::endl;
        std::cout << "   已加载关键词数量: " << keyword_count << std::endl;
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "状态文件加载错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::saveKeywordStates(const std::string& file_path) {
    try {
        // 更新元数据
        keyword_states_data_["version"] = "1.0";
        keyword_states_data_["last_updated"] = getCurrentTimestamp();
        
        std::ofstream file(file_path);
        if (!file.is_open()) {
            std::cerr << "无法创建状态文件: " << file_path << std::endl;
            return false;
        }
        
        Json::StyledWriter writer;
        file << writer.write(keyword_states_data_);
        file.close();
        
        keyword_states_file_ = file_path;
        
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
        // 确保keywords对象存在
        if (!keyword_states_data_.isMember("keywords")) {
            keyword_states_data_["keywords"] = Json::Value(Json::objectValue);
        }
        
        Json::Value& keywords = keyword_states_data_["keywords"];
        
        // 如果关键词不存在，创建新条目
        if (!keywords.isMember(keyword)) {
            keywords[keyword] = Json::Value(Json::objectValue);
            keywords[keyword]["history"] = Json::Value(Json::arrayValue);
        }
        
        // 更新当前状态
        keywords[keyword]["current_state"] = new_state;
        
        // 添加历史记录
        Json::Value history_entry;
        history_entry["state"] = new_state;
        history_entry["file_id"] = file_id;
        history_entry["timestamp"] = getCurrentTimestamp();
        history_entry["is_current"] = true;
        
        // 将之前的记录标记为非当前
        Json::Value& history = keywords[keyword]["history"];
        for (Json::Value::ArrayIndex i = 0; i < history.size(); i++) {
            history[i]["is_current"] = false;
        }
        
        // 添加新记录
        history.append(history_entry);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "状态更新错误: " << e.what() << std::endl;
        return false;
    }
}

std::string StorageClient::queryKeywordState(const std::string& keyword) {
    std::stringstream result;
    
    if (!states_loaded_) {
        result << "❌ 状态文件未加载，请先使用 load-states 命令加载状态文件";
        return result.str();
    }
    
    if (!keyword_states_data_.isMember("keywords") || 
        !keyword_states_data_["keywords"].isMember(keyword)) {
        result << "❌ 未找到关键词: " << keyword << "\n";
        result << "💡 该关键词可能尚未用于加密任何文件";
        return result.str();
    }
    
    const Json::Value& keyword_data = keyword_states_data_["keywords"][keyword];
    
    result << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    result << "🔍 关键词: " << keyword << "\n";
    result << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // 当前状态
    if (keyword_data.isMember("current_state")) {
        std::string current = keyword_data["current_state"].asString();
        result << "📌 当前状态:\n";
        result << "   " << current.substr(0, 32) << "..." << current.substr(current.length() - 8) << "\n\n";
    }
    
    // 历史记录
    if (keyword_data.isMember("history")) {
        const Json::Value& history = keyword_data["history"];
        result << "📜 历史记录 (共 " << history.size() << " 条):\n\n";
        
        for (Json::Value::ArrayIndex i = 0; i < history.size(); i++) {
            const Json::Value& entry = history[i];
            
            result << "   [" << (i + 1) << "] ";
            
            if (entry.isMember("is_current") && entry["is_current"].asBool()) {
                result << "✨ 最新 ";
            }
            
            result << "\n";
            
            if (entry.isMember("timestamp")) {
                result << "       时间: " << entry["timestamp"].asString() << "\n";
            }
            
            if (entry.isMember("file_id")) {
                std::string fid = entry["file_id"].asString();
                result << "       文件: " << fid.substr(0, 16) << "..." << "\n";
            }
            
            if (entry.isMember("state")) {
                std::string state = entry["state"].asString();
                result << "       状态: " << state.substr(0, 24) << "..." << "\n";
            }
            
            result << "\n";
        }
    }
    
    result << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    return result.str();
}

std::string StorageClient::getCurrentTimestamp() {
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", timeinfo);
    return std::string(buffer);
}

// ============ 原有功能实现（保持不变）============

bool StorageClient::encryptFileData(const std::vector<unsigned char>& plaintext,
                                    std::vector<unsigned char>& ciphertext) {
    try {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            std::cerr << "加密上下文创建失败" << std::endl;
            return false;
        }
        
        unsigned char iv[16];
        if (RAND_bytes(iv, sizeof(iv)) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::cerr << "IV生成失败" << std::endl;
            return false;
        }
        
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, ek_, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::cerr << "加密初始化失败" << std::endl;
            return false;
        }
        
        ciphertext.resize(16 + plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        
        memcpy(ciphertext.data(), iv, 16);
        
        int len;
        int ciphertext_len = 16;
        
        if (EVP_EncryptUpdate(ctx, ciphertext.data() + ciphertext_len, &len,
                             plaintext.data(), plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::cerr << "加密更新失败" << std::endl;
            return false;
        }
        ciphertext_len += len;
        
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + ciphertext_len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::cerr << "加密完成失败" << std::endl;
            return false;
        }
        ciphertext_len += len;
        
        ciphertext.resize(ciphertext_len);
        
        EVP_CIPHER_CTX_free(ctx);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "文件加密错误: " << e.what() << std::endl;
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
        
        unsigned char iv[16];
        memcpy(iv, ciphertext.data(), 16);
        
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) {
            std::cerr << "解密上下文创建失败" << std::endl;
            return false;
        }
        
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, ek_, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::cerr << "解密初始化失败" << std::endl;
            return false;
        }
        
        plaintext.resize(ciphertext.size());
        int len;
        int plaintext_len = 0;
        
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                             ciphertext.data() + 16, ciphertext.size() - 16) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::cerr << "解密更新失败" << std::endl;
            return false;
        }
        plaintext_len += len;
        
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + plaintext_len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            std::cerr << "解密完成失败" << std::endl;
            return false;
        }
        plaintext_len += len;
        
        plaintext.resize(plaintext_len);
        
        EVP_CIPHER_CTX_free(ctx);
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "文件解密错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::generateAuthTags(const std::string& file_id,
                                     const std::vector<unsigned char>& ciphertext,
                                     std::vector<std::string>& auth_tags) {
    try {
        std::vector<std::vector<unsigned char>> blocks = splitIntoBlocks(ciphertext, BLOCK_SIZE);
        
        auth_tags.clear();
        auth_tags.reserve(blocks.size() * SECTORS_PER_BLOCK);
        
        for (size_t i = 0; i < blocks.size(); i++) {
            std::vector<std::vector<unsigned char>> sectors = splitIntoBlocks(blocks[i], SECTOR_SIZE);
            
            for (size_t j = 0; j < sectors.size(); j++) {
                element_t tag;
                element_init_G1(tag, pairing_);
                
                std::stringstream sector_id;
                sector_id << file_id << "_block_" << i << "_sector_" << j;
                
                element_t h_elem;
                element_init_G1(h_elem, pairing_);
                computeHashH2(sector_id.str(), h_elem);
                
                mpz_t sector_value;
                mpz_init(sector_value);
                
                std::string sector_hex = bytesToHex(sectors[j]);
                mpz_set_str(sector_value, sector_hex.c_str(), 16);
                
                element_pow_mpz(tag, mu_, sector_value);
                element_mul(tag, tag, h_elem);
                element_pow_mpz(tag, tag, sk_);
                
                auth_tags.push_back(serializeElement(tag));
                
                element_clear(tag);
                element_clear(h_elem);
                mpz_clear(sector_value);
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "认证标签生成错误: " << e.what() << std::endl;
        return false;
    }
}

bool StorageClient::generateKeywordTag(const std::string& file_id,
                                       const std::string& keyword,
                                       const std::string& current_state,
                                       const std::string& previous_state,
                                       std::string& kt_output) {
    try {
        element_t kt;
        element_init_G1(kt, pairing_);
        
        std::string tag_input = keyword + file_id + current_state;
        element_t h_elem;
        element_init_G1(h_elem, pairing_);
        computeHashH2(tag_input, h_elem);
        
        element_pow_mpz(kt, h_elem, sk_);
        
        kt_output = serializeElement(kt);
        
        element_clear(kt);
        element_clear(h_elem);
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "关键词标签生成错误: " << e.what() << std::endl;
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

std::string StorageClient::encryptKeyword(const std::string& keyword) {
    try {
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return "";
        
        unsigned char iv[16];
        memset(iv, 0, sizeof(iv));
        
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, mk_, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        
        std::vector<unsigned char> ciphertext(keyword.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        int len, ciphertext_len = 0;
        
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                             (const unsigned char*)keyword.c_str(), keyword.size()) != 1) {
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
        std::cerr << "关键词加密错误: " << e.what() << std::endl;
        return "";
    }
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

std::string StorageClient::encryptPointer(const std::string& data, 
                                          const std::string& key) {
    try {
        unsigned char aes_key[32];
        memset(aes_key, 0, sizeof(aes_key));
        size_t key_len = std::min(key.size() / 2, (size_t)32);
        for (size_t i = 0; i < key_len; i++) {
            sscanf(key.substr(i * 2, 2).c_str(), "%02hhx", &aes_key[i]);
        }
        
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return "";
        
        unsigned char iv[16];
        memset(iv, 0, sizeof(iv));
        
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, aes_key, iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        
        std::vector<unsigned char> ciphertext(data.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
        int len, ciphertext_len = 0;
        
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                             (const unsigned char*)data.c_str(), data.size()) != 1) {
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