#include "storage_node.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <chrono>
#include <algorithm>

// ==================== 构造函数和析构函数 ====================

StorageNode::StorageNode(const std::string& data_directory, int port) 
    : data_dir(data_directory), server_port(port), crypto_initialized(false) {
    
    files_dir = data_dir + "/EncFiles";
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

bool StorageNode::setup_cryptography(int security_param, 
                                    const std::string& public_params_path) {
    std::cout << "🔧 初始化密码学参数 (Setup算法)..." << std::endl;
    std::cout << "   安全参数 K: " << security_param << " bits" << std::endl;
    
    // 初始化配对参数（type a 配对）
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
    
    // 从配对参数中提取 p 和 q，计算 N = p × q
    // 对于 type a 配对，p = q（群的阶）
    mpz_t p, q;
    mpz_init(p);
    mpz_init(q);
    
    // 从配对参数中获取群的阶
    // 对于 type a 配对，使用配对中定义的q值
    mpz_set_str(p, "8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791", 10);
    mpz_set_str(q, "8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791", 10);
    
    // 计算 N = p × q
    mpz_mul(N, p, q);
    
    // 输出 N 的信息（截断显示）
    char* n_str = mpz_get_str(NULL, 10, N);
    std::string n_full(n_str);
    free(n_str);
    std::cout << "   N = p × q (前50位): " << n_full.substr(0, 50) << "..." << std::endl;
    std::cout << "   N 总位数: " << n_full.length() << " 位十进制数" << std::endl;
    
    mpz_clear(p);
    mpz_clear(q);
    
    crypto_initialized = true;
    std::cout << "✅ 密码学参数初始化成功" << std::endl;
    
    // 如果提供了公共参数路径，保存公共参数
    if (!public_params_path.empty()) {
        if (!save_public_params(public_params_path)) {
            std::cerr << "⚠️  公共参数保存失败，但密码学系统已初始化" << std::endl;
        } else {
            std::cout << "✅ 公共参数已保存到: " << public_params_path << std::endl;
        }
    }
    
    return true;
}

bool StorageNode::save_public_params(const std::string& filepath) {
    if (!crypto_initialized) {
        std::cerr << "❌ 密码学系统未初始化" << std::endl;
        return false;
    }
    
    Json::Value root;
    
    // 基本信息
    root["version"] = "2.0";  // 升级版本号，表示使用新的序列化格式
    root["created_at"] = get_current_timestamp();
    root["description"] = "Public Parameters (N, g, μ) for Decentralized Storage System";
    root["serialization_method"] = "element_to_bytes";  // 标注序列化方法
    
    // 公共参数 PP = {N, g, μ}
    Json::Value public_params;
    
    // N: 计算得到的大整数 N = p × q
    char* n_str = mpz_get_str(NULL, 10, N);
    public_params["N"] = std::string(n_str);
    free(n_str);
    
    // g: G_1的生成元（使用element_to_bytes序列化）
    int g_len = element_length_in_bytes(g);
    unsigned char* g_bytes = new unsigned char[g_len];
    element_to_bytes(g_bytes, g);
    public_params["g"] = bytes_to_hex(g_bytes, g_len);  // 转为hex字符串存储
    public_params["g_length"] = g_len;  // 保存字节长度，用于验证
    delete[] g_bytes;
    
    // μ: G_1的生成元（使用element_to_bytes序列化）
    // 注意：在type a配对中，G_1和G_2是同一个群，但μ是独立的生成元
    int mu_len = element_length_in_bytes(mu);
    unsigned char* mu_bytes = new unsigned char[mu_len];
    element_to_bytes(mu_bytes, mu);
    public_params["mu"] = bytes_to_hex(mu_bytes, mu_len);  // 转为hex字符串存储
    public_params["mu_length"] = mu_len;  // 保存字节长度，用于验证
    delete[] mu_bytes;
    
    root["public_params"] = public_params;
    
    // 保存到文件
    bool success = save_json_to_file(root, filepath);
    
    if (success) {
        std::cout << "   ✅ 公共参数已保存 (N, g, μ)" << std::endl;
        std::cout << "   📊 序列化信息:" << std::endl;
        std::cout << "      - g 字节长度: " << g_len << std::endl;
        std::cout << "      - μ 字节长度: " << mu_len << std::endl;
    }
    
    return success;
}

bool StorageNode::load_public_params(const std::string& filepath) {
    std::cout << "🔄 从文件加载公共参数并初始化密码学系统..." << std::endl;
    
    if (!file_exists(filepath)) {
        std::cerr << "❌ 公共参数文件不存在: " << filepath << std::endl;
        return false;
    }
    
    // 加载JSON文件
    Json::Value root = load_json_from_file(filepath);
    
    if (!root.isMember("public_params")) {
        std::cerr << "❌ 公共参数格式错误" << std::endl;
        return false;
    }
    
    Json::Value pp = root["public_params"];
    
    // 检查必需字段
    if (!pp.isMember("N") || !pp.isMember("g") || !pp.isMember("mu")) {
        std::cerr << "❌ 公共参数缺少必需字段 (N, g, μ)" << std::endl;
        return false;
    }
    
    // ============ 步骤1: 显示公共参数信息 ============
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📖 公共参数 (Public Parameters)" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "版本:         " << root["version"].asString() << std::endl;
    std::cout << "创建时间:     " << root["created_at"].asString() << std::endl;
    std::cout << "描述:         " << root["description"].asString() << std::endl;
    
    // 检查序列化方法（兼容旧版本）
    std::string serialization_method = "element_to_mpz";  // 默认旧格式
    if (root.isMember("serialization_method")) {
        serialization_method = root["serialization_method"].asString();
    }
    std::cout << "序列化方法:   " << serialization_method << std::endl;
    
    std::cout << "\n[公共参数 PP = {N, g, μ}]" << std::endl;
    
    // N: 大整数
    std::string n_str = pp["N"].asString();
    std::cout << "N (前50位):   " << n_str.substr(0, 50) << "..." << std::endl;
    std::cout << "N (总位数):   " << n_str.length() << " 位十进制数" << std::endl;
    
    // g: G_1的生成元
    std::string g_str = pp["g"].asString();
    if (serialization_method == "element_to_bytes") {
        int g_len = pp.isMember("g_length") ? pp["g_length"].asInt() : (g_str.length() / 2);
        std::cout << "g (字节长度): " << g_len << " bytes" << std::endl;
        std::cout << "g (hex前40位):" << g_str.substr(0, 40) << "..." << std::endl;
    } else {
        std::cout << "g (前40位):   " << g_str.substr(0, 40) << "..." << std::endl;
        std::cout << "g (总长度):   " << g_str.length() << " 位十进制数" << std::endl;
    }
    
    // μ: G_1的生成元
    std::string mu_str = pp["mu"].asString();
    if (serialization_method == "element_to_bytes") {
        int mu_len = pp.isMember("mu_length") ? pp["mu_length"].asInt() : (mu_str.length() / 2);
        std::cout << "μ (字节长度): " << mu_len << " bytes" << std::endl;
        std::cout << "μ (hex前40位):" << mu_str.substr(0, 40) << "..." << std::endl;
    } else {
        std::cout << "μ (前40位):   " << mu_str.substr(0, 40) << "..." << std::endl;
        std::cout << "μ (总长度):   " << mu_str.length() << " 位十进制数" << std::endl;
    }
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    // ============ 步骤2: 初始化密码学系统 ============
    std::cout << "🔧 初始化密码学系统..." << std::endl;
    
    // 初始化配对参数（使用相同的type a配对）
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
    
    // ============ 步骤3: 加载参数到内存 ============
    
    // 加载 N
    if (mpz_set_str(N, n_str.c_str(), 10) != 0) {
        std::cerr << "❌ N 参数格式错误" << std::endl;
        element_clear(g);
        element_clear(mu);
        mpz_clear(N);
        pairing_clear(pairing);
        return false;
    }
    std::cout << "   ✅ 加载 N (" << n_str.length() << " 位十进制数)" << std::endl;
    
    // 加载 g - 根据序列化方法选择不同的加载方式
    if (serialization_method == "element_to_bytes") {
        // 新格式：使用 element_from_bytes
        std::vector<unsigned char> g_bytes = hex_to_bytes(g_str);
        if (g_bytes.empty()) {
            std::cerr << "❌ g 参数hex解码失败" << std::endl;
            element_clear(g);
            element_clear(mu);
            mpz_clear(N);
            pairing_clear(pairing);
            return false;
        }
        
        int bytes_read = element_from_bytes(g, g_bytes.data());
        if (bytes_read <= 0) {
            std::cerr << "❌ g 参数反序列化失败 (element_from_bytes返回: " << bytes_read << ")" << std::endl;
            element_clear(g);
            element_clear(mu);
            mpz_clear(N);
            pairing_clear(pairing);
            return false;
        }
        std::cout << "   ✅ 加载 g (bytes长度: " << g_bytes.size() << ")" << std::endl;
    } else {
        // 旧格式：使用 element_set_mpz（兼容性支持）
        mpz_t g_mpz;
        mpz_init(g_mpz);
        if (mpz_set_str(g_mpz, g_str.c_str(), 10) != 0) {
            std::cerr << "❌ g 参数格式错误" << std::endl;
            mpz_clear(g_mpz);
            element_clear(g);
            element_clear(mu);
            mpz_clear(N);
            pairing_clear(pairing);
            return false;
        }
        element_set_mpz(g, g_mpz);
        mpz_clear(g_mpz);
        std::cout << "   ✅ 加载 g (" << g_str.length() << " 位十进制数，使用兼容模式)" << std::endl;
        std::cout << "   ⚠️  建议重新生成并保存公共参数以使用新格式" << std::endl;
    }
    
    // 加载 μ - 根据序列化方法选择不同的加载方式
    if (serialization_method == "element_to_bytes") {
        // 新格式：使用 element_from_bytes
        std::vector<unsigned char> mu_bytes = hex_to_bytes(mu_str);
        if (mu_bytes.empty()) {
            std::cerr << "❌ μ 参数hex解码失败" << std::endl;
            element_clear(g);
            element_clear(mu);
            mpz_clear(N);
            pairing_clear(pairing);
            return false;
        }
        
        int bytes_read = element_from_bytes(mu, mu_bytes.data());
        if (bytes_read <= 0) {
            std::cerr << "❌ μ 参数反序列化失败 (element_from_bytes返回: " << bytes_read << ")" << std::endl;
            element_clear(g);
            element_clear(mu);
            mpz_clear(N);
            pairing_clear(pairing);
            return false;
        }
        std::cout << "   ✅ 加载 μ (bytes长度: " << mu_bytes.size() << ")" << std::endl;
    } else {
        // 旧格式：使用 element_set_mpz（兼容性支持）
        mpz_t mu_mpz;
        mpz_init(mu_mpz);
        if (mpz_set_str(mu_mpz, mu_str.c_str(), 10) != 0) {
            std::cerr << "❌ μ 参数格式错误" << std::endl;
            mpz_clear(mu_mpz);
            element_clear(g);
            element_clear(mu);
            mpz_clear(N);
            pairing_clear(pairing);
            return false;
        }
        element_set_mpz(mu, mu_mpz);
        mpz_clear(mu_mpz);
        std::cout << "   ✅ 加载 μ (" << mu_str.length() << " 位十进制数，使用兼容模式)" << std::endl;
        std::cout << "   ⚠️  建议重新生成并保存公共参数以使用新格式" << std::endl;
    }
    
    crypto_initialized = true;
    std::cout << "✅ 密码学系统已从公共参数恢复\n" << std::endl;
    
    return true;
}

bool StorageNode::display_public_params(const std::string& filepath) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔑 查看公共参数" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    // 情况1: 如果提供了文件路径，从文件读取并显示
    if (!filepath.empty()) {
        if (!file_exists(filepath)) {
            std::cerr << "❌ 公共参数文件不存在: " << filepath << std::endl;
            return false;
        }
        
        std::cout << "📄 从文件读取: " << filepath << std::endl;
        
        // 加载JSON文件
        Json::Value root = load_json_from_file(filepath);
        
        if (!root.isMember("public_params")) {
            std::cerr << "❌ 公共参数格式错误" << std::endl;
            return false;
        }
        
        Json::Value pp = root["public_params"];
        
        // 检查必需字段
        if (!pp.isMember("N") || !pp.isMember("g") || !pp.isMember("mu")) {
            std::cerr << "❌ 公共参数缺少必需字段 (N, g, μ)" << std::endl;
            return false;
        }
        
        // 显示基本信息
        std::cout << "\n📋 文件信息:" << std::endl;
        std::cout << "   版本:         " << root["version"].asString() << std::endl;
        std::cout << "   创建时间:     " << root["created_at"].asString() << std::endl;
        std::cout << "   描述:         " << root["description"].asString() << std::endl;
        
        // 检查序列化方法
        std::string serialization_method = "element_to_mpz";  // 默认旧格式
        if (root.isMember("serialization_method")) {
            serialization_method = root["serialization_method"].asString();
        }
        std::cout << "   序列化方法:   " << serialization_method << std::endl;
        
        std::cout << "\n[公共参数 PP = {N, g, μ}]" << std::endl;
        
        // N: 大整数
        std::string n_str = pp["N"].asString();
        std::cout << "   N (前50位):   " << n_str.substr(0, 50) << "..." << std::endl;
        std::cout << "   N (总位数):   " << n_str.length() << " 位十进制数" << std::endl;
        
        // g: G_1的生成元
        std::string g_str = pp["g"].asString();
        if (serialization_method == "element_to_bytes") {
            int g_len = pp.isMember("g_length") ? pp["g_length"].asInt() : (g_str.length() / 2);
            std::cout << "   g (字节长度): " << g_len << " bytes" << std::endl;
            std::cout << "   g (hex前40位):" << g_str.substr(0, 40) << "..." << std::endl;
        } else {
            std::cout << "   g (前40位):   " << g_str.substr(0, 40) << "..." << std::endl;
            std::cout << "   g (总长度):   " << g_str.length() << " 位十进制数" << std::endl;
        }
        
        // μ: G_1的生成元
        std::string mu_str = pp["mu"].asString();
        if (serialization_method == "element_to_bytes") {
            int mu_len = pp.isMember("mu_length") ? pp["mu_length"].asInt() : (mu_str.length() / 2);
            std::cout << "   μ (字节长度): " << mu_len << " bytes" << std::endl;
            std::cout << "   μ (hex前40位):" << mu_str.substr(0, 40) << "..." << std::endl;
        } else {
            std::cout << "   μ (前40位):   " << mu_str.substr(0, 40) << "..." << std::endl;
            std::cout << "   μ (总长度):   " << mu_str.length() << " 位十进制数" << std::endl;
        }
        
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "💡 提示: 这是只读查看，不会修改系统状态" << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
        
        return true;
    }
    
    // 情况2: 如果未提供文件路径，显示内存中的参数
    if (!crypto_initialized) {
        std::cerr << "❌ 密码学系统未初始化，无法显示内存中的参数" << std::endl;
        std::cerr << "💡 提示: 请提供文件路径，或先加载公共参数" << std::endl;
        return false;
    }
    
    std::cout << "📦 显示内存中的公共参数:" << std::endl;
    std::cout << "\n[公共参数 PP = {N, g, μ}]" << std::endl;
    
    // N: 大整数
    char* n_str = mpz_get_str(NULL, 10, N);
    std::string n_full(n_str);
    free(n_str);
    std::cout << "   N (前50位):   " << n_full.substr(0, 50) << "..." << std::endl;
    std::cout << "   N (总位数):   " << n_full.length() << " 位十进制数" << std::endl;
    
    // g: G_1的生成元
    int g_len = element_length_in_bytes(g);
    std::cout << "   g (字节长度): " << g_len << " bytes" << std::endl;
    
    // μ: G_1的生成元
    int mu_len = element_length_in_bytes(mu);
    std::cout << "   μ (字节长度): " << mu_len << " bytes" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "✅ 密码学系统状态: 已初始化" << std::endl;
    std::cout << "💡 提示: 这是内存中的当前参数" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    return true;
}

std::string StorageNode::computeHashH1(const std::string& input, mpz_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    mpz_import(result, SHA256_DIGEST_LENGTH, 1, 1, 0, 0, hash);
    mpz_mod(result, result, N_);
}

void StorageNode::computeHashH2(const std::string& input, element_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    element_from_hash(result, hash, SHA256_DIGEST_LENGTH);
}

std::string StorageNode::computeHashH3(const std::string& input) {
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
// 生成伪随机数，暂时没用
void StorageNode::compute_prf(mpz_t result, const std::string& seed, const std::string& input) {
    std::string combined = seed + input;
    std::string hash_hex = computeHashH3(combined);
    mpz_set_str(result, hash_hex.c_str(), 16);
    mpz_mod(result, result, N);
}

std::string StorageNode::decrypt_pointer(const std::string& current_state_hash, const std::string& encrypted_pointer) {
    // 和加密保持一致：全0表示没有前一个状态
    if (encrypted_pointer.empty() || encrypted_pointer == std::string(64, '0')) {
        return "";
    }

    // 1. 将十六进制密文转换回字节
    std::vector<unsigned char> ciphertext = hexToBytes(encrypted_pointer);
    if (ciphertext.empty()) {
        return "";
    }

    // 2. 创建解密上下文
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return "";
    }

    // 3. 从 current_state_hash 中提取 32 字节 AES 密钥（与加密完全一致）
    unsigned char key[32] = {0};
    for (size_t i = 0; i < 32 && i * 2 < current_state_hash.length(); ++i) {
        std::string byte_str = current_state_hash.substr(i * 2, 2);
        key[i] = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
    }

    // 4. 固定全 0 IV（与加密一致）
    unsigned char iv[16] = {0};

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    // 5. 分配明文缓存：密文长度足够
    std::vector<unsigned char> plaintext(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int len = 0;
    int total_len = 0;

    // 6. DecryptUpdate
    if (EVP_DecryptUpdate(ctx,
                          plaintext.data(), &len,
                          ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len = len;

    // 7. DecryptFinal（处理 padding）
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        // padding 错误或数据被篡改
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len += len;

    // 8. 调整明文长度并转成 std::string
    plaintext.resize(total_len);
    EVP_CIPHER_CTX_free(ctx);

    return std::string(plaintext.begin(), plaintext.end());
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

bool StorageNode::file_exists(const std::string& filepath) const {
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
    
    config["version"] = "3.4";
    config["node"]["node_id"] = node_id;
    config["node"]["created_at"] = get_current_timestamp();
    config["node"]["description"] = "去中心化存储节点 (支持公共参数持久化)";
    
    config["paths"]["data_dir"] = data_dir;
    config["paths"]["files_dir"] = files_dir;
    config["paths"]["metadata_dir"] = metadata_dir;
    config["paths"]["index_db"] = data_dir + "/index_db.json";
    config["paths"]["public_params"] = data_dir + "/public_params.json";
    
    config["server"]["port"] = server_port;
    config["server"]["enable_server"] = false;
    
    config["storage"]["max_file_size_mb"] = 100;
    config["storage"]["enable_compression"] = false;
    
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
    
    std::cout << "✅ 配置加载成功" << std::endl;
    return true;
}

bool StorageNode::save_config() {
    Json::Value config;
    
    config["version"] = "3.4";
    config["node"]["node_id"] = node_id;
    config["node"]["last_update"] = get_current_timestamp();
    
    config["paths"]["data_dir"] = data_dir;
    config["paths"]["files_dir"] = files_dir;
    config["paths"]["metadata_dir"] = metadata_dir;
    
    config["server"]["port"] = server_port;
    
    std::string config_path = data_dir + "/config.json";
    return save_json_to_file(config, config_path);
}

// ==================== 索引数据库操作 (重构) ====================

bool StorageNode::load_index_database() {
    std::string index_path = data_dir + "/index_db.json";
    
    if (!file_exists(index_path)) {
        std::cout << "⚠️  索引数据库不存在,将创建新数据库" << std::endl;
        return save_index_database();
    }
    
    Json::Value root = load_json_from_file(index_path);
    
    // 新格式：支持 file_count, ID_Fs, database 字段
    if (root.isMember("database") && root["database"].isArray()) {
        // 新格式
        index_database.clear();
        
        for (const auto& entry_json : root["database"]) {
            IndexEntry entry;
            entry.ID_F = entry_json["ID_F"].asString();
            entry.PK = entry_json["PK"].asString();
            entry.state = entry_json["state"].asString();
            entry.file_path = entry_json.get("file_path", "").asString();
            
            // 加载 TS_F
            if (entry_json.isMember("TS_F") && entry_json["TS_F"].isArray()) {
                for (const auto& ts : entry_json["TS_F"]) {
                    entry.TS_F.push_back(ts.asString());
                }
            }
            
            // 加载 keywords
            if (entry_json.isMember("keywords") && entry_json["keywords"].isArray()) {
                for (const auto& kw_json : entry_json["keywords"]) {
                    IndexKeywords kw;
                    kw.ptr_i = kw_json.get("ptr_i", "").asString();
                    kw.kt_wi = kw_json.get("kt_wi", "").asString();
                    kw.Ti_bar = kw_json.get("Ti_bar", "").asString();
                    entry.keywords.push_back(kw);
                }
            }
            
            // 以 ID_F 为键存储
            index_database[entry.ID_F] = entry;
        }
        
        std::cout << "✅ 索引数据库加载成功 (新格式，共 " << index_database.size() << " 个文件)" << std::endl;
        
    } else if (root.isMember("indices")) {
        // 旧格式兼容：indices 是一个对象，键是 Ti_bar
        std::cout << "⚠️  检测到旧格式数据库，正在转换..." << std::endl;
        index_database.clear();
        
        for (const auto& token : root["indices"].getMemberNames()) {
            for (const auto& entry_json : root["indices"][token]) {
                IndexEntry entry;
                entry.ID_F = entry_json["ID_F"].asString();
                entry.PK = entry_json["PK"].asString();
                entry.state = entry_json["state"].asString();
                entry.file_path = entry_json.get("file_path", "").asString();
                
                // 加载 TS_F
                if (entry_json.isMember("TS_F") && entry_json["TS_F"].isArray()) {
                    for (const auto& ts : entry_json["TS_F"]) {
                        entry.TS_F.push_back(ts.asString());
                    }
                }
                
                // 加载 keywords
                if (entry_json.isMember("keywords") && entry_json["keywords"].isArray()) {
                    for (const auto& kw_json : entry_json["keywords"]) {
                        IndexKeywords kw;
                        kw.ptr_i = kw_json.get("ptr_i", "").asString();
                        kw.kt_wi = kw_json.get("kt_wi", "").asString();
                        kw.Ti_bar = kw_json.get("Ti_bar", "").asString();
                        entry.keywords.push_back(kw);
                    }
                }
                
                // 以 ID_F 为键存储（去重）
                if (index_database.find(entry.ID_F) == index_database.end()) {
                    index_database[entry.ID_F] = entry;
                }
            }
        }
        
        std::cout << "✅ 索引数据库加载成功 (旧格式已转换，共 " << index_database.size() << " 个文件)" << std::endl;
        std::cout << "💡 建议：下次保存时将自动更新为新格式" << std::endl;
        
    } else {
        std::cerr << "❌ 索引数据库格式错误" << std::endl;
        return false;
    }
    
    return true;
}

bool StorageNode::save_index_database() {
    Json::Value root;
    root["version"] = "3.4";
    root["last_update"] = get_current_timestamp();
    
    // 新格式：file_count, ID_Fs, database
    root["file_count"] = static_cast<int>(index_database.size());
    
    // 生成 ID_Fs 数组
    Json::Value id_fs_array(Json::arrayValue);
    for (const auto& pair : index_database) {
        id_fs_array.append(pair.first);
    }
    root["ID_Fs"] = id_fs_array;
    
    // 生成 database 数组
    Json::Value database_array(Json::arrayValue);
    for (const auto& pair : index_database) {
        const IndexEntry& entry = pair.second;
        
        Json::Value entry_json;
        entry_json["ID_F"] = entry.ID_F;
        entry_json["PK"] = entry.PK;
        entry_json["state"] = entry.state;
        entry_json["file_path"] = entry.file_path;
        
        // 保存 TS_F
        Json::Value ts_f_array(Json::arrayValue);
        for (const auto& ts : entry.TS_F) {
            ts_f_array.append(ts);
        }
        entry_json["TS_F"] = ts_f_array;
        
        // 保存 keywords
        Json::Value keywords_array(Json::arrayValue);
        for (const auto& kw : entry.keywords) {
            Json::Value kw_json;
            kw_json["ptr_i"] = kw.ptr_i;
            kw_json["kt_wi"] = kw.kt_wi;
            kw_json["Ti_bar"] = kw.Ti_bar;
            keywords_array.append(kw_json);
        }
        entry_json["keywords"] = keywords_array;
        
        database_array.append(entry_json);
    }
    root["database"] = database_array;
    
    std::string index_path = data_dir + "/index_db.json";
    return save_json_to_file(root, index_path);
}

// ==================== 节点信息 ====================

bool StorageNode::load_node_info() {
    std::string info_path = data_dir + "/node_info.json";
    
    if (!file_exists(info_path)) {
        return save_node_info();
    }
    
    Json::Value info = load_json_from_file(info_path);
    
    std::cout << "✅ 节点信息加载成功" << std::endl;
    return true;
}

bool StorageNode::save_node_info() {
    Json::Value info;
    
    info["node_id"] = node_id;
    info["version"] = "3.4";
    info["last_update"] = get_current_timestamp();
    info["statistics"]["total_files"] = static_cast<int>(index_database.size());
    info["statistics"]["total_indices"] = static_cast<int>(index_database.size());
    
    std::string info_path = data_dir + "/node_info.json";
    return save_json_to_file(info, info_path);
}

void StorageNode::update_statistics(const std::string& operation) {
    save_node_info();
}

// ==================== 文件操作 (修改) ====================

bool StorageNode::insert_file(const std::string& param_json_path, const std::string& enc_file_path) {
    std::cout << "\n📤 插入文件..." << std::endl;
    
    // 验证密码学系统
    if (!crypto_initialized) {
        std::cerr << "❌ 密码学系统未初始化" << std::endl;
        return false;
    }
    
    // 加载参数JSON
    Json::Value params = load_json_from_file(param_json_path);
    if (params.isNull()) {
        std::cerr << "❌ 参数文件加载失败" << std::endl;
        return false;
    }
    
    // 验证必需字段（注意：ptr字段是可选的）
    if (!params.isMember("PK") || !params.isMember("ID_F") || 
        !params.isMember("TS_F") || !params.isMember("state") || 
        !params.isMember("keywords")) {
        std::cerr << "❌ 参数文件缺少必需字段 (PK, ID_F, TS_F, state, keywords)" << std::endl;
        return false;
    }
    
    std::string PK = params["PK"].asString();
    std::string ID_F = params["ID_F"].asString();
    std::string state = params["state"].asString();
    
    std::cout << "   文件ID: " << ID_F << std::endl;
    std::cout << "   客户端PK: " << PK.substr(0, 16) << "..." << std::endl;
    std::cout << "   状态: " << state << std::endl;
    
    // 验证PK格式
    if (!verify_pk_format(PK)) {
        std::cerr << "❌ PK格式无效" << std::endl;
        return false;
    }
    
    // 检查文件是否已存在
    if (has_file(ID_F)) {
        std::cerr << "❌ 文件ID已存在" << std::endl;
        return false;
    }
    
    // 加载加密文件
    std::string ciphertext = read_file_content(enc_file_path);
    if (ciphertext.empty()) {
        std::cerr << "❌ 加密文件读取失败" << std::endl;
        return false;
    }
    
    // 创建 IndexEntry（统一的数据结构）
    IndexEntry entry;
    entry.ID_F = ID_F;
    entry.PK = PK;
    entry.state = state;
    entry.file_path = files_dir + "/" + ID_F + ".enc";
    
    // 解析 TS_F（文件认证标签）
    Json::Value ts_f_array = params["TS_F"];
    if (ts_f_array.isArray()) {
        for (const auto& tag : ts_f_array) {
            entry.TS_F.push_back(tag.asString());
        }
    } else {
        entry.TS_F.push_back(ts_f_array.asString());
    }
    
    std::cout << "   认证标签数量: " << entry.TS_F.size() << std::endl;
    
    // 解析关键词信息
    Json::Value keywords_array = params["keywords"];
    if (!keywords_array.isArray()) {
        std::cerr << "❌ keywords 字段格式错误（应为数组）" << std::endl;
        return false;
    }
    
    std::cout << "   关键词数量: " << keywords_array.size() << std::endl;
    
    // 处理每个关键词
    for (const auto& kw : keywords_array) {
        // 检查必需字段：Ti_bar 和 kt_wi
        if (!kw.isMember("Ti_bar") || !kw.isMember("kt_wi")) {
            std::cerr << "❌ 关键词格式错误（缺少 Ti_bar 或 kt_wi）" << std::endl;
            return false;
        }
        
        std::string Ti_bar = kw["Ti_bar"].asString();  // 状态令牌（也是搜索令牌）
        std::string kt_wi = kw["kt_wi"].asString();    // 关键词标签
        
        // ptr_i 字段是可选的，如果存在则使用，否则使用 ID_F
        std::string ptr_i = ID_F;  // 默认值
        if (kw.isMember("ptr_i")) {
            ptr_i = kw["ptr_i"].asString();
        }
        
        // 创建 IndexKeywords 结构
        IndexKeywords idx_kw;
        idx_kw.ptr_i = ptr_i;      // 使用提供的指针或文件ID
        idx_kw.kt_wi = kt_wi;      // 关键词标签
        idx_kw.Ti_bar = Ti_bar;    // 状态令牌
        
        entry.keywords.push_back(idx_kw);
        
        std::cout << "   ✅ 已添加关键词索引: " << Ti_bar.substr(0, 16) << "..." << std::endl;
    }
    
    // 修改：直接使用 ID_F 作为键存储到 index_database
    index_database[ID_F] = entry;
    
    
    // 保存加密文件到磁盘
    if (!save_encrypted_file(ID_F, enc_file_path)) {
        std::cerr << "⚠️  加密文件保存失败" << std::endl;
    }
    
    // 保存元数据
    Json::Value metadata;
    metadata["ID_F"] = ID_F;
    metadata["PK"] = PK;
    metadata["state"] = state;
    metadata["file_path"] = entry.file_path;
    metadata["inserted_at"] = get_current_timestamp();
    metadata["ciphertext_size"] = (Json::UInt64)ciphertext.size();
    
    // 保存 TS_F
    Json::Value ts_f_json(Json::arrayValue);
    for (const auto& tag : entry.TS_F) {
        ts_f_json.append(tag);
    }
    metadata["TS_F"] = ts_f_json;
    
    // 保存 keywords
    Json::Value keywords_json(Json::arrayValue);
    for (const auto& kw : entry.keywords) {
        Json::Value kw_obj;
        kw_obj["ptr_i"] = kw.ptr_i;
        kw_obj["kt_wi"] = kw.kt_wi;
        kw_obj["Ti_bar"] = kw.Ti_bar;
        keywords_json.append(kw_obj);
    }
    metadata["keywords"] = keywords_json;
    
    std::string metadata_path = metadata_dir + "/" + ID_F + ".json";
    save_json_to_file(metadata, metadata_path);
    
    // ========== 更新搜索数据库 ==========
    std::cout << "\n🔍 更新搜索数据库..." << std::endl;
    
    // 为每个关键词创建一个 IndexSearchEntry
    for (const auto& kw : entry.keywords) {
        IndexSearchEntry search_entry;
        search_entry.Ti_bar = kw.Ti_bar;
        search_entry.ID_F = ID_F;
        search_entry.ptr_i = kw.ptr_i;
        search_entry.state = entry.state;
        search_entry.kt_wi = kw.kt_wi;
        
        // 以 Ti_bar 为键插入到搜索数据库
        search_database[search_entry.Ti_bar] = search_entry;
        
        std::cout << "   ✅ 添加搜索索引: Ti_bar=" << kw.Ti_bar.substr(0, 16) << "..." << std::endl;
    }
    
    std::cout << "   📊 当前搜索索引总数: " << search_database.size() << std::endl;
    
    // 保存搜索数据库
    save_search_database();
    
    // 保存更新
    save_index_database();
    update_statistics("insert");
    
    std::cout << "✅ 文件插入成功!" << std::endl;
    return true;
}

bool StorageNode::delete_file(const std::string& PK, const std::string& file_id, const std::string& del_proof) {
    // 函数体保持空白 - 待后续实现
    std::cout << "\n🗑️  删除文件功能待实现" << std::endl;
    std::cout << "   文件ID: " << file_id << std::endl;
    std::cout << "   请求者PK: " << PK.substr(0, 16) << "..." << std::endl;
    return false;
}

SearchResult StorageNode::search_keyword(const std::string& PK,
                                        const std::string& search_token, 
                                        const std::string& latest_state) {
    SearchResult result;
    
    // 函数体保持空白 - 待后续实现
    std::cout << "\n🔍 搜索关键词功能待实现" << std::endl;
    std::cout << "   请求者PK: " << PK.substr(0, 16) << "..." << std::endl;
    std::cout << "   搜索令牌: " << search_token.substr(0, 16) << "..." << std::endl;
    
    return result;
}

std::string StorageNode::generate_integrity_proof(const std::string& file_id, 
                                                  const std::string& seed) {
    // 函数体为空 - 根据用户要求保持不实现
    return "";
}

// ==================== 检索函数 (重写) ====================

Json::Value StorageNode::retrieve_file(const std::string& file_id) {
    Json::Value result;
    
    std::cout << "\n📥 检索文件: " << file_id << std::endl;
    
    // 在 index_database 中查找 ID_F
    auto it = index_database.find(file_id);
    if (it == index_database.end()) {
        std::cerr << "❌ 文件不存在" << std::endl;
        result["success"] = false;
        result["error"] = "文件不存在";
        return result;
    }
    
    const IndexEntry& entry = it->second;
    
    // 可选：验证 PK（如果需要身份验证，可以添加 PK 参数）
    // 这里暂时不验证，直接返回文件信息
    
    std::cout << "   ✅ 找到文件" << std::endl;
    std::cout << "   PK: " << entry.PK.substr(0, 16) << "..." << std::endl;
    std::cout << "   状态: " << entry.state << std::endl;
    
    // 构造返回结果
    result["success"] = true;
    result["file_id"] = entry.ID_F;
    result["PK"] = entry.PK;
    result["state"] = entry.state;
    result["file_path"] = entry.file_path;
    
    // 读取加密文件内容
    std::string ciphertext;
    if (load_encrypted_file(file_id, ciphertext)) {
        result["ciphertext"] = ciphertext;
    } else {
        result["ciphertext"] = "";
        std::cerr << "⚠️  无法读取加密文件" << std::endl;
    }
    
    // TS_F
    Json::Value ts_f_array(Json::arrayValue);
    for (const auto& ts : entry.TS_F) {
        ts_f_array.append(ts);
    }
    result["TS_F"] = ts_f_array;
    
    // 提取第一个 TS_F 作为 file_auth_tag（兼容旧接口）
    if (!entry.TS_F.empty()) {
        result["file_auth_tag"] = entry.TS_F[0];
    }
    
    // keywords
    Json::Value keywords_array(Json::arrayValue);
    for (const auto& kw : entry.keywords) {
        Json::Value kw_obj;
        kw_obj["ptr_i"] = kw.ptr_i;
        kw_obj["kt_wi"] = kw.kt_wi;
        kw_obj["Ti_bar"] = kw.Ti_bar;
        keywords_array.append(kw_obj);
    }
    result["keywords"] = keywords_array;
    
    // 提取第一个 ptr_i 作为 pointer（兼容旧接口）
    if (!entry.keywords.empty()) {
        result["pointer"] = entry.keywords[0].ptr_i;
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
    
    for (const auto& pair : index_database) {
        file_list.push_back(pair.first);
    }
    
    return file_list;
}

std::vector<std::string> StorageNode::list_files_by_pk(const std::string& PK) {
    std::vector<std::string> file_list;
    
    for (const auto& pair : index_database) {
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
    std::cout << "   版本:         v3.4 (改进的参数序列化)" << std::endl;
    
    std::cout << "\n📦 存储统计:" << std::endl;
    std::cout << "   文件总数:        " << index_database.size() << std::endl;
    std::cout << "   索引总数:        " << get_index_count() << std::endl;
    std::cout << "   搜索索引总数:    " << search_database.size() << std::endl;
    
    // 统计各状态文件数
    int valid_count = 0;
    int invalid_count = 0;
    for (const auto& pair : index_database) {
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
    
    if (!index_database.empty()) {
        std::cout << "\n📄 文件列表:" << std::endl;
        int count = 0;
        for (const auto& pair : index_database) {
            count++;
            std::cout << "   [" << count << "] " << pair.first 
                     << " (PK: " << pair.second.PK.substr(0, 8) << "..., "
                     << "状态: " << pair.second.state << ")" << std::endl;
            if (count >= 10) {
                std::cout << "   ... (还有 " << (index_database.size() - 10) << " 个文件)" << std::endl;
                break;
            }
        }
    }
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
}

// ==================== 公共参数文件检查 ====================

bool StorageNode::has_public_params_file(const std::string& filepath) const {
    return file_exists(filepath);
}

// ==================== 搜索数据库操作 ====================

bool StorageNode::load_search_database() {
    std::string search_db_path = data_dir + "/search_db.json";
    
    std::cout << "📥 加载搜索数据库..." << std::endl;
    std::cout << "   文件路径: " << search_db_path << std::endl;
    
    // 检查文件是否存在
    if (!file_exists(search_db_path)) {
        std::cout << "   ⚠️  搜索数据库文件不存在，创建新的空数据库" << std::endl;
        
        // 创建空的搜索数据库文件
        Json::Value root;
        root["version"] = "1.0";
        root["created_at"] = get_current_timestamp();
        root["description"] = "Search Database for Quick Keyword Lookup";
        root["search_index_count"] = 0;
        root["search_database"] = Json::Value(Json::arrayValue);
        
        if (!save_json_to_file(root, search_db_path)) {
            std::cerr << "   ❌ 创建搜索数据库文件失败" << std::endl;
            return false;
        }
        
        std::cout << "   ✅ 已创建新的搜索数据库文件" << std::endl;
        return true;
    }
    
    // 加载现有文件
    Json::Value root = load_json_from_file(search_db_path);
    
    if (!root.isMember("search_database")) {
        std::cerr << "   ❌ 搜索数据库格式错误：缺少 search_database 字段" << std::endl;
        return false;
    }
    
    // 清空当前搜索数据库
    search_database.clear();
    
    // 加载搜索索引条目
    const Json::Value& search_db = root["search_database"];
    for (const auto& entry : search_db) {
        IndexSearchEntry search_entry;
        
        // 提取字段
        if (entry.isMember("Ti_bar")) {
            search_entry.Ti_bar = entry["Ti_bar"].asString();
        }
        if (entry.isMember("ID_F")) {
            search_entry.ID_F = entry["ID_F"].asString();
        }
        if (entry.isMember("ptr_i")) {
            search_entry.ptr_i = entry["ptr_i"].asString();
        }
        if (entry.isMember("state")) {
            search_entry.state = entry["state"].asString();
        }
        if (entry.isMember("kt_wi")) {
            search_entry.kt_wi = entry["kt_wi"].asString();
        }
        
        // 以 Ti_bar 为键插入到映射中
        if (!search_entry.Ti_bar.empty()) {
            search_database[search_entry.Ti_bar] = search_entry;
        }
    }
    
    std::cout << "   ✅ 搜索数据库加载成功" << std::endl;
    std::cout << "   📊 搜索索引数量: " << search_database.size() << std::endl;
    
    return true;
}

bool StorageNode::save_search_database() {
    std::string search_db_path = data_dir + "/search_db.json";
    
    Json::Value root;
    
    // 基本信息
    root["version"] = "1.0";
    root["updated_at"] = get_current_timestamp();
    root["description"] = "Search Database for Quick Keyword Lookup";
    root["search_index_count"] = static_cast<int>(search_database.size());
    
    // 序列化搜索数据库
    Json::Value search_db_array(Json::arrayValue);
    
    for (const auto& pair : search_database) {
        const IndexSearchEntry& entry = pair.second;
        
        Json::Value entry_json;
        entry_json["Ti_bar"] = entry.Ti_bar;
        entry_json["ID_F"] = entry.ID_F;
        entry_json["ptr_i"] = entry.ptr_i;
        entry_json["state"] = entry.state;
        entry_json["kt_wi"] = entry.kt_wi;
        
        search_db_array.append(entry_json);
    }
    
    root["search_database"] = search_db_array;
    
    // 保存到文件
    bool success = save_json_to_file(root, search_db_path);
    
    if (success) {
        std::cout << "   💾 搜索数据库已保存: " << search_db_path << std::endl;
        std::cout << "   📊 搜索索引数量: " << search_database.size() << std::endl;
    } else {
        std::cerr << "   ❌ 搜索数据库保存失败" << std::endl;
    }
    
    return success;
}