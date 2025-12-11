#include<iostream>
#include <cstring>
#include <vector>
#include <pbc/pbc.h>
#include <gmp.h>
#include <openssl/sha.h>


struct testFileProof {
    mpz_t psi;   // ψ值（累积证明）
    element_t phi;   // φ值（累积签名）
    std::string seed; // 客户端公钥
    std::string ID_F; // 文件ID
    std::string pk;   // 公钥
};


void testVerifyFileProof_str();
void testVerifyFileProof_struct(testFileProof file_proof);
void testGetFileProof(std::string file_id, testFileProof& file_proof);
void computeHashH2(const std::string& input, element_t result);
void compute_prf(mpz_t result, const std::string& seed, const std::string& ID_F, int index, mpz_t N);
void computeHashH1(const std::string& input, mpz_t result, mpz_t N);
bool deserializeElement(const std::string& hex_str, element_t elem);
std::vector<unsigned char> hexToBytes(const std::string& hex);
std::string bytesToHex(const unsigned char* data, size_t len);
bool load_index_database();

int main()
{
    testFileProof file_proof;
    file_proof.phi = nullptr;
    file_proof.psi = nullptr;
    file_proof.seed = "";
    std::string file_id = "";
    file_proof.ID_F = file_id;
    file_proof.pk = "9d098b372d6c8944b1c6119500b57ae1d06951a6c7563140ced8d66e59a50269cd5be7d2f25bcd289dfbfcbad4254fb9d62f21e6b436391e186aead8227fa63ba3be5cea8873f186c647c01d1918caca9c6769d9d33acbba09ac9da048b46e86cf728a8c9593be309e106b4d08b11b85b1d22ba12585606e195350fa264450d1";

    testGetFileProof(file_id, file_proof);
    testVerifyFileProof_struct();
    return 0;
}
void testGetFileProof(std::string file_id, testFileProof& file_proof)
{
    const char* param_str = 
        "type a\n"
        "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
        "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
        "r 730750818665451621361119245571504901405976559617\n"
        "exp2 159\n"
        "exp1 107\n"
        "sign1 1\n"
        "sign0 1\n";

    pairing_t pairing;
    pairing_init_set_buf(pairing, param_str, strlen(param_str));

    std::cout << "✅ pairing 初始化成功" << std::endl;
  
    std::cout << "GetFileProof start" << std::endl;
    // 首先加载数据库
    if (!load_index_database()) {
        std::cerr << "❌ 加载索引数据库失败，无法进行GetFileProof测试" << std::endl;
        return;
    }
    // 自己实现GetFileProof函数的测试逻辑
    auto it = index_database.find(file_id);
    if (it == index_database.end()) {
        std::cerr << "❌ 文件不存在: " << file_id << std::endl;
        return;
    }
    const IndexEntry& entry = it->second;
    std::cout << "   ✅ 找到文件" << std::endl;

    const std::vector<std::string>& TS_F = entry.TS_F;
    int n = TS_F.size();  // 块数量
    std::string PK = entry.PK;

    std::string ciphertext;
    load_encrypted_file(file_id, ciphertext);

    // seed定义为固定值以便测试，与自动生成的seed相同
    std::string seed = "fb11610a83e1eab28714cb7363f4764f82b022f0c70d89c42b8d68a5f9f5c344";
    file_proof.seed = seed;

    file_proof.pk = PK;
    file_proof.ID_F = file_id;

    // 开始计算证明proof
    element_t phi;
    mpz_t psi;
    element_init_G1(phi, pairing);
    element_set1(phi);
    mpz_init_set_ui(psi, 0);

    
    std::cout << "GetFileProof end" << std::endl;
}


void testVerifyFileProof_str()
{
    const char* param_str = 
        "type a\n"
        "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
        "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
        "r 730750818665451621361119245571504901405976559617\n"
        "exp2 159\n"
        "exp1 107\n"
        "sign1 1\n"
        "sign0 1\n";

    pairing_t pairing;
    pairing_init_set_buf(pairing, param_str, strlen(param_str));

    std::cout << "✅ pairing 初始化成功" << std::endl;

    mpz_t N,psi, h2_temp, prf_temp;
    element_t pk,g,left,right,phi,mu,zeta_mu,mu_pow_psi;

    mpz_init_set_str(N,"77100882147323929259202707660697850182257322147504210450519405245425484999510534240288025446710114574437574014076458251591453146776013641689093802143952936303738599772833586128652280514157551668081419186182299772668216085721727589152216703947487484154379286657216379752967775433235508106221969798811366993681",10);

    std::string g_str = "26e20d2d87e9eba4cf32df7e0302b15adf21a1a5b4ea3ad1ac5d39fa20af8d67088a8fc5bf65f4509bc705cbe3f090ba095e1e2046edc5bf31d4636629a893894d81602b34c744c98622f0fad4f9ed99e3f8ee103203f4530b68bd3a2293b65e490314c60522dc99049151566df5bea465ac62c581f297ae14cc4fe55d99e6e4";
    std::string pk_str = "9d098b372d6c8944b1c6119500b57ae1d06951a6c7563140ced8d66e59a50269cd5be7d2f25bcd289dfbfcbad4254fb9d62f21e6b436391e186aead8227fa63ba3be5cea8873f186c647c01d1918caca9c6769d9d33acbba09ac9da048b46e86cf728a8c9593be309e106b4d08b11b85b1d22ba12585606e195350fa264450d1";
    std::string phi_str = "9fd5071feb60f66bcac37e7070ded740c2108ef8195b2d592365aede0a72e263b92a33de7b99f514419dc6f6a4d38e32f306cf52278d4d5a7f53d0eb7cd09373109ad0c3e7a0df716cffffe6a1ae5fa2cd5e3a2c5d1a920a3776daf763ff0d389c30e7094126d6316ae8eec7dc32a54649f844f30bb0d4a42ab62a7fdd205d64";
    std::string psi_str = "423ce98e6ffb0028f3d67c6e5df2b4e7bb2d278c445c9f89a6c40109725cf3e413a7362d0b0fe163d7ed1dc76531eab0e4eaaa0459ff212339648fa5d33f1e6c0790e530c4cbf61cd8d91dd8f8d189ab5dedd600e15b820bac2378c9dd65ca87713c25d9a5ee5fd261010456d4e784d21d356dd095a39d7bdeac0d1a0597f208";
    std::string ID_F = "84606356905417135441071610032132613430179155420263986033608988119595160690064";
    std::string seed = "fb11610a83e1eab28714cb7363f4764f82b022f0c70d89c42b8d68a5f9f5c344";
    std::string mu_str = "39d336c989c98716412307348cf14501464697941b4b46772467f2bcc5c9f700c53fa374d007d5a0d5f5fe8c0a129e716fa059adb0b814b3182f1afc562a8f468651a77ef07d78a0510ab3b85ce2773ef6a348753f01236a38adb5f4ef21368b507e04c929ab34345a4c4075a1a410c998f1dd72624bfcb828b3822b9399b3b7";

    element_init_G1(g, pairing);
    element_init_G1(pk, pairing);
    element_init_G1(phi, pairing);
    element_init_G1(mu, pairing);
    element_init_GT(left, pairing);
    element_init_GT(right, pairing);
    element_init_G1(mu, pairing);
    element_init_G1(zeta_mu, pairing);
    element_init_G1(mu_pow_psi, pairing);
    mpz_init(psi);
    mpz_init(h2_temp);
    mpz_init(prf_temp);
    mpz_init_set_str(psi, psi_str.c_str(),10);


    // 反序列化g
    deserializeElement(g_str, g);
    // 反序列化pk
    deserializeElement(pk_str, pk);
    // 反序列化phi
    deserializeElement(phi_str, phi);

    // 反序列化mu
    deserializeElement(mu_str, mu);

    // 计算left = e(phi, g)
    pairing_apply(left, phi, g, pairing);


    // 计算zeta
    element_t zeta;
    element_init_G1(zeta, pairing);
    element_set1(zeta); // zeta 初始化为群的单位元
    for(int i = 0; i<5; i++)
    {
        // 计算prf_temp
        compute_prf(prf_temp, seed, ID_F, i, N);

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
    }

    // 计算mu^psi
    element_pow_mpz(mu_pow_psi, mu, psi);
    element_mul(zeta_mu, zeta, mu_pow_psi);
    // 计算right = e(zeta * mu^psi, pk)
    pairing_apply(right, zeta_mu, pk, pairing);

    // 比较left和right
    int cmp = element_cmp(left, right);
    if(cmp == 0)
        std::cout << "Verify success" << std::endl; 
    else
        std::cout << "Verify failed" << std::endl;
    
}

void computeHashH2(const std::string& input, element_t result) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    element_from_hash(result, hash, SHA256_DIGEST_LENGTH);
}

void compute_prf(mpz_t result, const std::string& seed, const std::string& ID_F, int index, mpz_t N) {
    // 组合输入：seed + ID_F + index
    std::string combined = seed + ID_F + std::to_string(index);
    
    // 使用computeHashH1计算哈希并直接设置到result
    computeHashH1(combined, result, N);
}

void computeHashH1(const std::string& input, mpz_t result, mpz_t N) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()),
           input.length(), hash);
    
    mpz_import(result, SHA256_DIGEST_LENGTH, 1, 1, 0, 0, hash);
    mpz_mod(result, result, N);
}

bool deserializeElement(const std::string& hex_str, element_t elem) {
    // 错误检查1：hex字符串长度必须是偶数
    if (hex_str.length() % 2 != 0) {
        std::cerr << "⚠️  deserializeElement: hex字符串长度必须是偶数，当前长度: " 
                  << hex_str.length() << std::endl;
        return false;
    }
    
    // 错误检查2：hex字符串不能为空
    if (hex_str.empty()) {
        std::cerr << "⚠️  deserializeElement: hex字符串为空" << std::endl;
        return false;
    }
    
    // 步骤1：将hex转换为bytes
    std::vector<unsigned char> bytes = hexToBytes(hex_str);
    if (bytes.empty()) {
        std::cerr << "⚠️  deserializeElement: hex转换为bytes失败" << std::endl;
        return false;
    }
    
    // 步骤2：从bytes反序列化为element
    int bytes_read = element_from_bytes(elem, bytes.data());
    if (bytes_read <= 0) {
        std::cerr << "⚠️  deserializeElement: element_from_bytes失败，返回值: " 
                  << bytes_read << std::endl;
        return false;
    }
    
    // 错误检查3：验证元素不是单位元（无效元素）
    if (element_is1(elem)) {
        std::cerr << "⚠️  deserializeElement: 反序列化后的元素是单位元（无效）" << std::endl;
        return false;
    }
    
    // 所有检查通过
    return true;
}
std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        unsigned char byte = static_cast<unsigned char>(strtol(byte_str.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string bytesToHex(const unsigned char* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

bool load_index_database() {
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
        
    } else {
        std::cerr << "❌ 索引数据库格式错误" << std::endl;
        return false;
    }
    
    return true;
}
bool load_encrypted_file(const std::string& file_id, std::string& ciphertext) {
    std::string file_path = files_dir + "/" + file_id + ".enc";
    
    if (!file_exists(file_path)) {
        return false;
    }
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "❌ 无法读取文件: " << filepath << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    ciphertext = buffer.str();
    return !ciphertext.empty();
}
void testVerifyFileProof_struct(testFileProof file_proof)
{
    return 0;
}