#include <iostream>
#include <string>
#include <vector>
#include <pbc/pbc.h>
#include <gmp.h>
#include <openssl/sha.h>
#include "storage_node.h"

static void choose_type_a_bits(int k, int &rbits, int &qbits) {
    if (k <= 80) {
        rbits = 160;   // 群阶 r 大约 160bit
        qbits = 512;   // 底层域 q 大约 512bit
    } else if (k <= 112) {
        rbits = 224;
        qbits = 1024;
    } else if (k <= 128) {
        rbits = 256;
        qbits = 1536;
    } else if (k <= 192) {
        rbits = 384;
        qbits = 3072;
    } else {  // k >= 192 或更高
        rbits = 512;
        qbits = 7680;
    }
    // 说明：
    // 这是一种经验映射，不是严格标准，但对实验 / 论文代码是够用的。
    // 真正工程化时建议参考最新 pairing 安全性推荐。
}

// 生成 Type A 参数字符串（q, h, r, exp1, exp2, sign0, sign1）
std::string generate_type_a_param_str(int security_k) {
    int rbits, qbits;
    choose_type_a_bits(security_k, rbits, qbits);

    pbc_param_t par;
    pbc_param_init_a_gen(par, rbits, qbits);  // PBC 自动生成一条 Type A 曲线参数

    // 把参数导出为字符串（和你现在手写的 param_str 一样的格式）
    char *buf = nullptr;
    size_t size = 0;

    // 使用 GNU 的 open_memstream 把 pbc_param_out_str 输出到内存
    FILE *fp = open_memstream(&buf, &size);
    if (!fp) {
        std::cerr << "❌ 无法创建内存流" << std::endl;
        pbc_param_clear(par);
        return "";
    }

    pbc_param_out_str(fp, par);  // 输出到 fp
    fclose(fp);                  // 关闭后，buf/size 中就有完整字符串

    std::string param_str(buf, size);
    free(buf);
    pbc_param_clear(par);

    return param_str;
}

// 示例：初始化 pairing，并顺便初始化 g, mu, N = r
bool init_pairing_with_security(pairing_t pairing,
                                element_t &g,
                                element_t &mu,
                                mpz_t &N,
                                int security_k) {
    std::string param_str = generate_type_a_param_str(security_k);
    if (param_str.empty()) {
        std::cerr << "❌ 生成配对参数失败" << std::endl;
        return false;
    }

    // 用生成的参数字符串初始化 pairing
    if (pairing_init_set_buf(pairing, param_str.c_str(), param_str.size()) != 0) {
        std::cerr << "❌ 配对初始化失败" << std::endl;
        return false;
    }

    // 初始化 G1 中的公共参数 g, mu
    element_init_G1(g, pairing);
    element_init_G1(mu, pairing);
    element_random(g);
    element_random(mu);

    // 关键点：N 建议直接取为群阶 r（安全 & 一致）
    // 从 pairing 提取群阶 r：用一个 Zr 元素的阶
    element_t zr;
    element_init_Zr(zr, pairing);
    // zr 所在的环是 Z_r，N 直接用它的模数即可
    // PBC 里没有直接 element_to_mpz，常见做法是使用 element_to_bytes 再 mpz_import，
    // 但作为简单实现，你也可以直接把 N 设为 2^rbits 级别的大数；
    // 这里给一个简单的“从参数字符串再解析 r”的方式会太长，就先用“N = 2^rbits”做演示。
    // 对你的 PoR 协议来说，更关键的是：指数在使用前要 mod 群阶 r，
    // 而不是像之前那样用 p*p。

    // 简单起见：这里示例用 group order 的 bit 长度近似 N（实验环境够用）
    // 若要更严谨，可以从 param_str 里把 "r ..." 那一行 parse 出来给 N。
    mpz_init(N);
    // 假设安全级别 security_k 对应的 rbits 同 choose_type_a_bits
    int rbits, qbits;
    choose_type_a_bits(security_k, rbits, qbits);
    mpz_ui_pow_ui(N, 2, rbits); // N ≈ 2^rbits，仅用于 mod，实验足够

    element_clear(zr);
    std::cout << "✅ pairing 初始化成功，安全级别约 " << security_k << " bit" << std::endl;
    return true;
}

// 辅助函数：十六进制字符串转字节数组
std::vector<unsigned char> hexToBytes(const std::string& hex) {
    std::vector<unsigned char> bytes;
    std::cout<<hex.length()<<std::endl;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

// 辅助函数：从十六进制字符串反序列化element
bool deserializeElement(const std::string& hex_str, element_t elem){
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
        printf("element_is1 returned 1\n");
    }
    
    // 所有检查通过
    return true;
}

/**
 * 文件证明验证函数
 * 
 * 验证等式: e(psi, g) = e(phi, mu)
 * 
 * @param pairing 配对参数
 * @param psi_hex ψ值的十六进制字符串
 * @param phi_hex φ值的十六进制字符串
 * @param g_hex 生成元g的十六进制字符串
 * @param mu_hex 生成元μ的十六进制字符串
 * @param TS_F_hex 认证标签集合（用于额外验证）
 * @return 验证成功返回true，失败返回false
 */
bool verifyFileProof(
    pairing_t pairing,
    const std::string& psi_hex,
    const std::string& phi_hex,
    const std::string& g_hex,
    const std::string& mu_hex,
    const std::string& seed,
    const std::vector<std::string>& TS_F_hex,
    const std::string& ID_F,
    const std::string& PK
)
{
    StorageNode node; //调用辅助函数
    // 初始化 element
    element_t  phi, g, mu;
    mpz_t psi;
    element_init_G1(phi, pairing);
    element_init_G1(g, pairing);
    element_init_G1(mu, pairing);
    
    // 反序列化参数
    
    mpz_init(psi);
    mpz_set_str(psi, psi_hex.c_str(), 16);

    
    if (!deserializeElement(phi_hex, phi)) return false;
    if (!deserializeElement(g_hex, g)) return false;
    if (!deserializeElement(mu_hex, mu)) return false;
    
    // 计算mu^psi
    element_pow_mpz(mu, mu, psi);
    std::cout << "✅ 计算 mu^psi 成功" << std::endl;

    
    element_t left, right;
    element_init_GT(left, pairing);
    element_init_GT(right, pairing);
    
    // 计算 e(psi, g)
    std::cout << "计算 e(psi, g)..." << std::endl;

    pairing_apply(left, phi, g, pairing);
    
    // right
    std::cout << "计算 e(zeta*mu^psi,pk)" << std::endl;
    element_t zeta_mu_psi;
    element_init_G1(zeta_mu_psi, pairing);
    element_set(zeta_mu_psi, mu);
    for(int i = 0; i < TS_F_hex.size(); ++i)
    {
        element_t h2_temp;
        mpz_t prf_temp;
        element_init_G1(h2_temp, pairing);
        mpz_init(prf_temp);
        node.compute_prf(prf_temp, seed, ID_F, i);
        std::string id_with_index = ID_F + std::to_string(i);
        node.computeHashH2(id_with_index ,h2_temp);
        
        element_t temp_pow;
        element_init_G1(temp_pow, pairing);
        element_pow_mpz(temp_pow, h2_temp, prf_temp);
        

        element_mul(zeta_mu_psi, zeta_mu_psi, temp_pow);
        element_clear(h2_temp);
        mpz_clear(prf_temp);
        element_clear(temp_pow);
    }
    element_t pk;
    element_init_G1(pk, pairing);
    if (!deserializeElement(PK, pk)) return false;

    pairing_apply(right, zeta_mu_psi, pk, pairing);

    // 验证等式: e(psi, g) = e(phi, mu)
    
    bool result = (element_cmp(left, right) == 0);
    
    // 显示左边值
    char* left_str = new char[element_length_in_bytes(left) * 2 + 100];
    element_snprint(left_str, element_length_in_bytes(left) * 2 + 100, left);
    std::cout << "e(phi, g)  = " << std::string(left_str).substr(0, 60) << "..." << std::endl;
    delete[] left_str;
    
    // 显示右边值
    char* right_str = new char[element_length_in_bytes(right) * 2 + 100];
    element_snprint(right_str, element_length_in_bytes(right) * 2 + 100, right);
    std::cout << "e(zeta*mu^psi,pk) = " << std::string(right_str).substr(0, 60) << "..." << std::endl;
    delete[] right_str;
    
    std::cout << "\n等式结果: " << (result ? "相等 ✓" : "不相等 ✗") << std::endl;
    
    // 清理
    element_clear(phi);
    element_clear(g);
    element_clear(mu);
    element_clear(left);
    element_clear(right);
    
    return result;
}

/**
 * 主测试函数
 */  
int main() {

    
    // 1. 文件标识信息
    std::string ID_F = "29623136847719743332609599635319152073467003710545598034443509938335505712094";
    std::string PK = "0b97c9dd3c4a8a90ca1cdb176e9371560aafca31d731bd206b1a71cd22b41150c6f174cc714cb0ca4e010cd732db3eb058235001f10d9e7ae974e69e3cad33e097c7131975117f1d1945e09c7a9e529e30e964ec6e173cfada128a5320fe82dadd6ba055fc2a6423383ed1069438ae72eae926c30a35160d50c7d192d81c5c71"; // 示例公钥
    std::string seed = "2681e7985a73b14c3d9d5c6110ff8bce34d9feaf666a382982b46bd2d11a2c7a";

    std::cout << "文件 ID: " << ID_F << std::endl;
    std::cout << "公钥 PK: " << PK << std::endl;


    char* param_str;
    param_str = generate_type_a_param_str(80).data();
    printf("生成的 Type A 参数:\n%s\n", param_str);

    // 3. 初始化配对
    pairing_t pairing;
    pairing_init_set_buf(pairing, param_str, strlen(param_str));

    
    // N (大素数，mpz_t类型，十六进制)
    std::string N_hex = "77100882147323929259202707660697850182257322147504210450519405245425484999510534240288025446710114574437574014076458251591453146776013641689093802143952936303738599772833586128652280514157551668081419186182299772668216085721727589152216703947487484154379286657216379752967775433235508106221969798811366993681";
    
    // g (G1群生成元，十六进制)
    std::string g_hex = "a0b41b546a2b80478d7f5e98f5ec150703a2fea61e69a5de9694b10bd8009a67461cafca84540ccf0e7d5170da267003308fd14de20cb5c6eaf913edbfe00697385d1115e98f5a0c91ac979dd153f6e52ea2271be39babab3cde10fc5613c09d3e442237a054e7458d98df69077e07bbf87f74322bacee29527c37dee33b3cf7"; // 示例数据，需替换为真实值
    
    // mu (G1群元素，十六进制)
    std::string mu_hex = "0e231739ec082c1972c9dcfc31351bcd2e8a44f5a94e370ec8eed3902402cef20ff24950713d29dd42c0549eb16c4706bedbebf519a73fe76e5231cf55ed400c9591efe922d84862ada73dab6d1ecf677e78483fe94dd54e1471aba4bfda571a59db52cc112348dfea6963d6105d290ceba7335aeaebeb7674f908eaefcf0bc9"; // 示例数据，需替换为真实值
    
    // 5. 文件证明数据 (十六进制字符串格式)
    // 注意：这些是示例数据，实际使用时需要替换为从 GetFileProof 生成的真实数据
    
    // psi (ψ - 累积证明，G1群元素)
    std::string psi_hex = "32d4a4eee8d2da533bd53d7018b2fef913c3cebfd10619bc24f97c64dc442d01f400f283933a626ee9fcbffc380ec5e90ddffc39b83aedc435f0f6f7c36027c94fee8c9a99d3461fe4935592fb3c87fbb902f5a1e2b7cacb48dccee7fb76f11732aea73ddd74aaec9512b453acf2d590bd71b6a2327460502c3d6a6f2ca9c233"; // 示例数据，需替换为真实值
    
    // phi (φ - 累积签名，G1群元素)
    std::string phi_hex = "63c0bc2ba31edf6b6c3eaaf2bd196b592e023eb1e2d0a5bc9791e117dfd4232de6cc7d95607ed7c596fb762144aa0a371b5960b8d5d35845d1b020222c601d8614d2c1b542a468f77b2c840e7253fc1e632af906f93bf0c50b9e1234b432b33bdda55ec96e893d26dd744876137d3f3ed713348df2fb936a292023bedb818d6d"; // 示例数据，需替换为真实值
    
    // 6. 文件认证标签集合 (TS_F)
    std::vector<std::string> TS_F_hex;
    TS_F_hex.push_back("4168db53e17a10752582c988d9d72ad274e3a966beee4e74885e9166a70d99f4967c7161e90ca9edc8bf0395c22a73c072fa52ddc05245647154c92d9ae7b8ca43a84516baa8fd3311311e60916da2d1befc08029ca1436cb9d3efd240dfe8a00038e325fe9f3669c361de79eaa5681509a4c3a52027e1fd478b4a6f2984adcf");
    TS_F_hex.push_back("22d51f079cf2724df8d0dc9dc9616897c0fa1795f09fc5bccc166f8e8da8e74b1d992a2e3e5ee3211433aee275f7cbe8fab6ca77bdf4cf8206c66ebca2d297ff75539f834f3a37be10fe51b1f17d565b0949d21f23e6ee09e108f850a37170c3e6b6120e9bf567c8f07b4f281618ac063545ba58dfbec9d67ac33f7081bf4924");
    TS_F_hex.push_back("941f1cfb6473e6147b2a841bd217f88a120d44eb726e596986fe6a3e9d1e21af6b2746b0e09d194d9b4015ed9386195b2dfac61573bc842dda74a58c60eb4e027d76caf27333620e0982688c8236af1071f7661af709f30d05e5260f1553ded6ffb4f22a54c8a04900e3b6c5f66ce633b9a549ba43edf7c0fe298f08dd8a1a62");
    TS_F_hex.push_back("76de7a9d4e8cd1179a9bdc05c2817b33b125416a36e8d10a807841d2ee31b95d5f78a77034ed904cd4d6abdd694bc039bcc7b821c470674716616c478798699f1b30889b09db6184c96d312079b1d85df16d83fd9a2dc12b6b5a02b18251fa0267d41bb390a9d2e6564c1551eeae1d6ac0374b836ddb1bdc4a04690cff5a9af2");
    
    // 执行验证

    bool verification_result = verifyFileProof(
        pairing,
        psi_hex,
        phi_hex,
        g_hex,
        mu_hex,
        seed,
        TS_F_hex,
        ID_F,
        PK
    );
    
    if (verification_result) {
        std::cout << "║  ✅ 验证成功！文件证明有效                        ║\n";
        std::cout << "║  文件数据完整性得到确认                          ║\n";
    } else {
        std::cout << "║  ❌ 验证失败！文件证明无效                        ║\n";
        std::cout << "║  文件可能已被篡改或证明数据不正确                ║\n";
    }
    
    // 清理
    pairing_clear(pairing);
    
    return verification_result ? 0 : 1;
}