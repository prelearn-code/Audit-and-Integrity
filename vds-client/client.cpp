#include "client.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <random>
#include <filesystem>
#include <sys/stat.h>
#include <errno.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

namespace fs = std::filesystem;

// ============================================================================
// 构造函数和析构函数
// ============================================================================

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
}

// ============================================================================
// v4.1新增：目录管理功能
// ============================================================================

bool StorageClient::initializeDataDirectories() {
    std::cout << "\n[目录初始化] 检查并创建数据目录..." << std::endl;
    
    std::vector<std::string> dirs = {
        DATA_DIR,
        INSERT_DIR,
        DELETE_DIR,
        ENC_FILES_DIR,
        META_FILES_DIR,
        SEARCH_DIR
    };
    
    for (const auto& dir : dirs) {
        try {
            if (!fs::exists(dir)) {
                fs::create_directories(dir);
                std::cout << "[成功] 目录已创建: " << dir << std::endl;
            } else {
                std::cout << "[存在] 目录已就绪: " << dir << std::endl;
            }
        } catch (const fs::filesystem_error& e) {
            std::cerr << "[错误] 创建目录失败: " << dir 
                      << " - " << e.what() << std::endl;
            return false;
        }
    }
    
    // 检查或创建 keyword_states.json
    if (!fs::exists(KEYWORD_STATES_FILE)) {
        std::cout << "[初始化] 创建新的 keyword_states.json" << std::endl;
        Json::Value initial_data;
        initial_data["version"] = "v4.1";
        initial_data["keywords"] = Json::Value(Json::objectValue);
        
        std::ofstream states_file(KEYWORD_STATES_FILE);
        if (states_file.is_open()) {
            Json::StreamWriterBuilder writer;
            writer["indentation"] = "  ";
            states_file << Json::writeString(writer, initial_data);
            states_file.close();
            std::cout << "[成功] keyword_states.json 已创建" << std::endl;
        } else {
            std::cerr << "[错误] 无法创建 keyword_states.json" << std::endl;
            return false;
        }
    } else {
        std::cout << "[存在] keyword_states.json 已就绪" << std::endl;
    }
    
    // 自动加载状态文件
    keyword_states_file_ = KEYWORD_STATES_FILE;
    if (!loadKeywordStates(KEYWORD_STATES_FILE)) {
        std::cerr << "[警告] 无法加载状态文件，将创建新文件" << std::endl;
        keyword_states_data_ = Json::Value(Json::objectValue);
        keyword_states_data_["version"] = "v4.1";
        keyword_states_data_["keywords"] = Json::Value(Json::objectValue);
        states_loaded_ = true;
    }
    
    std::cout << "[完成] 数据目录初始化成功\n" << std::endl;
    return true;
}

std::string StorageClient::extractFileName(const std::string& file_path) {
    // 使用 C++17 filesystem 提取文件名
    fs::path path(file_path);
    return path.filename().string();
}

bool StorageClient::fileExists(const std::string& file_path) {
    return fs::exists(file_path);
}

std::string StorageClient::generateUniqueFilePath(const std::string& base_path, 
                                                  const std::string& filename) {
    std::string full_path = base_path + "/" + filename;
    
    if (!fileExists(full_path)) {
        return full_path;
    }
    
    // 文件存在，添加时间戳后缀
    std::cout << "[提示] 文件已存在: " << full_path << std::endl;
    std::cout << "        将添加时间戳后缀以避免覆盖" << std::endl;
    
    // 分离文件名和扩展名
    fs::path path(filename);
    std::string stem = path.stem().string();
    std::string extension = path.extension().string();
    
    // 生成时间戳
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* local_time = std::localtime(&time_t_now);
    
    std::ostringstream timestamp;
    timestamp << "_" 
              << (local_time->tm_year + 1900)
              << std::setw(2) << std::setfill('0') << (local_time->tm_mon + 1)
              << std::setw(2) << std::setfill('0') << local_time->tm_mday
              << "_"
              << std::setw(2) << std::setfill('0') << local_time->tm_hour
              << std::setw(2) << std::setfill('0') << local_time->tm_min
              << std::setw(2) << std::setfill('0') << local_time->tm_sec;
    
    std::string new_filename = stem + timestamp.str() + extension;
    std::string new_path = base_path + "/" + new_filename;
    
    std::cout << "[生成] 新文件名: " << new_filename << std::endl;
    
    return new_path;
}

// ============================================================================
// 初始化函数（v4.0重构 - 方案A实现）
// ============================================================================

bool StorageClient::initialize(const std::string& public_params_file) {
    std::cout << "\n[初始化] 开始初始化客户端..." << std::endl;
    
    // ========================================
    // 步骤1: 初始化配对参数（硬编码Type A曲线）
    // ========================================
    std::cout << "[初始化] 步骤1: 加载配对参数（Type A曲线，1024位安全级别）" << std::endl;
    
    // Type A曲线参数（对称配对，G₁ = G₂）
    // 这是标准的1024位安全级别参数，公开且固定
    // ⚠️ 必须与 Storage Node 保持完全一致
    const char* PAIRING_PARAMS = 
        "type a\n" // 类型
        //大素数
        "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
        "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
        "r 730750818665451621361119245571504901405976559617\n"
        "exp2 159\n"
        "exp1 107\n"
        "sign1 1\n"
        "sign0 1\n";
    
    if (pairing_init_set_str(pairing_, PAIRING_PARAMS) != 0) {
        std::cerr << "[错误] 配对参数初始化失败" << std::endl;
        return false;
    }
    std::cout << "[成功] 配对参数加载完成" << std::endl;
    
    // ========================================
    // 步骤2: 从 public_params.json 加载公共参数
    // ========================================
    std::cout << "[初始化] 步骤2: 从 " << public_params_file << " 加载公共参数" << std::endl;
    
    std::ifstream file(public_params_file);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法打开文件: " << public_params_file << std::endl;
        std::cerr << "[提示] 请确保已从 Storage Node 获取此文件" << std::endl;
        return false;
    }
    
    Json::Value params;
    Json::CharReaderBuilder reader;
    std::string errs;
    
    if (!Json::parseFromStream(reader, file, &params, &errs)) {
        std::cerr << "[错误] JSON解析失败: " << errs << std::endl;
        file.close();
        return false;
    }
    file.close();
    
    // ========================================
    // 步骤3: 获取 public_params 对象
    // ========================================
    Json::Value public_params;
    
    // 检查参数是否嵌套在 "public_params" 对象中
    if (params.isMember("public_params") && params["public_params"].isObject()) {
        std::cout << "[解析] 检测到嵌套的 public_params 对象" << std::endl;
        public_params = params["public_params"];
        
        // 显示额外的元信息（如果存在）
        if (params.isMember("version")) {
            std::cout << "[信息] 参数文件版本: " << params["version"].asString() << std::endl;
        }
        if (params.isMember("created_at")) {
            std::cout << "[信息] 创建时间: " << params["created_at"].asString() << std::endl;
        }
    } else {
        // 向后兼容：直接从根对象读取
        std::cout << "[解析] 使用根级参数（旧格式）" << std::endl;
        public_params = params;
    }
    
    // ========================================
    // 步骤4: 加载和验证N（RSA模数）
    // ========================================
    if (!public_params.isMember("N") || !public_params["N"].isString()) {
        std::cerr << "[错误] public_params 缺少 'N' 字段" << std::endl;
        return false;
    }
    
    std::string N_str = public_params["N"].asString();
    if (mpz_set_str(N_, N_str.c_str(), 10) != 0) {
        std::cerr << "[错误] N 参数格式错误" << std::endl;
        return false;
    }
    
    // 验证N是否足够大（至少2048位）
    size_t n_bits = mpz_sizeinbase(N_, 2);
    if (n_bits < 2048) {
        std::cerr << "[警告] N 的位数过小(" << n_bits << "位)，建议至少2048位" << std::endl;
    }
    std::cout << "[成功] N 加载完成 (" << n_bits << " 位)" << std::endl;
    
    // ========================================
    // 步骤5: 加载和验证g（生成元）
    // ========================================
    if (!public_params.isMember("g") || !public_params["g"].isString()) {
        std::cerr << "[错误] public_params 缺少 'g' 字段" << std::endl;
        return false;
    }
    
    std::string g_hex = public_params["g"].asString();
    element_init_G1(g_, pairing_);
    
    if (!deserializeElement(g_hex, g_)) {
        std::cerr << "[错误] g 反序列化失败" << std::endl;
        element_clear(g_);
        return false;
    }
    
    // 验证g是否为单位元（应该不是）
    if (element_is1(g_)) {
        std::cerr << "[错误] g 不能是单位元" << std::endl;
        element_clear(g_);
        return false;
    }
    std::cout << "[成功] g 加载完成" << std::endl;
    
    // ========================================
    // 步骤6: 加载和验证μ（认证参数）
    // ========================================
    if (!public_params.isMember("mu") || !public_params["mu"].isString()) {
        std::cerr << "[错误] public_params 缺少 'mu' 字段" << std::endl;
        element_clear(g_);
        return false;
    }
    
    std::string mu_hex = public_params["mu"].asString();
    element_init_G1(mu_, pairing_);
    
    if (!deserializeElement(mu_hex, mu_)) {
        std::cerr << "[错误] μ 反序列化失败" << std::endl;
        element_clear(g_);
        element_clear(mu_);
        return false;
    }
    
    // 验证μ是否为单位元（应该不是）
    if (element_is1(mu_)) {
        std::cerr << "[错误] μ 不能是单位元" << std::endl;
        element_clear(g_);
        element_clear(mu_);
        return false;
    }
    std::cout << "[成功] μ 加载完成" << std::endl;
    
    // ========================================
    // 步骤7: 初始化公钥元素（但不计算值）
    // ========================================
    element_init_G1(pk_, pairing_);
    
    initialized_ = true;
    std::cout << "[完成] 客户端初始化成功" << std::endl;
    std::cout << "        配对参数: Type A (硬编码)" << std::endl;
    std::cout << "        公共参数: " << public_params_file << std::endl;
    std::cout << "        参数来源: Storage Node" << std::endl;
    
    return true;
}

// ============================================================================
// 密钥生成函数（v4.0简化 - 方案A实现）
// ============================================================================

bool StorageClient::generateKeys() {
    std::cout << "\n[密钥生成] 开始生成客户端密钥..." << std::endl;
    
    // ========================================
    // 检查初始化状态
    // ========================================
    if (!initialized_) {
        std::cerr << "[错误] 系统尚未初始化" << std::endl;
        std::cerr << "[提示] 请先调用 initialize() 函数" << std::endl;
        return false;
    }
    
    // ========================================
    // 步骤1: 生成主密钥 mk（256位随机数）
    // ========================================
    std::cout << "[密钥生成] 步骤1: 生成主密钥 mk (256位)" << std::endl;
    if (RAND_bytes(mk_, 32) != 1) {
        std::cerr << "[错误] 随机数生成失败" << std::endl;
        return false;
    }
    std::cout << "[成功] mk 生成完成" << std::endl;
    
    // ========================================
    // 步骤2: 生成加密密钥 ek（256位随机数）
    // ========================================
    std::cout << "[密钥生成] 步骤2: 生成加密密钥 ek (256位)" << std::endl;
    if (RAND_bytes(ek_, 32) != 1) {
        std::cerr << "[错误] 随机数生成失败" << std::endl;
        return false;
    }
    std::cout << "[成功] ek 生成完成" << std::endl;
    
    // ========================================
    // 步骤3: 生成私钥 sk（随机数 ∈ Z_r）
    // ========================================
    std::cout << "[密钥生成] 步骤3: 生成私钥 sk (随机大整数)" << std::endl;
    
    // 生成随机元素并提取其整数表示
    element_t temp;
    element_init_Zr(temp, pairing_);
    element_random(temp);
    element_to_mpz(sk_, temp);
    element_clear(temp);
    
    std::cout << "[成功] sk 生成完成 (" << mpz_sizeinbase(sk_, 10) << " 位)" << std::endl;
    
    // ========================================
    // 步骤4: 计算公钥 pk = g^sk
    // ========================================
    std::cout << "[密钥生成] 步骤4: 计算公钥 pk = g^sk" << std::endl;
    
    element_pow_mpz(pk_, g_, sk_);
    
    // 验证pk不是单位元
    if (element_is1(pk_)) {
        std::cerr << "[错误] 公钥计算错误（不应为单位元）" << std::endl;
        return false;
    }
    std::cout << "[成功] pk 计算完成" << std::endl;
    
    // ========================================
    // 步骤5: 保存私钥到 private_key.dat（二进制）
    // ========================================
    std::cout << "[密钥生成] 步骤5: 保存私钥到 private_key.dat" << std::endl;
    
    std::ofstream priv_file("private_key.dat", std::ios::binary);
    if (!priv_file.is_open()) {
        std::cerr << "[错误] 无法创建 private_key.dat" << std::endl;
        return false;
    }
    
    // 写入mk
    priv_file.write(reinterpret_cast<const char*>(mk_), 32);
    
    // 写入ek
    priv_file.write(reinterpret_cast<const char*>(ek_), 32);
    
    // 写入sk（使用GMP导出）
    size_t sk_size = (mpz_sizeinbase(sk_, 2) + 7) / 8;
    std::vector<unsigned char> sk_buf(sk_size);
    mpz_export(sk_buf.data(), nullptr, 1, 1, 0, 0, sk_);
    
    uint32_t size_marker = static_cast<uint32_t>(sk_size);
    priv_file.write(reinterpret_cast<const char*>(&size_marker), 4);
    priv_file.write(reinterpret_cast<const char*>(sk_buf.data()), sk_size);
    
    priv_file.close();
    std::cout << "[成功] 私钥已保存" << std::endl;
    
    // ========================================
    // 步骤6: 保存公钥到 public_key.json
    // ========================================
    std::cout << "[密钥生成] 步骤6: 保存公钥到 public_key.json" << std::endl;
    
    Json::Value pub_key_json;
    pub_key_json["pk"] = serializeElement(pk_);
    pub_key_json["timestamp"] = getCurrentTimestamp();
    pub_key_json["version"] = "v4.1";
    pub_key_json["note"] = "Public key generated by StorageClient v4.1";
    
    std::ofstream pub_file("public_key.json");
    if (!pub_file.is_open()) {
        std::cerr << "[错误] 无法创建 public_key.json" << std::endl;
        return false;
    }
    
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    pub_file << Json::writeString(writer, pub_key_json);
    pub_file.close();
    
    std::cout << "[成功] 公钥已保存" << std::endl;
    
    // ========================================
    // 完成
    // ========================================
    std::cout << "[完成] 密钥生成成功" << std::endl;
    std::cout << "        私钥: private_key.dat (请妥善保管)" << std::endl;
    std::cout << "        公钥: public_key.json" << std::endl;
    
    return true;
}

// ============================================================================
// 密钥保存和加载（v4.0增强）
// ============================================================================

bool StorageClient::saveKeys(const std::string& key_file) {
    if (!initialized_) {
        std::cerr << "[错误] 系统尚未初始化" << std::endl;
        return false;
    }
    
    std::ofstream file(key_file, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法创建文件: " << key_file << std::endl;
        return false;
    }
    
    // 写入mk
    file.write(reinterpret_cast<const char*>(mk_), 32);
    
    // 写入ek
    file.write(reinterpret_cast<const char*>(ek_), 32);
    
    // 写入sk
    size_t sk_size = (mpz_sizeinbase(sk_, 2) + 7) / 8;
    std::vector<unsigned char> sk_buf(sk_size);
    mpz_export(sk_buf.data(), nullptr, 1, 1, 0, 0, sk_);
    
    uint32_t size_marker = static_cast<uint32_t>(sk_size);
    file.write(reinterpret_cast<const char*>(&size_marker), 4);
    file.write(reinterpret_cast<const char*>(sk_buf.data()), sk_size);
    
    // 写入pk
    std::string pk_str = serializeElement(pk_);
    uint32_t pk_size = static_cast<uint32_t>(pk_str.size());
    file.write(reinterpret_cast<const char*>(&pk_size), 4);
    file.write(pk_str.c_str(), pk_str.size());
    
    file.close();
    return true;
}

bool StorageClient::loadKeys(const std::string& key_file) {
    // ========================================
    // v4.0新增检查：确保已初始化
    // ========================================
    if (!initialized_) {
        std::cerr << "[错误] 系统尚未初始化" << std::endl;
        std::cerr << "[提示] 请先调用 initialize() 加载公共参数" << std::endl;
        std::cerr << "        原因: 加载密钥需要先加载配对参数和公共参数" << std::endl;
        return false;
    }
    
    std::ifstream file(key_file, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法打开文件: " << key_file << std::endl;
        return false;
    }
    
    // 读取mk
    file.read(reinterpret_cast<char*>(mk_), 32);
    
    // 读取ek
    file.read(reinterpret_cast<char*>(ek_), 32);
    
    // 读取sk
    uint32_t sk_size;
    file.read(reinterpret_cast<char*>(&sk_size), 4);
    
    std::vector<unsigned char> sk_buf(sk_size);
    file.read(reinterpret_cast<char*>(sk_buf.data()), sk_size);
    mpz_import(sk_, sk_size, 1, 1, 0, 0, sk_buf.data());
    
    // 读取pk
    uint32_t pk_size;
    file.read(reinterpret_cast<char*>(&pk_size), 4);
    
    std::vector<char> pk_buf(pk_size);
    file.read(pk_buf.data(), pk_size);
    std::string pk_str(pk_buf.begin(), pk_buf.end());
    
    if (!deserializeElement(pk_str, pk_)) {
        std::cerr << "[错误] 公钥反序列化失败" << std::endl;
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

std::string StorageClient::getPublicKey() {
    if (!initialized_) {
        return "";
    }
    return serializeElement(pk_);
}

// ============================================================================
// 文件加密功能（v4.1重构）
// ============================================================================

bool StorageClient::encryptFile(const std::string& file_path,
                               const std::vector<std::string>& keywords) {
    if (!initialized_) {
        std::cerr << "[错误] 系统尚未初始化" << std::endl;
        return false;
    }
    
    std::cout << "\n[文件加密] 开始加密文件: " << file_path << std::endl;
    
    // 提取原始文件名
    std::string original_filename = extractFileName(file_path);
    std::cout << "[加密] 原始文件名: " << original_filename << std::endl;
    
    // 读取文件
    std::vector<unsigned char> plaintext;
    if (!readFile(file_path, plaintext)) {
        return false;
    }
    std::cout << "[加密] 文件大小: " << plaintext.size() << " 字节" << std::endl;
    
    // 加密文件数据
    std::vector<unsigned char> ciphertext;
    if (!encryptFileData(plaintext, ciphertext)) {
        std::cerr << "[错误] 文件数据加密失败" << std::endl;
        return false;
    }
    std::cout << "[加密] 密文大小: " << ciphertext.size() << " 字节" << std::endl;
    
    // 计算文件ID
    mpz_t file_id_int;
    mpz_init(file_id_int);
    std::string ciphertext_str(ciphertext.begin(), ciphertext.end());
    computeHashH1(ciphertext_str, file_id_int);
    char* file_id_cstr = mpz_get_str(nullptr, 10, file_id_int);
    std::string file_id(file_id_cstr);
    free(file_id_cstr);
    mpz_clear(file_id_int);
    std::cout << "[加密] 文件ID (H1(C)): " << file_id.substr(0, 32) << "..." << std::endl;
    
    // ========== v4.1修改：使用新的目录结构和唯一文件名 ==========
    
    // 1. 生成唯一的加密文件路径
    std::string enc_filename = original_filename + ".enc";
    std::string enc_file = generateUniqueFilePath(ENC_FILES_DIR, enc_filename);
    
    if (!writeFile(enc_file, ciphertext)) {
        std::cerr << "[错误] 无法保存加密文件: " << enc_file << std::endl;
        std::cerr << "       请检查:" << std::endl;
        std::cerr << "       1. 目录权限" << std::endl;
        std::cerr << "       2. 磁盘空间" << std::endl;
        return false;
    }
    std::cout << "[成功] 加密文件已保存: " << enc_file << std::endl;
    
    // 生成认证标签
    std::vector<std::string> auth_tags;
    if (!generateAuthTags(file_id, ciphertext, auth_tags)) {
        std::cerr << "[错误] 认证标签生成失败" << std::endl;
        return false;
    }
    std::cout << "[加密] 认证标签数量: " << auth_tags.size() << std::endl;
    
    // 处理关键词数据
    Json::Value keywords_data(Json::arrayValue);
    for (const auto& keyword : keywords) {
        Json::Value kw_obj;
        
        // 生成搜索令牌
        std::string Ti = generateSearchToken(keyword);
        
        // 获取前一个状态的最新状态，通过map映射
        std::string previous_state;
        auto it = keyword_states_.find(keyword);
        if (it != keyword_states_.end()) {
            previous_state = it->second;
        }
        
        // 生成新状态
        std::string new_state = generateRandomState();
        
        if(previous_state == "")  previous_state=new_state;
        // 计算状态链ptr
        
        std::string ptr = encryptPointer(computeHashH3(new_state), previous_state);
        kw_obj["ptr_i"] = ptr;
        
        // 生成状态关联令牌
        std::string Ti_bar = generateStateAssociatedToken(Ti, new_state);
        kw_obj["Ti_bar"] = Ti_bar;
        
        // 生成关键词关联标签
        std::string kt;
        if (!generateKeywordAssociatedTag(file_id, Ti, new_state, previous_state, kt)) {
            std::cerr << "[错误] 状态关联令牌生成失败" << std::endl;
            return false;
        }
        kw_obj["kt_wi"] = kt;
        
        keywords_data.append(kw_obj);
        
        // 更新状态存储（会自动保存到 ./data/keyword_states.json）
        if (!updateKeywordState(keyword, new_state, file_id)) {
            std::cerr << "[警告] 状态更新失败" << std::endl;
        }
    }
    
    // 2. 构建并保存 insert.json 到 Insert 目录
    Json::Value insert_json;
    insert_json["PK"] = getPublicKey();
    insert_json["ID_F"] = file_id;
    
    Json::Value ts_f_array(Json::arrayValue);
    for (const auto& tag : auth_tags) {
        ts_f_array.append(tag);
    }
    insert_json["TS_F"] = ts_f_array;
    insert_json["state"] = "valid";
    insert_json["keywords"] = keywords_data;
    
    // 使用与加密文件相同的基础名生成insert文件名
    fs::path enc_path(enc_file);
    std::string base_name = enc_path.stem().string(); // 移除.enc扩展名
    std::string insert_filename = base_name + "_insert.json";
    std::string insert_json_path = INSERT_DIR + "/" + insert_filename;
    
    std::ofstream insert_file(insert_json_path);
    if (!insert_file.is_open()) {
        std::cerr << "[错误] 无法创建 " << insert_json_path << std::endl;
        return false;
    }
    
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    insert_file << Json::writeString(writer, insert_json);
    insert_file.close();
    std::cout << "[成功] insert.json 已生成: " << insert_json_path << std::endl;
    
    // 3. 生成并保存元数据到 MetaFiles 目录
    Json::Value metadata;
    metadata["file_id"] = file_id;
    metadata["original_file"] = file_path;
    metadata["encrypted_file"] = enc_file;
    metadata["keywords"] = Json::Value(Json::arrayValue);
    for (const auto& kw : keywords) {
        metadata["keywords"].append(kw);
    }
    metadata["timestamp"] = getCurrentTimestamp();
    
    std::string metadata_filename = base_name + "_metadata.json";
    std::string metadata_file = META_FILES_DIR + "/" + metadata_filename;
    
    std::ofstream meta_file(metadata_file);
    if (meta_file.is_open()) {
        meta_file << Json::writeString(writer, metadata);
        meta_file.close();
        std::cout << "[成功] 元数据已保存: " << metadata_file << std::endl;
    }
    
    std::cout << "\n[完成] 文件加密成功" << std::endl;
    std::cout << "📦 生成的文件:" << std::endl;
    std::cout << "   - " << enc_file << std::endl;
    std::cout << "   - " << insert_json_path << std::endl;
    std::cout << "   - " << metadata_file << std::endl;
    std::cout << "   - " << KEYWORD_STATES_FILE << " (已自动更新)" << std::endl;
    
    return true;
}

// ============================================================================
// 文件解密功能
// ============================================================================

bool StorageClient::decryptFile(const std::string& encrypted_file,
                               const std::string& output_path) {
    if (!initialized_) {
        std::cerr << "[错误] 系统尚未初始化" << std::endl;
        return false;
    }
    
    std::cout << "\n[文件解密] 开始解密文件: " << encrypted_file << std::endl;
    
    // 读取加密文件
    std::vector<unsigned char> ciphertext;
    if (!readFile(encrypted_file, ciphertext)) {
        return false;
    }
    std::cout << "[解密] 密文大小: " << ciphertext.size() << " 字节" << std::endl;
    
    // 解密文件数据
    std::vector<unsigned char> plaintext;
    if (!decryptFileData(ciphertext, plaintext)) {
        std::cerr << "[错误] 文件解密失败" << std::endl;
        return false;
    }
    std::cout << "[解密] 明文大小: " << plaintext.size() << " 字节" << std::endl;
    
    // 保存解密文件
    if (!writeFile(output_path, plaintext)) {
        return false;
    }
    
    std::cout << "[完成] 文件解密成功: " << output_path << std::endl;
    return true;
}

// ============================================================================
// 密码学操作 - 数据加密/解密
// ============================================================================

bool StorageClient::encryptFileData(const std::vector<unsigned char>& plaintext,
                                   std::vector<unsigned char>& ciphertext) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "[错误] 无法创建加密上下文" << std::endl;
        return false;
    }
    
    // 使用AES-256-CBC加密
    unsigned char iv[16];
    if (RAND_bytes(iv, 16) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, ek_, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    // 预留空间（IV + 密文 + padding）
    ciphertext.resize(16 + plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    
    // 写入IV
    std::memcpy(ciphertext.data(), iv, 16);
    
    int len = 0;
    int ciphertext_len = 0;
    
    if (EVP_EncryptUpdate(ctx, ciphertext.data() + 16, &len,
                         plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + 16 + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    ciphertext_len += len;
    
    ciphertext.resize(16 + ciphertext_len);
    EVP_CIPHER_CTX_free(ctx);
    
    return true;
}

bool StorageClient::decryptFileData(const std::vector<unsigned char>& ciphertext,
                                   std::vector<unsigned char>& plaintext) {
    if (ciphertext.size() < 16) {
        std::cerr << "[错误] 密文长度不足" << std::endl;
        return false;
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cerr << "[错误] 无法创建解密上下文" << std::endl;
        return false;
    }
    
    // 提取IV
    unsigned char iv[16];
    std::memcpy(iv, ciphertext.data(), 16);
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, ek_, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    
    plaintext.resize(ciphertext.size() - 16);
    
    int len = 0;
    int plaintext_len = 0;
    
    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len,
                         ciphertext.data() + 16, ciphertext.size() - 16) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len = len;
    
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    plaintext_len += len;
    
    plaintext.resize(plaintext_len);
    EVP_CIPHER_CTX_free(ctx);
    
    return true;
}

// ============================================================================
// 密码学操作 - 认证标签生成
// ============================================================================

bool StorageClient::generateAuthTags(const std::string& file_id,
                                    const std::vector<unsigned char>& ciphertext,
                                    std::vector<std::string>& auth_tags) {
    // 将密文分块
    auto blocks = splitIntoBlocks(ciphertext, BLOCK_SIZE);
    auth_tags.clear();
    
    for (size_t i = 0; i < blocks.size(); ++i) {
        // σ_i = [H_2(ID_F||i) * ∏_{j=1}^s μ^{c_{i,j}}]^sk
        
        element_t sigma;
        element_init_G1(sigma, pairing_);
        
        // 计算 H_2(ID_F||i)
        std::string hash_input = file_id + std::to_string(i);
        element_t h2_result;
        element_init_G1(h2_result, pairing_);
        computeHashH2(hash_input, h2_result);
        
        element_set(sigma, h2_result);
        
        // 计算 ∏_{j=1}^s μ^{c_{i,j}}
        auto sectors = splitIntoBlocks(blocks[i], SECTOR_SIZE);
        
        for (size_t j = 0; j < sectors.size(); ++j) {
            // 将扇区数据转换为整数
            mpz_t c_ij;
            mpz_init(c_ij);
            mpz_import(c_ij, sectors[j].size(), 1, 1, 0, 0, sectors[j].data());
            
            // 计算 μ^{c_{i,j}}
            element_t mu_power;
            element_init_G1(mu_power, pairing_);
            element_pow_mpz(mu_power, mu_, c_ij);
            
            // 累乘到sigma
            element_mul(sigma, sigma, mu_power);
            
            element_clear(mu_power);
            mpz_clear(c_ij);
        }
        
        // 计算 [...]^sk
        element_t final_sigma;
        element_init_G1(final_sigma, pairing_);
        element_pow_mpz(final_sigma, sigma, sk_);
        
        auth_tags.push_back(serializeElement(final_sigma));
        
        element_clear(final_sigma);
        element_clear(sigma);
        element_clear(h2_result);
    }
    
    return true;
}

// ============================================================================
// 密码学操作 - 状态关联令牌生成
// ============================================================================

bool StorageClient::generateKeywordAssociatedTag(const std::string& file_id,
                                                const std::string& Ti,
                                                const std::string& current_state,
                                                const std::string& previous_state,
                                                std::string& kt_output) {
    element_t kt;
    element_init_G1(kt, pairing_);
    
    // 计算 H_2(ID_F)
    element_t h2_id;
    element_init_G1(h2_id, pairing_);
    computeHashH2(file_id, h2_id);
    
    element_set(kt, h2_id);
    
    // 计算 H_2(st_d||Ti)
    element_t h2_current;
    element_init_G1(h2_current, pairing_);
    computeHashH2(current_state + Ti, h2_current);
    
    element_mul(kt, kt, h2_current);
    
    if (!previous_state.empty()) {
        // 有前一状态: 除以 H_2(st_{d-1}||Ti)
        element_t h2_previous;
        element_init_G1(h2_previous, pairing_);
        computeHashH2(previous_state + Ti, h2_previous);
        
        // 计算逆元
        element_t h2_prev_inv;
        element_init_G1(h2_prev_inv, pairing_);
        element_invert(h2_prev_inv, h2_previous);
        
        element_mul(kt, kt, h2_prev_inv);
        
        element_clear(h2_prev_inv);
        element_clear(h2_previous);
    }
    
    // 计算 [...]^sk
    element_t final_kt;
    element_init_G1(final_kt, pairing_);
    element_pow_mpz(final_kt, kt, sk_);
    
    kt_output = serializeElement(final_kt);
    
    element_clear(final_kt);
    element_clear(kt);
    element_clear(h2_current);
    element_clear(h2_id);
    
    return true;
}

// ============================================================================
// 哈希函数实现
// ============================================================================

void StorageClient::computeHashH1(const std::string& input, mpz_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    mpz_import(result, SHA256_DIGEST_LENGTH, 1, 1, 0, 0, hash);
    mpz_mod(result, result, N_);
}

void StorageClient::computeHashH2(const std::string& input, element_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    element_from_hash(result, hash, SHA256_DIGEST_LENGTH);
}

std::string StorageClient::computeHashH3(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string StorageClient::generateSearchToken(const std::string& keyword) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return "";
    }
    
    // 使用AES-256-ECB加密关键词
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ecb(), nullptr, mk_, nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    
    // 准备输入（填充到16字节的倍数）
    std::vector<unsigned char> input(keyword.begin(), keyword.end());
    size_t padded_size = ((input.size() + 15) / 16) * 16;
    input.resize(padded_size, 0);
    
    std::vector<unsigned char> output(padded_size + EVP_CIPHER_block_size(EVP_aes_256_ecb()));
    int len = 0;
    int total_len = 0;
    
    if (EVP_EncryptUpdate(ctx, output.data(), &len, input.data(), input.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, output.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len += len;
    
    output.resize(total_len);
    EVP_CIPHER_CTX_free(ctx);
    
    return bytesToHex(output);
}

std::string StorageClient::generateRandomState() {
    unsigned char random_bytes[32];
    if (RAND_bytes(random_bytes, 32) != 1) {
        return "";
    }
    
    std::ostringstream oss;
    for (int i = 0; i < 32; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(random_bytes[i]);
    }
    return oss.str();
}

std::string StorageClient::encryptPointer(const std::string& current_state_hash,
                                         const std::string& previous_state) {
    if (previous_state.empty()) {
        // 第一个状态，返回空指针
        return std::string(64, '0');
    }
    
    // 使用当前状态的哈希作为密钥加密前一个状态
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return "";
    }
    
    // 从哈希中提取前32字节作为AES密钥
    unsigned char key[32];
    for (size_t i = 0; i < 32 && i * 2 < current_state_hash.length(); ++i) {
        std::string byte_str = current_state_hash.substr(i * 2, 2);
        key[i] = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
    }
    
    unsigned char iv[16] = {0}; // 使用零IV
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    
    std::vector<unsigned char> plaintext(previous_state.begin(), previous_state.end());
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    
    int len = 0;
    int total_len = 0;
    
    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len += len;
    
    ciphertext.resize(total_len);
    EVP_CIPHER_CTX_free(ctx);
    
    return bytesToHex(ciphertext);
}

std::string StorageClient::generateStateAssociatedToken(const std::string& Ti, 
                                                       const std::string& st_d) {
    // 计算 H_2(Ti||st_d)
    element_t result;
    element_init_G1(result, pairing_);
    computeHashH2(Ti + st_d, result);
    
    std::string serialized = serializeElement(result);
    element_clear(result);
    
    return serialized;
}

// ============================================================================
// 关键词状态管理（v4.1修改）
// ============================================================================

bool StorageClient::loadKeywordStates(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法打开状态文件: " << file_path << std::endl;
        return false;
    }
    
    Json::CharReaderBuilder reader;
    std::string errs;
    
    if (!Json::parseFromStream(reader, file, &keyword_states_data_, &errs)) {
        std::cerr << "[错误] JSON解析失败: " << errs << std::endl;
        file.close();
        return false;
    }
    file.close();
    
    // 加载当前状态映射
    keyword_states_.clear();
    if (keyword_states_data_.isMember("keywords")) {
        const Json::Value& keywords = keyword_states_data_["keywords"];
        for (const auto& key : keywords.getMemberNames()) {
            if (keywords[key].isMember("current_state")) {
                keyword_states_[key] = keywords[key]["current_state"].asString();
            }
        }
    }
    
    keyword_states_file_ = file_path;
    states_loaded_ = true;
    
    std::cout << "[状态管理] 已加载 " << keyword_states_.size() << " 个关键词状态" << std::endl;
    return true;
}

bool StorageClient::saveKeywordStates(const std::string& file_path) {
    if (!states_loaded_ && keyword_states_data_.isNull()) {
        keyword_states_data_ = Json::Value(Json::objectValue);
        keyword_states_data_["keywords"] = Json::Value(Json::objectValue);
        keyword_states_data_["version"] = "v4.1";
    }
    
    std::ofstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法创建文件: " << file_path << std::endl;
        return false;
    }
    
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "  ";
    file << Json::writeString(writer, keyword_states_data_);
    file.close();
    
    keyword_states_file_ = file_path;
    states_loaded_ = true;
    
    return true;
}

bool StorageClient::updateKeywordState(const std::string& keyword,
                                      const std::string& new_state,
                                      const std::string& file_id) {
    if (keyword_states_data_.isNull()) {
        keyword_states_data_ = Json::Value(Json::objectValue);
        keyword_states_data_["keywords"] = Json::Value(Json::objectValue);
        keyword_states_data_["version"] = "v4.1";
    }
    
    Json::Value& keywords = keyword_states_data_["keywords"];
    
    if (!keywords.isMember(keyword)) {
        keywords[keyword] = Json::Value(Json::objectValue);
        keywords[keyword]["history"] = Json::Value(Json::arrayValue);
    }
    
    // 更新当前状态
    keywords[keyword]["current_state"] = new_state;
    keywords[keyword]["last_update"] = getCurrentTimestamp();
    
    // 添加历史记录
    Json::Value history_entry;
    history_entry["state"] = new_state;
    history_entry["file_id"] = file_id;
    history_entry["timestamp"] = getCurrentTimestamp();
    keywords[keyword]["history"].append(history_entry);
    
    // 更新内存映射
    keyword_states_[keyword] = new_state;
    
    // ========== v4.1修改：始终保存到固定位置 ==========
    return saveKeywordStates(KEYWORD_STATES_FILE);
}

std::string StorageClient::queryKeywordState(const std::string& keyword) {
    std::ostringstream oss;
    
    if (keyword_states_data_.isNull() || 
        !keyword_states_data_["keywords"].isMember(keyword)) {
        oss << "\n[查询结果] 关键词 \"" << keyword << "\" 未找到" << std::endl;
        oss << "            可能尚未加密包含此关键词的文件" << std::endl;
        return oss.str();
    }
    
    const Json::Value& kw_data = keyword_states_data_["keywords"][keyword];
    
    oss << "\n[查询结果] 关键词: " << keyword << std::endl;
    oss << "============================================" << std::endl;
    oss << "当前状态: " << kw_data["current_state"].asString() << std::endl;
    oss << "最后更新: " << kw_data["last_update"].asString() << std::endl;
    
    if (kw_data.isMember("history")) {
        const Json::Value& history = kw_data["history"];
        oss << "\n历史记录 (" << history.size() << " 条):" << std::endl;
        
        for (size_t i = 0; i < history.size(); ++i) {
            oss << "  [" << (i+1) << "] "
                << "状态: " << history[static_cast<int>(i)]["state"].asString().substr(0, 16) << "... | "
                << "文件ID: " << history[static_cast<int>(i)]["file_id"].asString().substr(0, 16) << "... | "
                << "时间: " << history[static_cast<int>(i)]["timestamp"].asString() << std::endl;
        }
    }
    oss << "============================================" << std::endl;
    
    return oss.str();
}

// ============================================================================
// 辅助函数
// ============================================================================

bool StorageClient::readFile(const std::string& file_path,
                            std::vector<unsigned char>& data) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法打开文件: " << file_path << std::endl;
        return false;
    }
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    data.resize(size);
    if (!file.read(reinterpret_cast<char*>(data.data()), size)) {
        std::cerr << "[错误] 文件读取失败" << std::endl;
        file.close();
        return false;
    }
    
    file.close();
    return true;
}

bool StorageClient::writeFile(const std::string& file_path,
                             const std::vector<unsigned char>& data) {
    std::ofstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[错误] 无法创建文件: " << file_path << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    file.close();
    
    return true;
}

std::vector<std::vector<unsigned char>> StorageClient::splitIntoBlocks(
    const std::vector<unsigned char>& data,
    size_t block_size) {
    std::vector<std::vector<unsigned char>> blocks;
    
    for (size_t i = 0; i < data.size(); i += block_size) {
        size_t current_block_size = std::min(block_size, data.size() - i);
        std::vector<unsigned char> block(data.begin() + i,
                                        data.begin() + i + current_block_size);
        
        // 如果最后一块不足，填充零
        if (block.size() < block_size) {
            block.resize(block_size, 0);
        }
        
        blocks.push_back(block);
    }
    
    return blocks;
}

std::string StorageClient::serializeElement(element_t elem) {
    int len = element_length_in_bytes(elem);
    std::vector<unsigned char> buf(len);
    element_to_bytes(buf.data(), elem);
    return bytesToHex(buf);
}

bool StorageClient::deserializeElement(const std::string& hex_str, element_t elem) {
    if (hex_str.length() % 2 != 0) {
        return false;
    }
    
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        std::string byte_str = hex_str.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    
    int bytes_read = element_from_bytes(elem, bytes.data());
    if (bytes_read <= 0) {
        return false;
    }
    
    return true;
}

std::string StorageClient::bytesToHex(const std::vector<unsigned char>& bytes) {
    std::ostringstream oss;
    for (unsigned char byte : bytes) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(byte);
    }
    return oss.str();
}

std::string StorageClient::getCurrentTimestamp() {
    std::time_t now = std::time(nullptr);
    std::tm* local_time = std::localtime(&now);
    
    std::ostringstream oss;
    oss << (local_time->tm_year + 1900) << "-"
        << std::setw(2) << std::setfill('0') << (local_time->tm_mon + 1) << "-"
        << std::setw(2) << std::setfill('0') << local_time->tm_mday << " "
        << std::setw(2) << std::setfill('0') << local_time->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << local_time->tm_min << ":"
        << std::setw(2) << std::setfill('0') << local_time->tm_sec;
    
    return oss.str();
}
