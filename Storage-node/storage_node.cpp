#include "storage_node.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <chrono>
#include <algorithm>

// ==================== 构造函数和析构函数 ====================

StorageNode::StorageNode(const std::string& data_directory, int port) 
    : data_dir(data_directory), server_port(port), crypto_initialized(false) {
    
    files_dir = data_dir + "/files";
    metadata_dir = data_dir + "/metadata";
    
    // 生成节点ID
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << "node_" << timestamp;
    node_id = ss.str();
}

StorageNode::~StorageNode() {
    if (crypto_initialized) {
        element_clear(g);
        element_clear(mu);
        mpz_clear(N);
        pairing_clear(pairing);
    }
}

// ==================== 密码学函数 ====================

bool StorageNode::setup_cryptography() {
    std::cout << "🔧 初始化密码学参数..." << std::endl;
    
    // 初始化配对参数
    const char* param_str = 
        "type a\n"
        "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
        "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
        "r 730750818665451621361119245571504901405976559617\n"
        "exp2 159\n"
        "exp1 107\n"
        "sign1 1\n"
        "sign0 1\n";
    
    if (pairing_init_set_buf(pairing, param_str, strlen(param_str)) != 0) {
        std::cerr << "❌ 配对参数初始化失败" << std::endl;
        return false;
    }
    
    // 初始化元素
    element_init_G1(g, pairing);
    element_init_G1(mu, pairing);
    mpz_init(N);
    
    // 设置随机生成器
    element_random(g);
    element_random(mu);
    mpz_set_ui(N, 1000000007); // 大质数
    
    crypto_initialized = true;
    std::cout << "✅ 密码学参数初始化成功" << std::endl;
    
    return true;
}

std::string StorageNode::compute_hash_H1(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.length(), hash);
    return bytes_to_hex(hash, SHA256_DIGEST_LENGTH);
}

void StorageNode::compute_hash_H2(element_t result, const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.length(), hash);
    element_from_hash(result, hash, SHA256_DIGEST_LENGTH);
}

std::string StorageNode::compute_hash_H3(const std::string& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)input.c_str(), input.length(), hash);
    return bytes_to_hex(hash, 16); // 返回前16字节
}

void StorageNode::compute_prf(mpz_t result, const std::string& seed, const std::string& input) {
    std::string combined = seed + input;
    std::string hash_hex = compute_hash_H1(combined);
    mpz_set_str(result, hash_hex.c_str(), 16);
    mpz_mod(result, result, N);
}

std::string StorageNode::decrypt_pointer(const std::string& encrypted_pointer, const std::string& key) {
    // 简化的解密实现
    std::string result = encrypted_pointer;
    for (size_t i = 0; i < result.length() && i < key.length(); ++i) {
        result[i] ^= key[i % key.length()];
    }
    return result;
}

bool StorageNode::verify_pk_format(const std::string& pk) {
    // 验证PK格式：应该是hex字符串，长度合理
    if (pk.empty()) {
        return false;
    }
    
    // 检查是否为hex字符串
    for (char c : pk) {
        if (!isxdigit(c)) {
            return false;
        }
    }
    
    // 可以添加更多验证逻辑，如长度检查
    return true;
}

// ==================== JSON文件操作 ====================

Json::Value StorageNode::load_json_from_file(const std::string& filepath) {
    Json::Value root;
    std::ifstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "⚠️  无法打开文件: " << filepath << std::endl;
        return root;
    }
    
    Json::CharReaderBuilder builder;
    std::string errs;
    
    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        std::cerr << "❌ JSON解析失败: " << errs << std::endl;
    }
    
    file.close();
    return root;
}

bool StorageNode::save_json_to_file(const Json::Value& root, const std::string& filepath) {
    std::ofstream file(filepath);
    
    if (!file.is_open()) {
        std::cerr << "❌ 无法写入文件: " << filepath << std::endl;
        return false;
    }
    
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "    ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &file);
    
    file.close();
    return true;
}

// ==================== 文件系统操作 ====================

std::string StorageNode::read_file_content(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ 无法读取文件: " << filepath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    
    return buffer.str();
}

bool StorageNode::write_file_content(const std::string& filepath, const std::string& content) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ 无法写入文件: " << filepath << std::endl;
        return false;
    }
    
    file << content;
    file.close();
    return true;
}

bool StorageNode::file_exists(const std::string& filepath) {
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0);
}

bool StorageNode::create_directory(const std::string& dirpath) {
    #ifdef _WIN32
        return _mkdir(dirpath.c_str()) == 0 || errno == EEXIST;
    #else
        return mkdir(dirpath.c_str(), 0755) == 0 || errno == EEXIST;
    #endif
}

std::string StorageNode::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%dT%H:%M:%S");
    return ss.str() + "Z";
}

// ==================== 辅助函数 ====================

std::string StorageNode::bytes_to_hex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::vector<unsigned char> StorageNode::hex_to_bytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtol(byte_str.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// ==================== 初始化函数 ====================

bool StorageNode::initialize_directories() {
    std::cout << "📁 初始化数据目录..." << std::endl;
    
    bool success = true;
    success &= create_directory(data_dir);
    success &= create_directory(files_dir);
    success &= create_directory(metadata_dir);
    
    if (success) {
        std::cout << "✅ 数据目录创建成功" << std::endl;
    } else {
        std::cerr << "❌ 数据目录创建失败" << std::endl;
    }
    
    return success;
}

bool StorageNode::create_default_config() {
    Json::Value config;
    
    config["version"] = "3.1";
    config["node"]["node_id"] = node_id;
    config["node"]["created_at"] = get_current_timestamp();
    config["node"]["description"] = "去中心化存储节点 (支持PK身份验证)";
    
    config["paths"]["data_dir"] = data_dir;
    config["paths"]["files_dir"] = files_dir;
    config["paths"]["metadata_dir"] = metadata_dir;
    config["paths"]["index_db"] = data_dir + "/index_db.json";
    
    config["server"]["port"] = server_port;
    config["server"]["enable_server"] = false;
    
    config["storage"]["max_file_size_mb"] = 100;
    config["storage"]["max_total_storage_gb"] = 10;
    
    config["logging"]["enable_logging"] = true;
    config["logging"]["log_level"] = "INFO";
    
    config["security"]["enable_pk_verification"] = true;
    
    std::string config_path = data_dir + "/config.json";
    return save_json_to_file(config, config_path);
}

bool StorageNode::load_config() {
    std::string config_path = data_dir + "/config.json";
    
    if (!file_exists(config_path)) {
        std::cout << "⚠️  配置文件不存在,创建默认配置..." << std::endl;
        return create_default_config();
    }
    
    Json::Value config = load_json_from_file(config_path);
    
    if (config.isMember("node") && config["node"].isMember("node_id")) {
        node_id = config["node"]["node_id"].asString();
    }
    
    if (config.isMember("server") && config["server"].isMember("port")) {
        server_port = config["server"]["port"].asInt();
    }
    
    std::cout << "✅ 配置加载成功" << std::endl;
    return true;
}

bool StorageNode::save_config() {
    std::string config_path = data_dir + "/config.json";
    Json::Value config = load_json_from_file(config_path);
    
    config["node"]["node_id"] = node_id;
    config["node"]["last_updated"] = get_current_timestamp();
    config["server"]["port"] = server_port;
    
    return save_json_to_file(config, config_path);
}

// ==================== 索引数据库操作 ====================

bool StorageNode::load_index_database() {
    std::string index_path = data_dir + "/index_db.json";
    
    if (!file_exists(index_path)) {
        std::cout << "⚠️  索引数据库不存在,创建新数据库..." << std::endl;
        return save_index_database();
    }
    
    Json::Value root = load_json_from_file(index_path);
    
    index_database.clear();
    
    const Json::Value& indices = root["indices"];
    for (const auto& ts : indices.getMemberNames()) {
        std::vector<IndexEntry> entries;
        
        for (const auto& entry_obj : indices[ts]) {
            IndexEntry entry;
            entry.PK = entry_obj.get("PK", "").asString();
            entry.Ts = entry_obj["Ts"].asString();
            entry.keyword = entry_obj["keyword"].asString();
            entry.pointer = entry_obj["pointer"].asString();
            entry.file_identifier = entry_obj["file_identifier"].asString();
            entry.state = entry_obj.get("state", "valid").asString();
            entries.push_back(entry);
        }
        
        index_database[ts] = entries;
    }
    
    std::cout << "✅ 索引数据库加载成功 (共 " << get_index_count() << " 条)" << std::endl;
    return true;
}

bool StorageNode::save_index_database() {
    Json::Value root;
    root["version"] = "3.1";
    root["last_updated"] = get_current_timestamp();
    
    Json::Value indices;
    int total = 0;
    
    for (const auto& pair : index_database) {
        const std::string& ts = pair.first;
        const std::vector<IndexEntry>& entries = pair.second;
        
        Json::Value entry_array(Json::arrayValue);
        for (const auto& entry : entries) {
            Json::Value entry_obj;
            entry_obj["PK"] = entry.PK;
            entry_obj["Ts"] = entry.Ts;
            entry_obj["keyword"] = entry.keyword;
            entry_obj["pointer"] = entry.pointer;
            entry_obj["file_identifier"] = entry.file_identifier;
            entry_obj["state"] = entry.state;
            entry_array.append(entry_obj);
            total++;
        }
        indices[ts] = entry_array;
    }
    
    root["indices"] = indices;
    root["total_entries"] = total;
    
    std::string index_path = data_dir + "/index_db.json";
    return save_json_to_file(root, index_path);
}

// ==================== 节点信息操作 ====================

bool StorageNode::load_node_info() {
    std::string info_path = data_dir + "/node_info.json";
    
    if (!file_exists(info_path)) {
        return save_node_info();
    }
    
    return true;
}

bool StorageNode::save_node_info() {
    Json::Value info;
    
    info["node_id"] = node_id;
    info["status"] = "active";
    info["last_updated"] = get_current_timestamp();
    
    info["statistics"]["total_files"] = static_cast<int>(file_storage.size());
    info["statistics"]["total_index_entries"] = static_cast<int>(get_index_count());
    
    std::string info_path = data_dir + "/node_info.json";
    return save_json_to_file(info, info_path);
}

void StorageNode::update_statistics(const std::string& operation) {
    save_node_info();
}

// ==================== 文件操作 (v3.1修改) ====================

bool StorageNode::insert_file(const std::string& param_json_path, const std::string& enc_file_path) {
    std::cout << "\n📤 插入文件..." << std::endl;
    
    // 1. 验证文件存在
    if (!file_exists(param_json_path)) {
        std::cerr << "❌ 参数文件不存在: " << param_json_path << std::endl;
        return false;
    }
    
    if (!file_exists(enc_file_path)) {
        std::cerr << "❌ 加密文件不存在: " << enc_file_path << std::endl;
        return false;
    }
    
    // 2. 读取JSON参数 (新格式)
    Json::Value params = load_json_from_file(param_json_path);
    
    // 3. 验证必要字段
    if (!params.isMember("PK") || !params.isMember("ID_F") || 
        !params.isMember("ptr") || !params.isMember("TS_F") ||
        !params.isMember("keywords")) {
        std::cerr << "❌ JSON参数格式错误,缺少必要字段" << std::endl;
        std::cerr << "   必需字段: PK, ID_F, ptr, TS_F, keywords" << std::endl;
        return false;
    }
    
    // 4. 提取数据
    std::string PK = params["PK"].asString();
    std::string file_id = params["ID_F"].asString();
    std::string pointer = params["ptr"].asString();
    std::string file_auth_tag = params["TS_F"].asString();
    std::string state = params.get("state", "valid").asString();
    const Json::Value& keywords = params["keywords"];
    
    std::cout << "   客户端PK: " << PK.substr(0, 16) << "..." << std::endl;
    std::cout << "   文件ID: " << file_id << std::endl;
    std::cout << "   状态: " << state << std::endl;
    std::cout << "   关键词数: " << keywords.size() << std::endl;
    
    // 5. 验证PK格式
    if (!verify_pk_format(PK)) {
        std::cerr << "❌ PK格式无效" << std::endl;
        return false;
    }
    
    // 6. 验证状态值
    if (state != "valid" && state != "invalid") {
        std::cerr << "❌ 状态值无效,必须为 'valid' 或 'invalid'" << std::endl;
        return false;
    }
    
    // 7. 检查文件是否已存在
    if (has_file(file_id)) {
        std::cerr << "⚠️  文件已存在: " << file_id << std::endl;
        char choice;
        std::cout << "是否覆盖? (y/n): ";
        std::cin >> choice;
        if (choice != 'y' && choice != 'Y') {
            std::cout << "❌ 操作已取消" << std::endl;
            return false;
        }
    }
    
    // 8. 创建索引条目 (新格式: T_i, kt_i)
    for (unsigned int i = 0; i < keywords.size(); ++i) {
        if (!keywords[i].isMember("T_i") || !keywords[i].isMember("kt_i")) {
            std::cerr << "❌ 关键词 " << i << " 格式错误,需要 T_i 和 kt_i" << std::endl;
            return false;
        }
        
        std::string T_i = keywords[i]["T_i"].asString();
        std::string kt_i = keywords[i]["kt_i"].asString();
        
        IndexEntry entry;
        entry.PK = PK;
        entry.Ts = T_i;
        entry.keyword = kt_i;
        entry.pointer = pointer;
        entry.file_identifier = file_id;
        entry.state = state;
        
        index_database[T_i].push_back(entry);
        
        std::cout << "   [" << (i+1) << "] T_i: " << T_i.substr(0, 16) << "... → " << kt_i << std::endl;
    }
    
    // 9. 读取加密文件内容
    std::string ciphertext = read_file_content(enc_file_path);
    if (ciphertext.empty()) {
        std::cerr << "❌ 读取加密文件失败" << std::endl;
        return false;
    }
    
    std::cout << "   密文大小: " << ciphertext.length() << " 字节" << std::endl;
    
    // 10. 存储文件数据
    FileData file_data;
    file_data.PK = PK;
    file_data.file_id = file_id;
    file_data.ciphertext = ciphertext;
    file_data.pointer = pointer;
    file_data.file_auth_tag = file_auth_tag;
    file_data.state = state;
    
    file_storage[file_id] = file_data;
    
    // 11. 保存加密文件到文件系统
    if (!save_encrypted_file(file_id, enc_file_path)) {
        std::cerr << "❌ 保存加密文件失败" << std::endl;
        return false;
    }
    
    // 12. 保存索引数据库
    if (!save_index_database()) {
        std::cerr << "❌ 保存索引数据库失败" << std::endl;
        return false;
    }
    
    // 13. 保存元数据
    Json::Value metadata;
    metadata["PK"] = PK;
    metadata["file_id"] = file_id;
    metadata["pointer"] = pointer;
    metadata["file_auth_tag"] = file_auth_tag;
    metadata["state"] = state;
    metadata["insert_time"] = get_current_timestamp();
    metadata["keyword_count"] = static_cast<int>(keywords.size());
    metadata["file_size"] = static_cast<int>(ciphertext.length());
    
    if (params.isMember("metadata")) {
        metadata["original"] = params["metadata"];
    }
    
    std::string metadata_path = metadata_dir + "/" + file_id + ".json";
    save_json_to_file(metadata, metadata_path);
    
    // 14. 更新统计
    update_statistics("insert");
    
    std::cout << "✅ 文件插入成功!" << std::endl;
    std::cout << "   索引条目: " << keywords.size() << std::endl;
    std::cout << "   总文件数: " << file_storage.size() << std::endl;
    std::cout << "   总索引数: " << get_index_count() << std::endl;
    
    return true;
}

bool StorageNode::delete_file(const std::string& PK, const std::string& file_id, const std::string& del_proof) {
    std::cout << "\n🗑️  删除文件: " << file_id << std::endl;
    std::cout << "   请求者PK: " << PK.substr(0, 16) << "..." << std::endl;
    
    // 验证PK格式
    if (!verify_pk_format(PK)) {
        std::cerr << "❌ PK格式无效" << std::endl;
        return false;
    }
    
    if (!has_file(file_id)) {
        std::cerr << "❌ 文件不存在" << std::endl;
        return false;
    }
    
    // 验证文件所有权
    const FileData& file_data = file_storage[file_id];
    if (file_data.PK != PK) {
        std::cerr << "❌ 权限不足: 您不是此文件的所有者" << std::endl;
        std::cerr << "   文件所有者PK: " << file_data.PK.substr(0, 16) << "..." << std::endl;
        return false;
    }
    
    std::cout << "   ✅ 身份验证通过" << std::endl;
    
    // 标记索引为无效 (state = "invalid")
    int marked_count = 0;
    for (auto& pair : index_database) {
        for (auto& entry : pair.second) {
            if (entry.file_identifier == file_id && entry.PK == PK) {
                entry.state = "invalid";
                marked_count++;
            }
        }
    }
    
    std::cout << "   标记 " << marked_count << " 条索引为无效" << std::endl;
    
    // 更新文件状态
    file_storage[file_id].state = "invalid";
    
    // 可选：物理删除文件 (注释掉以保留文件)
    /*
    file_storage.erase(file_id);
    
    std::string enc_file_path = files_dir + "/" + file_id + ".enc";
    if (file_exists(enc_file_path)) {
        remove(enc_file_path.c_str());
    }
    
    std::string metadata_path = metadata_dir + "/" + file_id + ".json";
    if (file_exists(metadata_path)) {
        remove(metadata_path.c_str());
    }
    */
    
    // 保存更新 (修复拼写错误)
    save_index_database();
    update_statistics("delete");
    
    std::cout << "✅ 文件删除成功 (已标记为无效)" << std::endl;
    return true;
}

SearchResult StorageNode::search_keyword(const std::string& PK,
                                        const std::string& search_token, 
                                        const std::string& latest_state,
                                        const std::string& seed) {
    SearchResult result;
    
    std::cout << "\n🔍 搜索关键词..." << std::endl;
    std::cout << "   请求者PK: " << PK.substr(0, 16) << "..." << std::endl;
    std::cout << "   搜索令牌: " << search_token.substr(0, 16) << "..." << std::endl;
    
    // 验证PK格式
    if (!verify_pk_format(PK)) {
        std::cerr << "❌ PK格式无效" << std::endl;
        return result;
    }
    
    // 在索引数据库中查找
    auto it = index_database.find(search_token);
    if (it != index_database.end()) {
        for (const auto& entry : it->second) {
            // 只返回该PK的文件且状态为valid的条目
            if (entry.PK == PK && entry.state == "valid") {
                result.file_identifiers.push_back(entry.file_identifier);
                result.keyword_proofs.push_back(entry.keyword);
            }
        }
    }
    
    std::cout << "   找到 " << result.file_identifiers.size() << " 个匹配文件" << std::endl;
    
    return result;
}

std::string StorageNode::generate_integrity_proof(const std::string& file_id, 
                                                  const std::string& seed) {
    if (!has_file(file_id)) {
        return "";
    }
    
    const FileData& data = file_storage[file_id];
    
    // 生成完整性证明
    std::string combined = file_id + data.file_auth_tag + seed;
    return compute_hash_H1(combined);
}

// ==================== 检索函数 ====================

Json::Value StorageNode::retrieve_file(const std::string& file_id) {
    Json::Value result;
    
    if (!has_file(file_id)) {
        result["success"] = false;
        result["error"] = "文件不存在";
        return result;
    }
    
    const FileData& data = file_storage[file_id];
    
    result["success"] = true;
    result["PK"] = data.PK;
    result["file_id"] = file_id;
    result["ciphertext"] = data.ciphertext;
    result["pointer"] = data.pointer;
    result["file_auth_tag"] = data.file_auth_tag;
    result["state"] = data.state;
    
    return result;
}

Json::Value StorageNode::retrieve_files_batch(const std::vector<std::string>& file_ids) {
    Json::Value result;
    result["files"] = Json::arrayValue;
    
    for (const auto& file_id : file_ids) {
        result["files"].append(retrieve_file(file_id));
    }
    
    return result;
}

Json::Value StorageNode::get_file_metadata(const std::string& file_id) {
    std::string metadata_path = metadata_dir + "/" + file_id + ".json";
    
    if (!file_exists(metadata_path)) {
        Json::Value error;
        error["success"] = false;
        error["error"] = "元数据不存在";
        return error;
    }
    
    return load_json_from_file(metadata_path);
}

bool StorageNode::export_file_metadata(const std::string& file_id, const std::string& output_path) {
    Json::Value metadata = get_file_metadata(file_id);
    
    if (!metadata.isMember("success") || !metadata["success"].asBool()) {
        return false;
    }
    
    return save_json_to_file(metadata, output_path);
}

// ==================== 文件存储 ====================

bool StorageNode::save_encrypted_file(const std::string& file_id, const std::string& enc_file_path) {
    std::string content = read_file_content(enc_file_path);
    if (content.empty()) {
        return false;
    }
    
    std::string dest_path = files_dir + "/" + file_id + ".enc";
    return write_file_content(dest_path, content);
}

bool StorageNode::load_encrypted_file(const std::string& file_id, std::string& ciphertext) {
    std::string file_path = files_dir + "/" + file_id + ".enc";
    
    if (!file_exists(file_path)) {
        return false;
    }
    
    ciphertext = read_file_content(file_path);
    return !ciphertext.empty();
}

std::vector<std::string> StorageNode::list_all_files() {
    std::vector<std::string> file_list;
    
    for (const auto& pair : file_storage) {
        file_list.push_back(pair.first);
    }
    
    return file_list;
}

std::vector<std::string> StorageNode::list_files_by_pk(const std::string& PK) {
    std::vector<std::string> file_list;
    
    for (const auto& pair : file_storage) {
        if (pair.second.PK == PK) {
            file_list.push_back(pair.first);
        }
    }
    
    return file_list;
}

// ==================== 详细状态 ====================

void StorageNode::print_detailed_status() {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📊 存储节点详细状态" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    std::cout << "\n🔧 基本信息:" << std::endl;
    std::cout << "   节点 ID:      " << node_id << std::endl;
    std::cout << "   数据目录:     " << data_dir << std::endl;
    std::cout << "   端口:         " << server_port << std::endl;
    std::cout << "   版本:         v3.1 (支持PK身份验证)" << std::endl;
    
    std::cout << "\n📦 存储统计:" << std::endl;
    std::cout << "   文件总数:     " << file_storage.size() << std::endl;
    std::cout << "   索引总数:     " << get_index_count() << std::endl;
    
    // 统计各状态文件数
    int valid_count = 0;
    int invalid_count = 0;
    for (const auto& pair : file_storage) {
        if (pair.second.state == "valid") {
            valid_count++;
        } else {
            invalid_count++;
        }
    }
    std::cout << "   有效文件:     " << valid_count << std::endl;
    std::cout << "   无效文件:     " << invalid_count << std::endl;
    
    std::cout << "\n🔐 密码学状态:" << std::endl;
    std::cout << "   初始化:       " << (crypto_initialized ? "✅ 是" : "❌ 否") << std::endl;
    
    if (!file_storage.empty()) {
        std::cout << "\n📄 文件列表:" << std::endl;
        int count = 0;
        for (const auto& pair : file_storage) {
            count++;
            std::cout << "   [" << count << "] " << pair.first 
                     << " (" << pair.second.ciphertext.length() << " 字节, "
                     << "PK: " << pair.second.PK.substr(0, 8) << "..., "
                     << "状态: " << pair.second.state << ")" << std::endl;
            if (count >= 10) {
                std::cout << "   ... (还有 " << (file_storage.size() - 10) << " 个文件)" << std::endl;
                break;
            }
        }
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
}