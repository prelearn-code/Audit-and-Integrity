#include "storage_node.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <cstring>

namespace {
class ScopedTimerServer {
public:
    ScopedTimerServer(PerformanceCallback_s* cb, const std::string& name)
        : cb_(cb), name_(name), active_(cb != nullptr),
          start_(std::chrono::high_resolution_clock::now()) {}
    ~ScopedTimerServer() {
        if (active_) {
            auto end = std::chrono::high_resolution_clock::now();
            double ms = std::chrono::duration<double, std::milli>(end - start_).count();
            cb_->on_phase_complete(name_, ms);
        }
    }
private:
    PerformanceCallback_s* cb_;
    std::string name_;
    bool active_;
    std::chrono::high_resolution_clock::time_point start_;
};
} // namespace

// ==================== 构造函数和析构函数 ====================

StorageNode::StorageNode(const std::string& data_directory, int port) 
    : data_dir(data_directory), server_port(port), crypto_initialized(false) {
    
    files_dir = data_dir + "/EncFiles";
    metadata_dir = data_dir + "/metadata";
    FileProofs_dir = data_dir + "/FileProofs";
    SearchProof_dir = data_dir + "/SearchProof";
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
        mpz_clear(r);  // ✅ 新增：清理群阶r
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
        "r 730750818665451621361119245571504901405976559617\n" //群的阶，元素个数
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
    mpz_init(r);  // ✅ 新增：初始化群阶r
    
    // ✅ 设置群阶r（从pairing参数中提取）
    mpz_set_str(r, "730750818665451621361119245571504901405976559617", 10);
    std::cout << "   群阶 r: 730750818665451621361119245571504901405976559617" << std::endl;
    
    // 设置随机生成器
    element_random(g);
    element_random(mu);
    
    // 从配对参数中提取 p 和 q，计算 N = p × q
    mpz_t p, q;
    mpz_init(p);
    mpz_init(q);
    
    mpz_set_str(p, "8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791", 10);
    
    mpz_nextprime(q, p);
    //mpz_set_str(q, "8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791", 10);


    // 计算 N = p × q
    mpz_mul(N, p, q);
    
    // 输出 N 的信息（截断显示）
    char* n_str = mpz_get_str(NULL, 10, N);
    std::string n_full(n_str);
    free(n_str);
    std::cout << "   N = p × q  " << n_full << "..." << std::endl;
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
    root["version"] = "2.0";
    root["created_at"] = get_current_timestamp();
    root["description"] = "Public Parameters (N, g, μ) for Decentralized Storage System";
    
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
    public_params["g"] = bytesToHex(g_bytes, g_len);
    public_params["g_length"] = g_len;
    delete[] g_bytes;
    
    // μ: G_1的生成元（使用element_to_bytes序列化）
    int mu_len = element_length_in_bytes(mu);
    unsigned char* mu_bytes = new unsigned char[mu_len];
    element_to_bytes(mu_bytes, mu);
    public_params["mu"] = bytesToHex(mu_bytes, mu_len);
    public_params["mu_length"] = mu_len;
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
    
    std::cout << "\n[公共参数 PP = {N, g, μ}]" << std::endl;
    
    // N: 大整数
    std::string n_str = pp["N"].asString();
    std::cout << "N (前50位):   " << n_str.substr(0, 50) << "..." << std::endl;
    std::cout << "N (总位数):   " << n_str.length() << " 位十进制数" << std::endl;
    
    // g: G_1的生成元
    std::string g_str = pp["g"].asString();
   
    int g_len = pp.isMember("g_length") ? pp["g_length"].asInt() : (g_str.length() / 2);
    std::cout << "g (字节长度): " << g_len << " bytes" << std::endl;
    std::cout << "g (hex前40位):" << g_str.substr(0, 40) << "..." << std::endl;
 
    
    // μ: G_1的生成元
    std::string mu_str = pp["mu"].asString();
   
    int mu_len = pp.isMember("mu_length") ? pp["mu_length"].asInt() : (mu_str.length() / 2);
    std::cout << "μ (字节长度): " << mu_len << " bytes" << std::endl;
    std::cout << "μ (hex前40位):" << mu_str.substr(0, 40) << "..." << std::endl;

    
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
    mpz_init(r);  // ✅ 新增：初始化r
    
    // ✅ 设置群阶r（从pairing参数中提取）
    mpz_set_str(r, "730750818665451621361119245571504901405976559617", 10);
    
    // ============ 步骤3: 加载参数到内存 ============
    
    // 加载 N
    if (mpz_set_str(N, n_str.c_str(), 10) != 0) {
        std::cerr << "❌ N 参数格式错误" << std::endl;
        element_clear(g);
        element_clear(mu);
        mpz_clear(N);
        mpz_clear(r);  // ✅ 新增：清理r
        pairing_clear(pairing);
        return false;
    }
    std::cout << "   ✅ 加载 N (" << n_str.length() << " 位十进制数)" << std::endl;
    std::cout << "   ✅ 加载群阶 r (160位)" << std::endl;
    
    // 加载 g - 根据序列化方法选择不同的加载方式
    // g的类型是element_t

    std::vector<unsigned char> g_bytes = hexToBytes(g_str);
    if (g_bytes.empty()) {
        std::cerr << "❌ g 参数hex解码失败" << std::endl;
        element_clear(g);
        element_clear(mu);
        mpz_clear(N);
        mpz_clear(r);  // ✅ 新增
        pairing_clear(pairing);
        return false;
    }
        
    int bytes_read = element_from_bytes(g, g_bytes.data());
    if (bytes_read <= 0) {
        std::cerr << "❌ g 参数反序列化失败 (element_from_bytes返回: " << bytes_read << ")" << std::endl;
        element_clear(g);
        element_clear(mu);
        mpz_clear(N);
        mpz_clear(r);  // ✅ 新增
        pairing_clear(pairing);
        return false;
    }
    std::cout << "   ✅ 加载 g (bytes长度: " << g_bytes.size() << ")" << std::endl;
    
    // 加载 μ - 根据序列化方法选择不同的加载方式
    std::vector<unsigned char> mu_bytes = hexToBytes(mu_str);
    if (mu_bytes.empty()) {
        std::cerr << "❌ μ 参数hex解码失败" << std::endl;
        element_clear(g);
        element_clear(mu);
        mpz_clear(N);
        mpz_clear(r);  // ✅ 新增
        pairing_clear(pairing);
        return false;
    }

    bytes_read = element_from_bytes(mu, mu_bytes.data());
    if (bytes_read <= 0) {
        std::cerr << "❌ μ 参数反序列化失败 (element_from_bytes返回: " << bytes_read << ")" << std::endl;
        element_clear(g);
        element_clear(mu);
        mpz_clear(N);
        mpz_clear(r);  // ✅ 新增
        pairing_clear(pairing);
        return false;
    }

    std::cout << "   ✅ 加载 μ (bytes长度: " << mu_bytes.size() << ")" << std::endl;
    
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
        
        Json::Value root = load_json_from_file(filepath);
        
        if (!root.isMember("public_params")) {
            std::cerr << "❌ 公共参数格式错误" << std::endl;
            return false;
        }
        
        Json::Value pp = root["public_params"];
        
        if (!pp.isMember("N") || !pp.isMember("g") || !pp.isMember("mu")) {
            std::cerr << "❌ 公共参数缺少必需字段 (N, g, μ)" << std::endl;
            return false;
        }
        
        std::cout << "\n📋 文件信息:" << std::endl;
        std::cout << "   版本:         " << root["version"].asString() << std::endl;
        std::cout << "   创建时间:     " << root["created_at"].asString() << std::endl;
        std::cout << "   描述:         " << root["description"].asString() << std::endl;
        
        std::cout << "\n[公共参数 PP = {N, g, μ}]" << std::endl;
        
        std::string n_str = pp["N"].asString();
        std::cout << "   N :   " << n_str << "..." << std::endl;
        std::cout << "   N (总位数):   " << n_str.length() << " 位十进制数" << std::endl;
        
        std::string g_str = pp["g"].asString();
        
        int g_len = pp.isMember("g_length") ? pp["g_length"].asInt() : (g_str.length() / 2);
            std::cout << "   g (字节长度): " << g_len << " bytes" << std::endl;
            std::cout << "   g (hex前40位):" << g_str << "..." << std::endl;
        
        std::string mu_str = pp["mu"].asString();
        
        int mu_len = pp.isMember("mu_length") ? pp["mu_length"].asInt() : (mu_str.length() / 2);
        std::cout << "   μ (字节长度): " << mu_len << " bytes" << std::endl;
        std::cout << "   μ (hex前40位):" << mu_str << "..." << std::endl;
        
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
    
    char* n_str = mpz_get_str(NULL, 10, N);
    std::string n_full(n_str);
    free(n_str);
    std::cout << "   N (前50位):   " << n_full.substr(0, 50) << "..." << std::endl;
    std::cout << "   N (总位数):   " << n_full.length() << " 位十进制数" << std::endl;
    
    int g_len = element_length_in_bytes(g);
    std::cout << "   g (字节长度): " << g_len << " bytes" << std::endl;
    
    int mu_len = element_length_in_bytes(mu);
    std::cout << "   μ (字节长度): " << mu_len << " bytes" << std::endl;
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "✅ 密码学系统状态: 已初始化" << std::endl;
    std::cout << "💡 提示: 这是内存中的当前参数" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    return true;
}

// ==================== 修改后的密码学函数 ====================

void StorageNode::computeHashH1(const std::string& input, mpz_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    mpz_import(result, SHA256_DIGEST_LENGTH, 1, 1, 0, 0, hash);
    mpz_mod(result, result, N);
}

// ✅ 新增：hashToScalar - 将字符串哈希到Zᵣ中（用于所有标量运算）
void StorageNode::hashToScalar(const std::string& input, mpz_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    mpz_import(result, SHA256_DIGEST_LENGTH, 1, 1, 0, 0, hash);
    mpz_mod(result, result, r);  // ✅ 关键：模r而不是模N
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

// ✅ 修改后的compute_prf函数 - 现在使用hashToScalar（输出在Zᵣ中）
void StorageNode::compute_prf(mpz_t result, const std::string& seed, const std::string& ID_F, int index) {
    // 组合输入：seed + ID_F + index
    std::string combined = seed + ID_F + std::to_string(index);
    
    // ✅ 使用hashToScalar计算哈希（自动模r）
    hashToScalar(combined, result);
}

std::string StorageNode::decrypt_pointer(const std::string& current_state_hash, const std::string& encrypted_pointer) {
    if (encrypted_pointer.empty() || encrypted_pointer == std::string(64, '0')) {
        return "";
    }

    std::vector<unsigned char> ciphertext = hexToBytes(encrypted_pointer);
    if (ciphertext.empty()) {
        return "";
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return "";
    }

    unsigned char key[32] = {0};
    for (size_t i = 0; i < 32 && i * 2 < current_state_hash.length(); ++i) {
        std::string byte_str = current_state_hash.substr(i * 2, 2);
        key[i] = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
    }

    unsigned char iv[16] = {0};

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }

    std::vector<unsigned char> plaintext(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int len = 0;
    int total_len = 0;

    if (EVP_DecryptUpdate(ctx,
                          plaintext.data(), &len,
                          ciphertext.data(), static_cast<int>(ciphertext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    total_len += len;

    plaintext.resize(total_len);
    EVP_CIPHER_CTX_free(ctx);

    return std::string(plaintext.begin(), plaintext.end());
}

std::string StorageNode::generate_random_seed() {
    const int seed_length = 32;  // 32字节 = 256位
    unsigned char seed_bytes[seed_length];
    
    // 使用OpenSSL生成随机数
    RAND_bytes(seed_bytes, seed_length);
    
    return bytesToHex(seed_bytes, seed_length);
}

bool StorageNode::verify_pk_format(const std::string& pk) {
    if (pk.empty()) {
        return false;
    }
    
    for (char c : pk) {
        if (!isxdigit(c)) {
            return false;
        }
    }
    
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

std::string StorageNode::bytesToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::vector<unsigned char> StorageNode::hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(std::stoi(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}


// ==================== 序列化辅助函数（方案A：与client.cpp统一）====================

/**
 * serializeElement - 将element_t序列化为hex字符串
 * @param elem 要序列化的元素
 * @return hex字符串
 */
std::string StorageNode::serializeElement(element_t elem) {
    int len = element_length_in_bytes(elem);
    std::vector<unsigned char> buf(len);
    element_to_bytes(buf.data(), elem);
    return bytesToHex(buf.data(), len);
}

/**
 * deserializeElement - 从hex字符串反序列化为element_t（带完整错误检查）
 * @param hex_str hex字符串
 * @param elem 输出参数，反序列化后的元素
 * @return 成功返回true，失败返回false
 */
bool StorageNode::deserializeElement(const std::string& hex_str, element_t elem) {
    if (hex_str.length() % 2 != 0) {
        return false;
    }
    
    std::vector<unsigned char> bytes;
    bytes = hexToBytes(hex_str);
    int bytes_read = element_from_bytes(elem, bytes.data());
    if (bytes_read <= 0) {
        return false;
    }
    return true;  
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
    
    config["version"] = "3.5";
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
    
    config["version"] = "3.5";
    config["node"]["node_id"] = node_id;
    config["node"]["last_update"] = get_current_timestamp();
    
    config["paths"]["data_dir"] = data_dir;
    config["paths"]["files_dir"] = files_dir;
    config["paths"]["metadata_dir"] = metadata_dir;
    
    config["server"]["port"] = server_port;
    
    std::string config_path = data_dir + "/config.json";
    return save_json_to_file(config, config_path);
}

// ==================== 索引数据库操作 ====================

bool StorageNode::load_index_database() {
    std::string index_path = data_dir + "/index_db.json";
    
    if (!file_exists(index_path)) {
        std::cout << "⚠️  索引数据库不存在,将创建新数据库" << std::endl;
        return save_index_database();
    }
    
    Json::Value root = load_json_from_file(index_path);
    
    if (root.isMember("database") && root["database"].isArray()) {
        index_database.clear();
        
        for (const auto& entry_json : root["database"]) {
            IndexEntry entry;
            entry.ID_F = entry_json["ID_F"].asString();
            entry.PK = entry_json["PK"].asString();
            entry.state = entry_json["state"].asString();
            entry.file_path = entry_json.get("file_path", "").asString();
            
            if (entry_json.isMember("TS_F") && entry_json["TS_F"].isArray()) {
                for (const auto& ts : entry_json["TS_F"]) {
                    entry.TS_F.push_back(ts.asString());
                }
            }
            
            if (entry_json.isMember("keywords") && entry_json["keywords"].isArray()) {
                for (const auto& kw_json : entry_json["keywords"]) {
                    IndexKeywords kw;
                    kw.ptr_i = kw_json.get("ptr_i", "").asString();
                    kw.kt_wi = kw_json.get("kt_wi", "").asString();
                    kw.Ti_bar = kw_json.get("Ti_bar", "").asString();
                    entry.keywords.push_back(kw);
                }
            }
            
            index_database[entry.ID_F] = entry;
        }
        
        std::cout << "✅ 索引数据库加载成功 (新格式，共 " << index_database.size() << " 个文件)" << std::endl;
        
    } else if (root.isMember("indices")) {
        std::cout << "⚠️  检测到旧格式数据库，正在转换..." << std::endl;
        index_database.clear();
        
        for (const auto& token : root["indices"].getMemberNames()) {
            for (const auto& entry_json : root["indices"][token]) {
                IndexEntry entry;
                entry.ID_F = entry_json["ID_F"].asString();
                entry.PK = entry_json["PK"].asString();
                entry.state = entry_json["state"].asString();
                entry.file_path = entry_json.get("file_path", "").asString();
                
                if (entry_json.isMember("TS_F") && entry_json["TS_F"].isArray()) {
                    for (const auto& ts : entry_json["TS_F"]) {
                        entry.TS_F.push_back(ts.asString());
                    }
                }
                
                if (entry_json.isMember("keywords") && entry_json["keywords"].isArray()) {
                    for (const auto& kw_json : entry_json["keywords"]) {
                        IndexKeywords kw;
                        kw.ptr_i = kw_json.get("ptr_i", "").asString();
                        kw.kt_wi = kw_json.get("kt_wi", "").asString();
                        kw.Ti_bar = kw_json.get("Ti_bar", "").asString();
                        entry.keywords.push_back(kw);
                    }
                }
                
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
    root["version"] = "3.5";
    root["last_update"] = get_current_timestamp();
    
    root["file_count"] = static_cast<int>(index_database.size());
    
    Json::Value id_fs_array(Json::arrayValue);
    for (const auto& pair : index_database) {
        id_fs_array.append(pair.first);
    }
    root["ID_Fs"] = id_fs_array;
    
    Json::Value database_array(Json::arrayValue);
    for (const auto& pair : index_database) {
        const IndexEntry& entry = pair.second;
        
        Json::Value entry_json;
        entry_json["ID_F"] = entry.ID_F;
        entry_json["PK"] = entry.PK;
        entry_json["state"] = entry.state;
        entry_json["file_path"] = entry.file_path;
        
        Json::Value ts_f_array(Json::arrayValue);
        for (const auto& ts : entry.TS_F) {
            ts_f_array.append(ts);
        }
        entry_json["TS_F"] = ts_f_array;
        
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
    info["version"] = "3.5";
    info["last_update"] = get_current_timestamp();
    info["statistics"]["total_files"] = static_cast<int>(index_database.size());
    info["statistics"]["total_indices"] = static_cast<int>(index_database.size());
    
    std::string info_path = data_dir + "/node_info.json";
    return save_json_to_file(info, info_path);
}

void StorageNode::update_statistics(const std::string& operation) {
    save_node_info();
}

// ==================== 文件操作 ====================

bool StorageNode::insert_file(const std::string& param_json_path, const std::string& enc_file_path) {
    ScopedTimerServer timer(perf_callback_s, "server_insert_total");
    std::cout << "\n📤 插入文件..." << std::endl;
    std::cout << "   参数文件: " << param_json_path << std::endl;
    std::cout << "   加密文件: " << enc_file_path << std::endl;
    
    if (!file_exists(param_json_path)) {
        std::cerr << "❌ 参数文件不存在" << std::endl;
        return false;
    }
    
    Json::Value params = load_json_from_file(param_json_path);
    
    if (!params.isMember("PK") || !params.isMember("ID_F") || 
        !params.isMember("TS_F") || !params.isMember("state") || 
        !params.isMember("keywords")) {
        std::cerr << "❌ 参数文件格式错误（缺少必需字段）" << std::endl;
        return false;
    }
    
    std::string PK = params["PK"].asString();
    std::string ID_F = params["ID_F"].asString();
    std::string state = params["state"].asString();
    
    std::cout << "   文件ID: " << ID_F << std::endl;
    std::cout << "   状态: " << state << std::endl;
    
    if (!verify_pk_format(PK)) {
        std::cerr << "❌ PK格式无效" << std::endl;
        return false;
    }
    
    if (has_file(ID_F)) {
        std::cerr << "❌ 文件ID已存在" << std::endl;
        return false;
    }
    
    std::string ciphertext = read_file_content(enc_file_path);
    if (ciphertext.empty()) {
        std::cerr << "❌ 加密文件读取失败" << std::endl;
        return false;
    }
    
    IndexEntry entry;
    entry.ID_F = ID_F;
    entry.PK = PK;
    entry.state = state;
    entry.file_path = files_dir + "/" + ID_F + ".enc";
    
    Json::Value ts_f_array = params["TS_F"];
    if (ts_f_array.isArray()) {
        for (const auto& tag : ts_f_array) {
            entry.TS_F.push_back(tag.asString());
        }
    } else {
        entry.TS_F.push_back(ts_f_array.asString());
    }
    
    std::cout << "   认证标签数量: " << entry.TS_F.size() << std::endl;
    
    Json::Value keywords_array = params["keywords"];
    if (!keywords_array.isArray()) {
        std::cerr << "❌ keywords 字段格式错误（应为数组）" << std::endl;
        return false;
    }
    
    std::cout << "   关键词数量: " << keywords_array.size() << std::endl;
    
    for (const auto& kw : keywords_array) {
        if (!kw.isMember("Ti_bar") || !kw.isMember("kt_wi")) {
            std::cerr << "❌ 关键词格式错误（缺少 Ti_bar 或 kt_wi）" << std::endl;
            return false;
        }
        
        std::string Ti_bar = kw["Ti_bar"].asString();
        std::string kt_wi = kw["kt_wi"].asString();
        
        std::string ptr_i = ID_F;
        if (kw.isMember("ptr_i")) {
            ptr_i = kw["ptr_i"].asString();
        }
        
        IndexKeywords idx_kw;
        idx_kw.ptr_i = ptr_i;
        idx_kw.kt_wi = kt_wi;
        idx_kw.Ti_bar = Ti_bar;
        
        entry.keywords.push_back(idx_kw);
        
        std::cout << "   ✅ 已添加关键词索引: " << Ti_bar.substr(0, 16) << "..." << std::endl;
    }
    
    index_database[ID_F] = entry;
    
    if (!save_encrypted_file(ID_F, enc_file_path)) {
        std::cerr << "⚠️  加密文件保存失败" << std::endl;
    }
    
    Json::Value metadata;
    metadata["ID_F"] = ID_F;
    metadata["PK"] = PK;
    metadata["state"] = state;
    metadata["file_path"] = entry.file_path;
    metadata["inserted_at"] = get_current_timestamp();
    metadata["ciphertext_size"] = (Json::UInt64)ciphertext.size();
    
    Json::Value ts_f_json(Json::arrayValue);
    for (const auto& tag : entry.TS_F) {
        ts_f_json.append(tag);
    }
    metadata["TS_F"] = ts_f_json;
    
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
    
    std::cout << "\n🔍 更新搜索数据库..." << std::endl;
    
    for (const auto& kw : entry.keywords) {
        IndexSearchEntry search_entry;
        search_entry.Ti_bar = kw.Ti_bar;
        search_entry.ID_F = ID_F;
        search_entry.ptr_i = kw.ptr_i;
        search_entry.state = entry.state;
        search_entry.kt_wi = kw.kt_wi;
        
        search_database[search_entry.Ti_bar] = search_entry;
        
        std::cout << "   ✅ 添加搜索索引: Ti_bar=" << kw.Ti_bar.substr(0, 16) << "..." << std::endl;
    }
    
    std::cout << "   📊 当前搜索索引总数: " << search_database.size() << std::endl;
    
    save_search_database();
    save_index_database();
    update_statistics("insert");
    
    std::cout << "✅ 文件插入成功!" << std::endl;
    return true;
}


// ==================== 新增功能实现 ====================

bool StorageNode::delete_file_from_json(const std::string& delete_json_path) {
    std::cout << "\n🗑️  执行文件删除操作..." << std::endl;
    
    // 步骤1: 加载JSON文件
    if (!file_exists(delete_json_path)) {
        std::cerr << "❌ 删除参数文件不存在: " << delete_json_path << std::endl;
        return false;
    }
    
    Json::Value delete_params = load_json_from_file(delete_json_path);
    
    // 步骤2: 提取参数
    if (!delete_params.isMember("ID_F") || !delete_params.isMember("PK") || 
        !delete_params.isMember("del")) {
        std::cerr << "❌ JSON文件缺少必需字段" << std::endl;
        return false;
    }
    
    std::string ID_F = delete_params["ID_F"].asString();
    std::string PK = delete_params["PK"].asString();
    std::string del = delete_params["del"].asString();
    
    std::cout << "   文件ID: " << ID_F << std::endl;
    std::cout << "   公钥: " << PK.substr(0, 16) << "..." << std::endl;
    
    // 步骤3: 加载数据库
    if (!load_index_database()) {
        std::cerr << "❌ 索引数据库加载失败" << std::endl;
        return false;
    }
    
    if (!load_search_database()) {
        std::cerr << "❌ 搜索数据库加载失败" << std::endl;
        return false;
    }
    
    // 步骤4: 查找文件
    auto it = index_database.find(ID_F);
    if (it == index_database.end()) {
        std::cerr << "❌ 文件不存在: " << ID_F << std::endl;
        return false;
    }
    
    IndexEntry& entry = it->second;
    
    // 步骤5: 验证公钥
    if (entry.PK != PK) {
        std::cerr << "❌ 公钥验证失败，无权删除此文件" << std::endl;
        return false;
    }
    
    // 步骤6: 收集所有Ti_bar并更新索引数据库
    std::vector<std::string> Ti_bars;
    
    std::cout << "   更新关键词标签..." << std::endl;
    for (auto& keyword : entry.keywords) {
        Ti_bars.push_back(keyword.Ti_bar);
        
        // 更新kt_wi = kt_wi / del（大整数除法）
        mpz_t kt_wi_mpz, del_mpz, result_mpz;
        mpz_init(kt_wi_mpz);
        mpz_init(del_mpz);
        mpz_init(result_mpz);
        
        // 将hex字符串转换为mpz_t
        if (mpz_set_str(kt_wi_mpz, keyword.kt_wi.c_str(), 16) != 0) {
            std::cerr << "   ⚠️  kt_wi格式错误，跳过" << std::endl;
            mpz_clear(kt_wi_mpz);
            mpz_clear(del_mpz);
            mpz_clear(result_mpz);
            continue;
        }
        
        if (mpz_set_str(del_mpz, del.c_str(), 16) != 0) {
            std::cerr << "   ⚠️  del格式错误" << std::endl;
            mpz_clear(kt_wi_mpz);
            mpz_clear(del_mpz);
            mpz_clear(result_mpz);
            continue;
        }
        
        // 检查除数是否为0
        if (mpz_cmp_ui(del_mpz, 0) == 0) {
            std::cerr << "   ⚠️  del为0，无法执行除法" << std::endl;
            mpz_clear(kt_wi_mpz);
            mpz_clear(del_mpz);
            mpz_clear(result_mpz);
            continue;
        }
        
        // 执行除法：result = kt_wi / del
        mpz_fdiv_q(result_mpz, kt_wi_mpz, del_mpz);
        
        // 转换回hex字符串
        char* result_str = mpz_get_str(NULL, 16, result_mpz);
        keyword.kt_wi = std::string(result_str);
        free(result_str);
        
        mpz_clear(kt_wi_mpz);
        mpz_clear(del_mpz);
        mpz_clear(result_mpz);
    }
    
    // 步骤7: 设置文件状态为invalid
    entry.state = "invalid";
    std::cout << "   ✅ 文件状态已设置为 invalid" << std::endl;
    
    // 步骤7.5: 清空认证标签（方案A：防止已删除文件被误验证）
    int original_ts_f_count = entry.TS_F.size();
    entry.TS_F.clear();
    std::cout << "   ✅ 已清空认证标签 (原有 " << original_ts_f_count << " 个标签)" << std::endl;
    
    // 步骤8: 更新搜索数据库
    std::cout << "   更新搜索数据库..." << std::endl;
    for (const std::string& Ti_bar : Ti_bars) {
        auto search_it = search_database.find(Ti_bar);
        if (search_it != search_database.end()) {
            IndexSearchEntry& search_entry = search_it->second;
            
            // 更新状态
            search_entry.state = "invalid";
            
            // 更新kt_wi = kt_wi / del
            mpz_t kt_wi_mpz, del_mpz, result_mpz;
            mpz_init(kt_wi_mpz);
            mpz_init(del_mpz);
            mpz_init(result_mpz);
            
            if (mpz_set_str(kt_wi_mpz, search_entry.kt_wi.c_str(), 16) == 0 &&
                mpz_set_str(del_mpz, del.c_str(), 16) == 0 &&
                mpz_cmp_ui(del_mpz, 0) != 0) {
                
                mpz_fdiv_q(result_mpz, kt_wi_mpz, del_mpz);
                
                char* result_str = mpz_get_str(NULL, 16, result_mpz);
                search_entry.kt_wi = std::string(result_str);
                free(result_str);
            }
            
            mpz_clear(kt_wi_mpz);
            mpz_clear(del_mpz);
            mpz_clear(result_mpz);
        }
    }
    
    // 步骤9: 保存数据库
    if (!save_index_database()) {
        std::cerr << "❌ 索引数据库保存失败" << std::endl;
        return false;
    }
    
    if (!save_search_database()) {
        std::cerr << "❌ 搜索数据库保存失败" << std::endl;
        return false;
    }
    
    std::cout << "✅ 文件删除成功" << std::endl;
    std::cout << "   文件ID: " << ID_F << std::endl;
    std::cout << "   更新的Ti_bar数量: " << Ti_bars.size() << std::endl;
    std::cout << "   清空的认证标签数量: " << original_ts_f_count << std::endl;
    
    return true;
}

bool StorageNode::SearchKeywordsAssociatedFilesProof(const std::string& search_json_path) {
    std::cout << "\n🔍 执行关键词关联文件证明搜索..." << std::endl;
    
    // ========== 步骤1: 系统初始化 ==========
    
    // 创建SearchProof目录
    std::string search_proof_dir = data_dir + "/SearchProof";
    if (!create_directory(search_proof_dir)) {
        std::cerr << "❌ 无法创建SearchProof目录" << std::endl;
        return false;
    }
    
    // 加载JSON文件
    if (!file_exists(search_json_path)) {
        std::cerr << "❌ 搜索参数文件不存在: " << search_json_path << std::endl;
        return false;
    }
    
    Json::Value search_params = load_json_from_file(search_json_path);
    
    // 验证必需字段
    if (!search_params.isMember("PK") || !search_params.isMember("T") || 
        !search_params.isMember("std")) {
        std::cerr << "❌ JSON文件缺少必需字段" << std::endl;
        return false;
    }
    
    std::string PK = search_params["PK"].asString();
    std::string T = search_params["T"].asString();
    std::string std_input = search_params["std"].asString();
    
    std::cout << "   公钥: " << PK.substr(0, 16) << "..." << std::endl;
    std::cout << "   搜索令牌: " << T << std::endl;
    
    // ========== 步骤2: 加载数据库 ==========
    
    if (!load_index_database()) {
        std::cerr << "❌ 索引数据库加载失败" << std::endl;
        return false;
    }
    
    if (!load_search_database()) {
        std::cerr << "❌ 搜索数据库加载失败" << std::endl;
        return false;
    }
    
    // ========== 步骤3: 初始化结果容器 ==========
    
    std::vector<std::string> AS;  // 涉及的所有文件ID
    std::vector<SearchResult> PS;  // 搜索结果集合
    
    std::string st_alpha = std_input;  // 当前状态
    std::string st_alpha_next;         // 下一个状态
    
    // 新增：初始化全局phi变量（操作1使用）
    element_t global_phi;
    element_init_G1(global_phi, pairing);
    element_set1(global_phi);  // 初始化为单位元
    
    // 新增：生成随机种子（在循环开始前生成一次）
    std::string search_seed = generate_random_seed();
    std::cout << "   生成搜索种子: " << search_seed.substr(0, 16) << "..." << std::endl;

    // 用于统计计算证明时间（不计入数据库/文件读取）
    double compute_ms_total = 0.0;
    
    // ========== 步骤4: 主搜索循环 ==========
    
    std::cout << "   开始搜索链..." << std::endl;
    int loop_count = 0;
    const int MAX_LOOPS = 1000;  // 防止无限循环
    
    while (loop_count < MAX_LOOPS) {
        loop_count++;
        
        // --- 操作1: 计算Ti_bar并查找 ---
        
        element_t Ti_bar_elem;
        element_init_G1(Ti_bar_elem, pairing);
        computeHashH2(T + st_alpha , Ti_bar_elem);
        
        // 将element转换为hex字符串
        int Ti_bar_len = element_length_in_bytes(Ti_bar_elem);
        unsigned char* Ti_bar_bytes = new unsigned char[Ti_bar_len];
        element_to_bytes(Ti_bar_bytes, Ti_bar_elem);
        std::string Ti_bar = bytesToHex(Ti_bar_bytes, Ti_bar_len);
        delete[] Ti_bar_bytes;
        element_clear(Ti_bar_elem);
        
        std::cout << "   [" << loop_count << "] 查找 Ti_bar: " << Ti_bar.substr(0, 16) << "..." << std::endl;
        
        auto search_it = search_database.find(Ti_bar);
        if (search_it == search_database.end()) {
            std::cout << "   ⚠️  未找到Ti_bar，搜索结束" << std::endl;
            break;
        }
        
        IndexSearchEntry& search_entry = search_it->second;
        std::string ID_F = search_entry.ID_F;
        
        std::cout << "   ✅ 找到文件: " << ID_F << std::endl;
        
        // 查找文件
        auto index_it = index_database.find(ID_F);
        if (index_it == index_database.end()) {
            std::cerr << "❌ 文件不存在: " << ID_F << std::endl;
            break;
        }
        
        IndexEntry& file_entry = index_it->second;
        
        // 验证公钥
        if (file_entry.PK != PK) {
            std::cerr << "❌ 公钥验证失败" << std::endl;
            return false;
        }
        
        // 解密指针获取下一个状态
        std::string st_alpha_hash = computeHashH3(st_alpha);
        st_alpha_next = decrypt_pointer(st_alpha_hash, search_entry.ptr_i);
        
        
        // --- 操作2: 计算证明（仅当state为valid时） ---
        
        if (search_entry.state == "valid") {
            // 记录文件ID，有效文件ID集合
            AS.push_back(ID_F);
            
            // 更新全局phi变量
            element_t kt_wi_elem;
            element_init_G1(kt_wi_elem, pairing);
        
            std::vector<unsigned char> kt_wi_bytes = hexToBytes(search_entry.kt_wi);
            element_from_bytes(kt_wi_elem, kt_wi_bytes.data());
        
            element_mul(global_phi, global_phi, kt_wi_elem);
            element_clear(kt_wi_elem);
            
            std::cout << "   生成证明..." << std::endl;
            
            SearchResult temp_result;
            temp_result.ID_F = ID_F;
            
            // 获取TS_F集合
            const std::vector<std::string>& TS_F = file_entry.TS_F;
            int n = TS_F.size();  // 块数量
            
            std::cout << "   块数量: " << n << std::endl;
            
            // 加载密文文件（不计入计时）
            std::string ciphertext;
            if (!load_encrypted_file(ID_F, ciphertext)) {
                std::cerr << "❌ 无法加载密文文件: " << ID_F << std::endl;
                st_alpha = st_alpha_next;
                continue;
            }
            
            auto proof_start = std::chrono::high_resolution_clock::now();
            
            // 使用在步骤3中生成的search_seed
            std::string seed = search_seed;
            std::cout << "   使用种子: " << seed << "..." << std::endl;
            
            // 初始化累积变量
            mpz_t psi_alpha;
            mpz_init_set_ui(psi_alpha, 0);  // ✅ 修改：初始化为0
            
            element_t phi_element;
            element_init_G1(phi_element, pairing);
            element_set1(phi_element);  // 初始化为单位元
            
            // 遍历每个块（统一改为从0开始）
            for (int i = 0; i < n; ++i) {
                // 计算PRF值（保持PRF使用1-based索引以兼容已有数据）
                mpz_t prf_temp;
                mpz_init(prf_temp);
                compute_prf(prf_temp, seed, ID_F, i);
                
                // 获取第i块的数据
                size_t block_start = i * BLOCK_SIZE;
                size_t block_end = std::min(block_start + BLOCK_SIZE, ciphertext.size());
                std::vector<unsigned char> current_block;
                if (block_end > block_start) {
                    current_block.assign(
                    ciphertext.begin() + block_start,
                    ciphertext.begin() + block_end
                    );
                if (current_block.size() < BLOCK_SIZE) {
                    current_block.resize(BLOCK_SIZE, 0);
                    }
                }
                // 遍历该块的每个扇区
                for (size_t j = 0; j < SECTORS_PER_BLOCK; j++) {
                    size_t sector_start = j * SECTOR_SIZE;
                    size_t sector_end = sector_start + SECTOR_SIZE;
                    
                    // 提取扇区数据
                    std::vector<unsigned char> sector_data(
                        current_block.begin() + sector_start,
                        current_block.begin() + sector_end
                    );
                    
                    // 将扇区数据转换为mpz_t
                    mpz_t C_ij;
                    mpz_init(C_ij);
                    mpz_import(C_ij, sector_data.size(), 1, 1, 0, 0, sector_data.data());
                    
                    // 计算 prf_temp * C_ij
                    mpz_t product;
                    mpz_init(product);
                    mpz_mul(product, prf_temp, C_ij);
                    mpz_mod(product, product, r);  // 防止溢出
                
                    // ✅ 修改：累积并模r（而不是模N）
                    mpz_add(psi_alpha, psi_alpha, product);
                    mpz_mod(psi_alpha, psi_alpha, r);  // ✅ 关键修改：使用r
                    
                    mpz_clear(C_ij);
                    mpz_clear(product);
                }
                
                // 计算 sigma_i^prf_temp
                if (i < (int)TS_F.size()) {
                    element_t sigma_i;
                    element_init_G1(sigma_i, pairing);
                    
                    // 将TS_F[i]转换为element_t
                    std::vector<unsigned char> sigma_bytes = hexToBytes(TS_F[i]);
                    if (!sigma_bytes.empty()) {
                        element_from_bytes(sigma_i, sigma_bytes.data());
                        
                        // 计算 phi_temp = sigma_i^prf_temp
                        element_t phi_temp;
                        element_init_G1(phi_temp, pairing);
                        element_pow_mpz(phi_temp, sigma_i, prf_temp);
                        
                        // 累积：phi_element *= phi_temp
                        element_mul(phi_element, phi_element, phi_temp);
                        
                        element_clear(phi_temp);
                    }
                    
                    element_clear(sigma_i);
                }
                
                mpz_clear(prf_temp);
            }
            
            // 转换结果为字符串
            char* psi_str = mpz_get_str(NULL, 16, psi_alpha);
            temp_result.psi = std::string(psi_str);
            free(psi_str);
            
            // 将phi_element转换为hex字符串
            int phi_len = element_length_in_bytes(phi_element);
            unsigned char* phi_bytes = new unsigned char[phi_len];
            element_to_bytes(phi_bytes, phi_element);
            temp_result.phi = bytesToHex(phi_bytes, phi_len);
            delete[] phi_bytes;
            
            mpz_clear(psi_alpha);
            element_clear(phi_element);
            
            // 添加到PS
            PS.push_back(temp_result);
            
            std::cout << "   ✅ 证明生成完成" << std::endl;

            auto proof_end = std::chrono::high_resolution_clock::now();
            compute_ms_total += std::chrono::duration<double, std::milli>(proof_end - proof_start).count();
        } else {
            std::cout << "   ⚠️  文件状态为 invalid，跳过证明生成" << std::endl;
        }
        
        // --- 操作3: 检查是否继续循环 ---
        
        if (st_alpha == st_alpha_next || st_alpha_next.empty()) {
            std::cout << "   到达链表末尾" << std::endl;
            break;
        }
        
        st_alpha = st_alpha_next;
    }
    
    if (loop_count >= MAX_LOOPS) {
        std::cerr << "⚠️  达到最大循环次数，强制退出" << std::endl;
    }
    
    // ========== 步骤5: 生成输出JSON ==========
    
    std::cout << "   生成输出文件..." << std::endl;
    
    Json::Value output;
    output["T"] = T;
    output["std"] = std_input;
    
    // 新增：添加 seed 字段
    output["seed"] = search_seed;
    
    // 新增：添加 phi 字段
    int phi_len = element_length_in_bytes(global_phi);
    unsigned char* phi_bytes = new unsigned char[phi_len];
    element_to_bytes(phi_bytes, global_phi);
    output["phi"] = bytesToHex(phi_bytes, phi_len);
    delete[] phi_bytes;
    
    Json::Value as_array(Json::arrayValue);
    for (const std::string& id : AS) {
        as_array.append(id);
    }
    output["AS"] = as_array;   
    
    Json::Value ps_array(Json::arrayValue);
    for (const SearchResult& result : PS) {
        Json::Value ps_item;
        ps_item["ID_F"] = result.ID_F;
        ps_item["psi_alpha"] = result.psi;
        ps_item["phi_alpha"] = result.phi;
        ps_array.append(ps_item);
    }
    output["PS"] = ps_array;
    
    // ========== 步骤6: 保存结果文件 ==========
    
    std::string output_path = search_proof_dir + "/" + T + ".json";
    if (!save_json_to_file(output, output_path)) {
        std::cerr << "❌ 搜索结果保存失败" << std::endl;
        return false;
    }
    
    std::cout << "✅ 搜索证明生成成功" << std::endl;
    std::cout << "   输出文件: " << output_path << std::endl;
    std::cout << "   涉及文件数: " << AS.size() << std::endl;
    std::cout << "   有效证明数: " << PS.size() << std::endl;

    if (perf_callback_s) {
        perf_callback_s->on_phase_complete("server_search_total", compute_ms_total);
    }
    
    // 新增：清理资源
    element_clear(global_phi);
    
    return true;
}

// 生成文件证明
bool StorageNode::GetFileProof(const std::string& ID_F) {
    std::cout << "\n📄 生成文件证明..." << std::endl;
    std::cout << "   文件ID: " << ID_F << std::endl;
    
    // ========== 步骤1：系统初始化 ==========
    
    // 创建FileProofs目录
    std::string file_proofs_dir = data_dir + "/FileProofs";
    if (!create_directory(file_proofs_dir)) {
        std::cerr << "❌ 无法创建FileProofs目录" << std::endl;
        return false;
    }
    
    // ========== 步骤2：加载索引数据库并查找文件 ==========
    
    // 加载索引数据库
    if (!load_index_database()) {
        std::cerr << "❌ 索引数据库加载失败" << std::endl;
        return false;
    }
    
    // 查找文件
    auto it = index_database.find(ID_F);
    if (it == index_database.end()) {
        std::cerr << "❌ 文件不存在: " << ID_F << std::endl;
        return false;
    }
    
    const IndexEntry& entry = it->second;
    std::cout << "   ✅ 找到文件" << std::endl;
    
    // ========== 步骤2.5：检查文件状态（防止为已删除文件生成证明）==========
    if (entry.state != "valid") {
        std::cerr << "❌ 文件状态为 " << entry.state << "，无法生成证明" << std::endl;
        return false;
    }
    
    if (entry.TS_F.empty()) {
        std::cerr << "❌ 文件无认证标签，无法生成证明" << std::endl;
        return false;
    }
    // ===================================================================
    
    // 获取TS_F和公钥
    const std::vector<std::string>& TS_F = entry.TS_F;
    int n = TS_F.size();  // 块数量
    std::string PK = entry.PK;
    
    std::cout << "   块数量: " << n << std::endl;
    
    // ========== 步骤3：加载密文文件 ==========
    
    // 加载密文内容
    std::string ciphertext;
    if (!load_encrypted_file(ID_F, ciphertext)) {
        std::cerr << "❌ 无法加载密文文件: " << ID_F << std::endl;
        return false;
    }
    
    std::cout << "   密文大小: " << ciphertext.size() << " bytes" << std::endl;
    
    // ========== 步骤4：生成随机种子 ==========
    
    // 生成随机种子
    std::string seed = generate_random_seed();
    std::cout << "   随机种子: " << seed << "..." << std::endl;
    
    // ========== 步骤5：初始化累积变量 ==========
    
    // 初始化FileProof结构
    FileProof fileproof;
    
    // 初始化phi（G1元素，初始值为1）
    element_t phi_element;
    element_init_G1(phi_element, pairing);
    element_set1(phi_element);
    
    // 初始化psi（大整数，初始值为0）
    mpz_t psi_mpz;
    mpz_init_set_ui(psi_mpz, 0);
    
    // ========== 步骤6：主循环 - 遍历所有块 ==========
    
    // 遍历每个块（统一改为从0开始）
    for (int i = 0; i < n; ++i) {
        std::cout << "   处理块 " << (i) << "/" << n << std::endl;
        
        // 步骤6.1：计算PRF值（保持PRF使用1-based索引以兼容已有数据）
        mpz_t prf_result;
        mpz_init(prf_result);
        compute_prf(prf_result, seed, ID_F, i);
        
        // 步骤6.2：处理该块的所有扇区
        size_t block_start = i * BLOCK_SIZE;
        size_t block_end = std::min(block_start + BLOCK_SIZE, ciphertext.size());
        std::vector<unsigned char> current_block;
        if (block_end > block_start) {
            current_block.assign(
                ciphertext.begin() + block_start,
                ciphertext.begin() + block_end
            );
            if (current_block.size() < BLOCK_SIZE) {
                current_block.resize(BLOCK_SIZE, 0);
            }
        }

        for (size_t j = 0; j < SECTORS_PER_BLOCK; j++) {
            size_t sector_start = j * SECTOR_SIZE;
            size_t sector_end = sector_start + SECTOR_SIZE;
            
            // 提取扇区数据 c_(i,j)
            std::vector<unsigned char> sector_data(
                current_block.begin() + sector_start,
                current_block.begin() + sector_end
            );
            
            // 将扇区数据转换为mpz_t
            mpz_t C_ij;
            mpz_init(C_ij);
            mpz_import(C_ij, sector_data.size(), 1, 1, 0, 0, sector_data.data());
            
            // 计算 prf_result * C_ij
            mpz_t product;
            mpz_init(product);
            mpz_mul(product, prf_result, C_ij);
            
            // ✅ 修改：累加并模r（而不是模N）
            mpz_add(psi_mpz, psi_mpz, product);
            // 换了模操作
            mpz_mod(psi_mpz, psi_mpz, r);  // ✅ 关键修改：使用r模
            
            mpz_clear(C_ij);
            mpz_clear(product);
        }
        
        // 步骤6.3：计算 phi *= (theta_i)^prf_result
        if (i < (int)TS_F.size()) {
            element_t theta_i;
            element_init_G1(theta_i, pairing);
            
            // 将TS_F[i]转换为element_t
            std::vector<unsigned char> theta_bytes = hexToBytes(TS_F[i]);
            if (!theta_bytes.empty()) {
                element_from_bytes(theta_i, theta_bytes.data());
                
                // 计算 theta_i^prf_result
                element_t phi_temp;
                element_init_G1(phi_temp, pairing);
                element_pow_mpz(phi_temp, theta_i, prf_result);
                
                // 累乘：phi_element *= phi_temp
                element_mul(phi_element, phi_element, phi_temp);
                
                element_clear(phi_temp);
            }
            
            element_clear(theta_i);
        }
        
        mpz_clear(prf_result);
    }
    
    // ========== 步骤7：转换结果并构建JSON ==========
    
    // 转换psi为十六进制字符串
    char* psi_str = mpz_get_str(NULL, 16, psi_mpz);
    fileproof.psi = std::string(psi_str);
    free(psi_str);
    
    // 转换phi为十六进制字符串
    int phi_len = element_length_in_bytes(phi_element);
    unsigned char* phi_bytes = new unsigned char[phi_len];
    element_to_bytes(phi_bytes, phi_element);
    fileproof.phi = bytesToHex(phi_bytes, phi_len);
    delete[] phi_bytes;
    
    // 清理资源
    mpz_clear(psi_mpz);
    element_clear(phi_element);
    
    std::cout << "   ✅ 证明计算完成" << std::endl;
    
    // ========== 步骤8：生成输出JSON文件 ==========
    
    // 构建JSON输出
    Json::Value output;
    output["ID_F"] = ID_F;
    
    Json::Value fileproof_json;
    fileproof_json["psi"] = fileproof.psi;
    fileproof_json["phi"] = fileproof.phi;
    output["FileProof"] = fileproof_json;
    
    output["seed"] = seed;
    
    // 保存到文件
    std::string output_path = file_proofs_dir + "/" + ID_F + ".json";
    if (!save_json_to_file(output, output_path)) {
        std::cerr << "❌ 文件证明保存失败" << std::endl;
        return false;
    }
    
    std::cout << "✅ 文件证明生成成功" << std::endl;
    std::cout << "   输出文件: " << output_path << std::endl;
    
    return true;
}

bool StorageNode::VerifySearchProof(const std::string& search_proof_json_path) {
    std::cout << "\n🔍 验证搜索证明..." << std::endl;
    
    // ========== 步骤1：加载输入JSON ==========
    
    // 检查文件是否存在
    if (!file_exists(search_proof_json_path)) {
        std::cerr << "❌ 搜索证明文件不存在: " << search_proof_json_path << std::endl;
        return false;
    }
    
    // 加载JSON文件
    Json::Value proof_data = load_json_from_file(search_proof_json_path);
    
    // 验证必需字段
    if (!proof_data.isMember("AS") || !proof_data.isMember("PS") ||
        !proof_data.isMember("T") || !proof_data.isMember("std") ||
        !proof_data.isMember("seed") || !proof_data.isMember("phi")) {
        std::cerr << "❌ 搜索证明文件缺少必需字段" << std::endl;
        return false;
    }
    
    std::cout << "   ✅ 证明文件加载成功" << std::endl;
    
    // ========== 步骤2：提取数据 ==========
    
    // 提取数据
    const Json::Value& AS = proof_data["AS"];
    const Json::Value& PS = proof_data["PS"];
    std::string T = proof_data["T"].asString();
    std::string std_input = proof_data["std"].asString();
    std::string seed = proof_data["seed"].asString();
    std::string phi_input = proof_data["phi"].asString();
    
    int file_nums = AS.size();
    
    std::cout << "   文件数量: " << file_nums << std::endl;
    std::cout << "   证明数量: " << PS.size() << std::endl;
    std::cout << "   种子: " << seed.substr(0, 16) << "..." << std::endl;
    
    // ========== 步骤3：加载索引数据库并获取参数 ==========
    
    // 加载索引数据库
    if (!load_index_database()) {
        std::cerr << "❌ 索引数据库加载失败" << std::endl;
        return false;
    }

    // 仅对验证计算过程计时（不含文件/DB加载）
    ScopedTimerServer timer(perf_callback_s, "server_search_verify_total");
    
    // 获取第一个文件的索引信息（用于获取n和PK）
    if (AS.empty()) {
        std::cerr << "❌ AS数组为空" << std::endl;
        return false;
    }

    std::string first_ID_F = AS[0].asString();
    auto it = index_database.find(first_ID_F);
    if (it == index_database.end()) {
        std::cerr << "❌ 文件不存在: " << first_ID_F << std::endl;
        return false;
    }
    
    int n;  // 块数量
    std::string PK = it->second.PK;   // 公钥
    
    // ========== 步骤4：初始化变量 ==========
    
    // 初始化变量：zeta_1, zeta_2, zeta_3, pho
    element_t zeta_1, zeta_2, zeta_3;
    element_init_G1(zeta_1, pairing);
    element_init_G1(zeta_2, pairing);
    element_init_G1(zeta_3, pairing);
    
    element_set1(zeta_1);  // zeta_1 = 1
    element_set1(zeta_2);  // zeta_2 = 1
    
    // zeta_3 = phi (从输入中读取)
    if (!deserializeElement(phi_input, zeta_3)) {
        std::cerr << "❌ phi反序列化失败" << std::endl;
        element_clear(zeta_1);
        element_clear(zeta_2);
        element_clear(zeta_3);
        return false;
    }
    
    // pho 初始化为0（大整数）
    mpz_t pho;
    mpz_init_set_ui(pho, 0);
    
    // ========== 步骤5：主循环 - 遍历PS ==========
    
    std::cout << "   开始验证计算..." << std::endl;
    
    // 遍历PS数组
    for (int t = 0; t < file_nums; t++) {
        if (t >= (int)PS.size()) {
            std::cerr << "⚠️  PS数组元素不足" << std::endl;
            break;
        }
        
        const Json::Value& ps_item = PS[t];
        std::string ID_F = ps_item["ID_F"].asString();
        std::string phi_alpha = ps_item["phi_alpha"].asString();
        std::string psi_alpha = ps_item["psi_alpha"].asString();
        it = index_database.find(ID_F);
        if (it == index_database.end()) {
            std::cerr << "⚠️  文件不存在: " << ID_F << std::endl;
            continue;
        }
        // 每个文件都进行更新n,即文件的块数，每个文件可能不同
        n = it->second.TS_F.size();  // 块数量（确保使用正确的n）
        std::cout << "   块数量 n: " << n << std::endl;
        std::cout << "   [" << (t+1) << "/" << file_nums << "] 处理文件: " 
                  << ID_F.substr(0, 16) << "..." << std::endl;
        
        // 步骤5.1：计算 h2_temp_2 = H2(ID_F)
        element_t h2_temp_2;
        element_init_G1(h2_temp_2, pairing);
        computeHashH2(ID_F, h2_temp_2);
        
        // 步骤5.2：累乘 zeta_2 *= h2_temp_2
        element_mul(zeta_2, zeta_2, h2_temp_2);
        element_clear(h2_temp_2);
        
        // 步骤5.3：累乘 zeta_3 *= phi_alpha
        element_t phi_alpha_elem;
        element_init_G1(phi_alpha_elem, pairing);
        if (deserializeElement(phi_alpha, phi_alpha_elem)) {
            element_mul(zeta_3, zeta_3, phi_alpha_elem);
        } else {
            std::cerr << "⚠️  phi_alpha反序列化失败，跳过此项" << std::endl;
        }
        element_clear(phi_alpha_elem);
        
        // 步骤5.4：累加 pho += psi_alpha
        mpz_t psi_alpha_mpz;
        mpz_init(psi_alpha_mpz);
        if (mpz_set_str(psi_alpha_mpz, psi_alpha.c_str(), 16) == 0) {
            mpz_add(pho, pho, psi_alpha_mpz);
            mpz_mod(pho, pho, r);  // ✅ 关键修改：使用r
        }
        mpz_clear(psi_alpha_mpz);
        
        // 步骤5.5：内循环 - 遍历所有块（统一改为从0开始）
        for (int i = 0; i < n; ++i) {

            mpz_t prf_temp;
            mpz_init(prf_temp);
            compute_prf(prf_temp, seed, ID_F, i);
            
            // 计算 h2_temp_1 = H2(ID_F || i)
            std::string id_with_index = ID_F + std::to_string(i);
            element_t h2_temp_1;
            element_init_G1(h2_temp_1, pairing);
            computeHashH2(id_with_index, h2_temp_1);
            
            // 计算 h2_temp_1^prf_temp
            element_t temp_pow;
            element_init_G1(temp_pow, pairing);
            element_pow_mpz(temp_pow, h2_temp_1, prf_temp);
            
            // 累乘 zeta_1 *= temp_pow
            element_mul(zeta_1, zeta_1, temp_pow);
            
            element_clear(h2_temp_1);
            element_clear(temp_pow);
            mpz_clear(prf_temp);
        }
    }
    
    std::cout << "   ✅ 计算完成" << std::endl;
    
    // ========== 步骤6：构建验证等式 ==========
    
    // 步骤6.1：计算 left = e(zeta_3, g)
    element_t left_pairing;
    element_init_GT(left_pairing, pairing);
    pairing_apply(left_pairing, zeta_3, g, pairing);
    
    // 步骤6.2：计算 Ti_bar_temp = H2(T||std)
    element_t Ti_bar_temp;
    element_init_G1(Ti_bar_temp, pairing);
    computeHashH2(T + std_input, Ti_bar_temp);
    
    // 步骤6.3：计算 mu^pho
    element_t mu_pow_pho;
    element_init_G1(mu_pow_pho, pairing);
    element_pow_mpz(mu_pow_pho, mu, pho);
    
    // 步骤6.4：计算 right_g1 = zeta_1 * zeta_2 * Ti_bar_temp * mu^pho
    element_t right_g1;
    element_init_G1(right_g1, pairing);
    element_set1(right_g1);
    element_mul(right_g1, right_g1, zeta_1);
    element_mul(right_g1, right_g1, zeta_2);
    element_mul(right_g1, right_g1, Ti_bar_temp);
    element_mul(right_g1, right_g1, mu_pow_pho);
    
    // 步骤6.5：将PK从hex转换为element_t
    element_t PK_elem;
    element_init_G1(PK_elem, pairing);
    if (!deserializeElement(PK, PK_elem)) {
        std::cerr << "❌ PK反序列化失败" << std::endl;
        // 清理资源并返回
        element_clear(zeta_1);
        element_clear(zeta_2);
        element_clear(zeta_3);
        mpz_clear(pho);
        element_clear(left_pairing);
        element_clear(Ti_bar_temp);
        element_clear(mu_pow_pho);
        element_clear(right_g1);
        element_clear(PK_elem);
        return false;
    }
    
    // 步骤6.6：计算 right = e(right_g1, PK)
    element_t right_pairing;
    element_init_GT(right_pairing, pairing);
    pairing_apply(right_pairing, right_g1, PK_elem, pairing);
    
    // ========== 步骤7：验证等式 ==========
    
    // 步骤7：验证 left == right
    std::cout << "   验证配对等式..." << std::endl;
    
    int comparison = element_cmp(left_pairing, right_pairing);
    
    // test
    std::cout << "对比左右的结果："<< comparison << std::endl;

    bool verification_result = (comparison == 0);

    
    // 清理资源
    element_clear(zeta_1);
    element_clear(zeta_2);
    element_clear(zeta_3);
    mpz_clear(pho);
    element_clear(left_pairing);
    element_clear(right_pairing);
    element_clear(Ti_bar_temp);
    element_clear(mu_pow_pho);
    element_clear(right_g1);
    element_clear(PK_elem);
    
    if (verification_result) {
        std::cout << "✅ 搜索证明验证成功" << std::endl;
    } else {
        std::cout << "❌ 搜索证明验证失败" << std::endl;
    }
    
    return verification_result;
}

bool StorageNode::VerifyFileProof(const std::string& file_proof_json_path) {
    std::cout << "\n🔐 验证文件证明..." << std::endl;
    
    // ========== 步骤1：加载输入JSON ==========
    
    // 检查文件是否存在
    if (!file_exists(file_proof_json_path)) {
        std::cerr << "❌ 文件证明不存在: " << file_proof_json_path << std::endl;
        return false;
    }
    
    // 加载JSON文件
    Json::Value proof_data = load_json_from_file(file_proof_json_path);
    
    // 验证必需字段
    if (!proof_data.isMember("ID_F") || !proof_data.isMember("FileProof") ||
        !proof_data.isMember("seed")) {
        std::cerr << "❌ 文件证明缺少必需字段" << std::endl;
        return false;
    }
    
    std::cout << "   ✅ 证明文件加载成功" << std::endl;
    
    // ========== 步骤2：提取数据 ==========
    
    // 提取数据
    std::string ID_F = proof_data["ID_F"].asString();
    std::string seed = proof_data["seed"].asString();
    
    const Json::Value& fileproof_json = proof_data["FileProof"];
    std::string psi = fileproof_json["psi"].asString();
    std::string phi = fileproof_json["phi"].asString();
    
    std::cout << "   文件ID: " << ID_F << std::endl;
    std::cout << "   种子: " << seed << std::endl;
    
    // ========== 步骤3：加载索引数据库并获取参数 ==========
    
    // 加载索引数据库
    if (!load_index_database()) {
        std::cerr << "❌ 索引数据库加载失败" << std::endl;
        return false;
    }
    
    // 查找文件
    auto it = index_database.find(ID_F);
    if (it == index_database.end()) {
        std::cerr << "❌ 文件不存在: " << ID_F << std::endl;
        return false;
    }
    // 4块
    int n = it->second.TS_F.size();  // 块数量
    std::string PK = it->second.PK;   // 公钥
    
    std::cout << "   块数量 n: " << n << std::endl;
    
    // ========== 步骤4：计算zeta ==========
    
    // 初始化zeta = 1
    element_t zeta;
    element_init_G1(zeta, pairing);
    element_set1(zeta);
    
    std::cout << "   计算zeta..." << std::endl;
    
    // 循环计算zeta（统一改为从0开始）
    for (int i = 0; i < n; ++i) {
        // 计算prf_temp
        mpz_t prf_temp;
        mpz_init(prf_temp);
        compute_prf(prf_temp, seed, ID_F, i);
        
        // 计算h2_temp = H2(ID_F || i)
        std::string id_with_index = ID_F + std::to_string(i);
        element_t h2_temp;
        element_init_G1(h2_temp, pairing);
        computeHashH2(id_with_index, h2_temp);
        
        // 计算h2_temp^prf_temp
        element_t temp_pow;
        element_init_G1(temp_pow, pairing);
        element_pow_mpz(temp_pow, h2_temp, prf_temp);
        
        
        // 累乘：zeta *= temp_pow
        element_mul(zeta, zeta, temp_pow);
        
        element_clear(h2_temp);
        element_clear(temp_pow);
        mpz_clear(prf_temp);
    }
    
    std::cout << "   ✅ zeta计算完成" << std::endl;
    
    // ========== 步骤5：构建验证等式 ==========
    
    // 将phi从hex转换为element_t
    element_t phi_elem;
    element_init_G1(phi_elem, pairing);
    if (!deserializeElement(phi, phi_elem)) {
        std::cerr << "❌ phi反序列化失败" << std::endl;
        element_clear(zeta);
        element_clear(phi_elem);
        return false;
    }
    
    // 将psi从hex转换为mpz_t
    mpz_t psi_mpz;
    mpz_init(psi_mpz);
    mpz_set_str(psi_mpz, psi.c_str(), 16);
    
    // 计算left = e(phi, g)
    element_t left_pairing;
    element_init_GT(left_pairing, pairing);
    pairing_apply(left_pairing, phi_elem, g, pairing);
    
    // 计算mu^psi
    element_t mu_pow_psi;
    element_init_G1(mu_pow_psi, pairing);
    element_pow_mpz(mu_pow_psi, mu, psi_mpz);
    
    // 计算right_g1 = zeta * mu^psi
    element_t right_g1;
    element_init_G1(right_g1, pairing);
    element_mul(right_g1, zeta, mu_pow_psi);
    
    // 将PK从hex转换为element_t
    element_t PK_elem;
    element_init_G1(PK_elem, pairing);
    if (!deserializeElement(PK, PK_elem)) {
        std::cerr << "❌ PK反序列化失败" << std::endl;
        // 清理资源并返回
        element_clear(zeta);
        element_clear(phi_elem);
        mpz_clear(psi_mpz);
        element_clear(left_pairing);
        element_clear(mu_pow_psi);
        element_clear(right_g1);
        element_clear(PK_elem);
        return false;
    }
    
    // 计算right = e(right_g1, PK)
    element_t right_pairing;
    element_init_GT(right_pairing, pairing);
    pairing_apply(right_pairing, right_g1, PK_elem, pairing);
    
    // ========== 步骤6：验证等式 ==========
    
    // 验证等式：left == right
    std::cout << "   验证配对等式..." << std::endl;
    
    int comparison = element_cmp(left_pairing, right_pairing);
    bool verification_result = (comparison == 0);
    
    // 清理资源
    element_clear(zeta);
    element_clear(phi_elem);
    mpz_clear(psi_mpz);
    element_clear(left_pairing);
    element_clear(right_pairing);
    element_clear(mu_pow_psi);
    element_clear(right_g1);
    element_clear(PK_elem);
    
    if (verification_result) {
        std::cout << "✅ 文件证明验证成功" << std::endl;
    } else {     
        std::cout << "❌ 文件证明验证失败" << std::endl;
    }
    
    return verification_result;
}

// ==================== 检索函数 ====================

Json::Value StorageNode::retrieve_file(const std::string& file_id) {
    Json::Value result;
    
    std::cout << "\n📥 检索文件: " << file_id << std::endl;
    
    auto it = index_database.find(file_id);
    if (it == index_database.end()) {
        std::cerr << "❌ 文件不存在" << std::endl;
        result["success"] = false;
        result["error"] = "文件不存在";
        return result;
    }
    
    const IndexEntry& entry = it->second;
    
    std::cout << "   ✅ 找到文件" << std::endl;
    std::cout << "   PK: " << entry.PK.substr(0, 16) << "..." << std::endl;
    std::cout << "   状态: " << entry.state << std::endl;
    
    result["success"] = true;
    result["file_id"] = entry.ID_F;
    result["PK"] = entry.PK;
    result["state"] = entry.state;
    result["file_path"] = entry.file_path;
    
    std::string ciphertext;
    if (load_encrypted_file(file_id, ciphertext)) {
        result["ciphertext"] = ciphertext;
    } else {
        result["ciphertext"] = "";
        std::cerr << "⚠️  无法读取加密文件" << std::endl;
    }
    
    Json::Value ts_f_array(Json::arrayValue);
    for (const auto& ts : entry.TS_F) {
        ts_f_array.append(ts);
    }
    result["TS_F"] = ts_f_array;
    
    if (!entry.TS_F.empty()) {
        result["file_auth_tag"] = entry.TS_F[0];
    }
    
    Json::Value keywords_array(Json::arrayValue);
    for (const auto& kw : entry.keywords) {
        Json::Value kw_obj;
        kw_obj["ptr_i"] = kw.ptr_i;
        kw_obj["kt_wi"] = kw.kt_wi;
        kw_obj["Ti_bar"] = kw.Ti_bar;
        keywords_array.append(kw_obj);
    }
    result["keywords"] = keywords_array;
    
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

// ==================== 搜索数据库操作 ====================

bool StorageNode::load_search_database() {
    std::string search_db_path = data_dir + "/search_db.json";
    
    std::cout << "📥 加载搜索数据库..." << std::endl;
    std::cout << "   文件路径: " << search_db_path << std::endl;
    
    if (!file_exists(search_db_path)) {
        std::cout << "   ⚠️  搜索数据库文件不存在，创建新的空数据库" << std::endl;
        
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
    
    Json::Value root = load_json_from_file(search_db_path);
    
    if (!root.isMember("search_database")) {
        std::cerr << "   ❌ 搜索数据库格式错误：缺少 search_database 字段" << std::endl;
        return false;
    }
    
    search_database.clear();
    
    const Json::Value& search_db = root["search_database"];
    for (const auto& entry : search_db) {
        IndexSearchEntry search_entry;
        
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
    
    root["version"] = "1.0";
    root["updated_at"] = get_current_timestamp();
    root["description"] = "Search Database for Quick Keyword Lookup";
    root["search_index_count"] = static_cast<int>(search_database.size());
    
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
    
    bool success = save_json_to_file(root, search_db_path);
    
    if (success) {
        std::cout << "   💾 搜索数据库已保存: " << search_db_path << std::endl;
        std::cout << "   📊 搜索索引数量: " << search_database.size() << std::endl;
    } else {
        std::cerr << "   ❌ 搜索数据库保存失败" << std::endl;
    }
    
    return success;
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
    std::cout << "   版本:         v3.5 (新增删除和搜索证明功能)" << std::endl;
    
    std::cout << "\n📦 存储统计:" << std::endl;
    std::cout << "   文件总数:        " << index_database.size() << std::endl;
    std::cout << "   索引总数:        " << get_index_count() << std::endl;
    std::cout << "   搜索索引总数:    " << search_database.size() << std::endl;
    
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
