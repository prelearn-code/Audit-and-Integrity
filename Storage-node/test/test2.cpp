#include <iostream>
#include <string>
#include <vector>
#include <pbc/pbc.h>
#include <gmp.h>
#include <openssl/sha.h>
#include "storage_node.h"

std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    
    std::cout << "🔍 hexToBytes 调试信息:" << std::endl;
    std::cout << "   - 输入长度: " << hex.length() << std::endl;
    std::cout << "   - 前20字符: " << hex.substr(0, std::min(size_t(20), hex.length())) << std::endl;
    
    // 检查是否包含非法字符
    for (size_t i = 0; i < hex.length(); i++) {
        char c = hex[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
            std::cerr << "   ❌ 发现非法字符 '" << c << "' 在位置 " << i << std::endl;
            return bytes; // 返回空vector
        }
    }
    
    for (size_t i = 0; i < hex.length(); i += 2) {
        if (i + 1 >= hex.length()) {
            std::cerr << "   ⚠️  奇数长度字符串，最后一个字符被忽略" << std::endl;
            break;
        }
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    
    std::cout << "   - 输出字节数: " << bytes.size() << std::endl;
    std::cout << "   - 前10字节: ";
    for (size_t i = 0; i < std::min(size_t(10), bytes.size()); i++) {
        printf("%02x ", bytes[i]);
    }
    std::cout << std::endl;
    
    return bytes;
}

bool deserializeElement(const std::string& hex_str, element_t elem, const std::string& name, pairing_t pairing) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "🔍 反序列化 " << name << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    // 检查1：hex字符串长度必须是偶数
    if (hex_str.length() % 2 != 0) {
        std::cerr << "❌ hex字符串长度必须是偶数，当前长度: " << hex_str.length() << std::endl;
        return false;
    }
    
    // 检查2：hex字符串不能为空
    if (hex_str.empty()) {
        std::cerr << "❌ hex字符串为空" << std::endl;
        return false;
    }
    
    // 获取预期的字节长度
    int expected_length = element_length_in_bytes(elem);
    std::cout << "📏 Element预期字节长度: " << expected_length << std::endl;
    std::cout << "📏 Hex字符串长度: " << hex_str.length() << " (对应 " 
              << hex_str.length() / 2 << " 字节)" << std::endl;
    
    // 步骤1：将hex转换为bytes
    std::vector<unsigned char> bytes = hexToBytes(hex_str);
    if (bytes.empty()) {
        std::cerr << "❌ hex转换为bytes失败" << std::endl;
        return false;
    }
    
    // 检查字节长度是否匹配
    if (bytes.size() != static_cast<size_t>(expected_length)) {
        std::cerr << "⚠️  警告: 字节长度不匹配！" << std::endl;
        std::cerr << "   - 预期: " << expected_length << " 字节" << std::endl;
        std::cerr << "   - 实际: " << bytes.size() << " 字节" << std::endl;
        std::cerr << "   - 这可能导致反序列化失败或得到错误的element" << std::endl;
    }
    
    // 步骤2：从bytes反序列化为element
    std::cout << "🔄 调用 element_from_bytes..." << std::endl;
    int bytes_read = element_from_bytes(elem, bytes.data());
    std::cout << "   - 返回值: " << bytes_read << std::endl;
    
    if (bytes_read <= 0) {
        std::cerr << "❌ element_from_bytes失败，返回值: " << bytes_read << std::endl;
        return false;
    }
    
    // 检查3：验证元素不是单位元（这是关键检查！）
    if (element_is1(elem)) {
        std::cerr << "❌ 反序列化后的元素是单位元（值为1）！" << std::endl;
        std::cerr << "   可能的原因：" << std::endl;
        std::cerr << "   1. 输入的十六进制数据不正确" << std::endl;
        std::cerr << "   2. 序列化和反序列化使用的配对参数不一致" << std::endl;
        std::cerr << "   3. 字节长度不匹配" << std::endl;
        std::cerr << "   4. 数据来源有误（可能不是element序列化的结果）" << std::endl;
        return false;
    }
    
    // 检查4：验证元素是否在正确的群中
    if (element_is0(elem)) {
        std::cerr << "❌ 反序列化后的元素是零元（无效）" << std::endl;
        return false;
    }
    
    std::cout << "✅ " << name << " 反序列化成功" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    return true;
}

/**
 * 测试序列化和反序列化的一致性
 */
void testSerializationConsistency(pairing_t pairing) {
    std::cout << "\n╔═══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║   序列化/反序列化一致性测试                      ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════╝\n" << std::endl;
    
    // 生成一个随机的G1元素
    element_t test_elem, restored_elem;
    element_init_G1(test_elem, pairing);
    element_init_G1(restored_elem, pairing);
    
    // 随机生成
    element_random(test_elem);
    std::cout << "1️⃣ 生成随机element..." << std::endl;
    
    // 序列化
    int len = element_length_in_bytes(test_elem);
    unsigned char* buf = new unsigned char[len];
    element_to_bytes(buf, test_elem);
    
    std::cout << "2️⃣ 序列化为字节数组 (" << len << " 字节)..." << std::endl;
    std::cout << "   前10字节: ";
    for (int i = 0; i < std::min(10, len); i++) {
        printf("%02x ", buf[i]);
    }
    std::cout << std::endl;
    
    // 转换为十六进制字符串
    std::string hex_str;
    char temp[3];
    for (int i = 0; i < len; i++) {
        sprintf(temp, "%02x", buf[i]);
        hex_str += temp;
    }
    
    std::cout << "3️⃣ 转换为十六进制字符串 (" << hex_str.length() << " 字符)..." << std::endl;
    std::cout << "   前40字符: " << hex_str.substr(0, 40) << "..." << std::endl;
    
    // 反序列化
    std::cout << "4️⃣ 从十六进制字符串反序列化..." << std::endl;
    bool success = deserializeElement(hex_str, restored_elem, "test_elem", pairing);
    
    if (!success) {
        std::cerr << "❌ 反序列化失败！" << std::endl;
        delete[] buf;
        element_clear(test_elem);
        element_clear(restored_elem);
        return;
    }
    
    // 比较
    std::cout << "5️⃣ 比较原始element和恢复的element..." << std::endl;
    bool match = (element_cmp(test_elem, restored_elem) == 0);
    
    if (match) {
        std::cout << "✅ 测试通过！序列化和反序列化一致" << std::endl;
    } else {
        std::cerr << "❌ 测试失败！序列化和反序列化不一致" << std::endl;
    }
    
    // 清理
    delete[] buf;
    element_clear(test_elem);
    element_clear(restored_elem);
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
}

/**
 * 检查G1元素的预期长度
 */
void checkG1ElementLength(pairing_t pairing) {
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    std::cout << "📏 检查G1元素长度" << std::endl;
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    
    element_t temp;
    element_init_G1(temp, pairing);
    
    int g1_length = element_length_in_bytes(temp);
    std::cout << "G1 元素字节长度: " << g1_length << std::endl;
    std::cout << "对应的十六进制字符串长度: " << (g1_length * 2) << std::endl;
    
    element_clear(temp);
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
}

/**
 * 文件证明验证函数（简化版，专注于调试）
 */
bool verifyFileProof(
    pairing_t pairing,
    const std::string& psi_hex,
    const std::string& phi_hex,
    const std::string& g_hex,
    const std::string& mu_hex,
    const std::vector<std::string>& TS_F_hex)
{
    std::cout << "\n╔═══════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║          开始文件证明验证                         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════╝\n" << std::endl;
    
    // 初始化 element
    element_t psi, phi, g, mu;
    element_init_G1(psi, pairing);
    element_init_G1(phi, pairing);
    element_init_G1(g, pairing);
    element_init_G1(mu, pairing);
    
    // 反序列化参数（带详细诊断）
    if (!deserializeElement(psi_hex, psi, "psi", pairing)) {
        element_clear(psi); element_clear(phi); 
        element_clear(g); element_clear(mu);
        return false;
    }
    
    if (!deserializeElement(phi_hex, phi, "phi", pairing)) {
        element_clear(psi); element_clear(phi); 
        element_clear(g); element_clear(mu);
        return false;
    }
    
    if (!deserializeElement(g_hex, g, "g", pairing)) {
        element_clear(psi); element_clear(phi); 
        element_clear(g); element_clear(mu);
        return false;
    }
    
    if (!deserializeElement(mu_hex, mu, "mu", pairing)) {
        element_clear(psi); element_clear(phi); 
        element_clear(g); element_clear(mu);
        return false;
    }
    
    std::cout << "\n📊 标签数量: " << TS_F_hex.size() << std::endl;
    
    // 计算配对
    std::cout << "\n🔢 计算双线性配对..." << std::endl;
    
    element_t left, right;
    element_init_GT(left, pairing);
    element_init_GT(right, pairing);
    
    // 计算 e(psi, g)
    std::cout << "   计算 e(psi, g)..." << std::endl;
    pairing_apply(left, psi, g, pairing);
    
    // 计算 e(phi, mu)
    std::cout << "   计算 e(phi, mu)..." << std::endl;
    pairing_apply(right, phi, mu, pairing);
    
    // 验证等式
    std::cout << "\n✓ 验证等式 e(psi, g) = e(phi, mu)..." << std::endl;
    bool result = (element_cmp(left, right) == 0);
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
    if (result) {
        std::cout << "✅ 验证成功：等式成立" << std::endl;
    } else {
        std::cout << "❌ 验证失败：等式不成立" << std::endl;
    }
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    
    // 清理
    element_clear(psi);
    element_clear(phi);
    element_clear(g);
    element_clear(mu);
    element_clear(left);
    element_clear(right);
    
    return result;
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║     文件证明验证 - 诊断调试版本                  ║\n";
    std::cout << "║     File Proof Verification - Debug Version      ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n";
    std::cout << std::endl;
    
    // 配对参数
    const char* param_str = 
        "type a\n"
        "q 8780710799663312522437781984754049815806883199414208211028653399266475630880222957078625179422662221423155858769582317459277713367317481324925129998224791\n"
        "h 12016012264891146079388821366740534204802954401251311822919615131047207289359704531102844802183906537786776\n"
        "r 730750818665451621361119245571504901405976559617\n"
        "exp2 159\n"
        "exp1 107\n"
        "sign1 1\n"
        "sign0 1\n";
    
    // 初始化配对
    std::cout << "🔧 初始化配对参数..." << std::endl;
    pairing_t pairing;
    if (pairing_init_set_buf(pairing, param_str, strlen(param_str)) != 0) {
        std::cerr << "❌ 配对初始化失败" << std::endl;
        return 1;
    }
    std::cout << "✅ 配对参数初始化成功\n" << std::endl;
    
    // ========== 关键诊断步骤 ==========
    
    // 步骤1：检查G1元素的预期长度
    checkG1ElementLength(pairing);
    
    // 步骤2：测试序列化/反序列化一致性
    testSerializationConsistency(pairing);
    
    // ========== 实际验证 ==========
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "开始实际数据验证" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // 你的实际数据
    std::string g_hex =  "a0b41b546a2b80478d7f5e98f5ec150703a2fea61e69a5de9694b10bd8009a67461cafca84540ccf0e7d5170da267003308fd14de20cb5c6eaf913edbfe00697385d1115e98f5a0c91ac979dd153f6e52ea2271be39babab3cde10fc5613c09d3e442237a054e7458d98df69077e07bbf87f74322bacee29527c37dee33b3cf7";
    std::string mu_hex = "0e231739ec082c1972c9dcfc31351bcd2e8a44f5a94e370ec8eed3902402cef20ff24950713d29dd42c0549eb16c4706bedbebf519a73fe76e5231cf55ed400c9591efe922d84862ada73dab6d1ecf677e78483fe94dd54e1471aba4bfda571a59db52cc112348dfea6963d6105d290ceba7335aeaebeb7674f908eaefcf0bc9";
    std::string psi_hex = "32d4a4eee8d2da533bd53d7018b2fef913c3cebfd10619bc24f97c64dc442d01f400f283933a626ee9fcbffc380ec5e90ddffc39b83aedc435f0f6f7c36027c94fee8c9a99d3461fe4935592fb3c87fbb902f5a1e2b7cacb48dccee7fb76f11732aea73ddd74aaec9512b453acf2d590bd71b6a2327460502c3d6a6f2ca9c233";
    std::string phi_hex = "63c0bc2ba31edf6b6c3eaaf2bd196b592e023eb1e2d0a5bc9791e117dfd4232de6cc7d95607ed7c596fb762144aa0a371b5960b8d5d35845d1b020222c601d8614d2c1b542a468f77b2c840e7253fc1e632af906f93bf0c50b9e1234b432b33bdda55ec96e893d26dd744876137d3f3ed713348df2fb936a292023bedb818d6d";
    
    std::vector<std::string> TS_F_hex;
    TS_F_hex.push_back("4168db53e17a10752582c988d9d72ad274e3a966beee4e74885e9166a70d99f4967c7161e90ca9edc8bf0395c22a73c072fa52ddc05245647154c92d9ae7b8ca43a84516baa8fd3311311e60916da2d1befc08029ca1436cb9d3efd240dfe8a00038e325fe9f3669c361de79eaa5681509a4c3a52027e1fd478b4a6f2984adcf");
    TS_F_hex.push_back("22d51f079cf2724df8d0dc9dc9616897c0fa1795f09fc5bccc166f8e8da8e74b1d992a2e3e5ee3211433aee275f7cbe8fab6ca77bdf4cf8206c66ebca2d297ff75539f834f3a37be10fe51b1f17d565b0949d21f23e6ee09e108f850a37170c3e6b6120e9bf567c8f07b4f281618ac063545ba58dfbec9d67ac33f7081bf4924");
    TS_F_hex.push_back("941f1cfb6473e6147b2a841bd217f88a120d44eb726e596986fe6a3e9d1e21af6b2746b0e09d194d9b4015ed9386195b2dfac61573bc842dda74a58c60eb4e027d76caf27333620e0982688c8236af1071f7661af709f30d05e5260f1553ded6ffb4f22a54c8a04900e3b6c5f66ce633b9a549ba43edf7c0fe298f08dd8a1a62");
    TS_F_hex.push_back("76de7a9d4e8cd1179a9bdc05c2817b33b125416a36e8d10a807841d2ee31b95d5f78a77034ed904cd4d6abdd694bc039bcc7b821c470674716616c478798699f1b30889b09db6184c96d312079b1d85df16d83fd9a2dc12b6b5a02b18251fa0267d41bb390a9d2e6564c1551eeae1d6ac0374b836ddb1bdc4a04690cff5a9af2");
    
    bool result = verifyFileProof(pairing, psi_hex, phi_hex, g_hex, mu_hex, TS_F_hex);
    
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════╗\n";
    if (result) {
        std::cout << "║  ✅ 最终结果：验证成功                           ║\n";
    } else {
        std::cout << "║  ❌ 最终结果：验证失败                           ║\n";
    }
    std::cout << "╚═══════════════════════════════════════════════════╝\n";
    std::cout << std::endl;
    
    pairing_clear(pairing);
    
    return result ? 0 : 1;
}